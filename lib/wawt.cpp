/** @file wawt.cpp
 *  @brief Windowing User Interface Toolkit
 *
 * Copyright 2018 Bruce Szablak

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wawt.h"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>

namespace BDS {

namespace {

constexpr static const std::size_t kCANVAS    = 0;
constexpr static const std::size_t kTEXTENTRY = 1;
constexpr static const std::size_t kLABEL     = 2;
constexpr static const std::size_t kBUTTON    = 3;
constexpr static const std::size_t kBUTTONBAR = 4;
constexpr static const std::size_t kLIST      = 5;
constexpr static const std::size_t kPANEL     = 6;

using FontIdMap  = std::map<Wawt::FontSizeGrp, uint16_t>;

WawtDump s_defaultAdapter(std::cout);

inline
std::ostream& operator<<(std::ostream& os, WawtDump::Indent indent) {
    if (indent.d_indent > 0) {
        os << std::setw(indent.d_indent) << ' ';
    }
    return os;                                                        // RETURN
}

inline
std::ostream& operator<<(std::ostream& os, const Wawt::WidgetId id) {
    if (id.isSet()) {
        os << id.value();
        if (id.isRelative()) {
            os << "_wr";
        }
        else {
            os << "_w";
        }
    }
    else {
        os << "unset";
    }
    return os;
}

inline int charLng(wchar_t) {
    return 1;
}

inline int charLng(char ch) {
    return (ch & 0340) == 0340 ? ((ch & 020) ? 4 : 3)
                               : ((ch & 0200) ? 2 : 1);
}

inline int charLng(const char *ch) {
    return charLng(ch[0]);
}

inline bool isBackspace(wchar_t ch) {
    return ch == L'\b';
}

inline bool isBackspace(const char *c) {
    return c[0] == '\b';
}

inline bool isEnter(wchar_t ch) {
    return ch == L'\r';
}

inline bool isEnter(const char *c) {
    return c[0] == '\r';
}

inline bool isEos(wchar_t ch) {
    return ch == L'\0';
}

inline bool isEos(const char *c) {
    return c[0] == '\0';
}

inline std::wstring makeString(wchar_t ch) {
    return isEos(ch) ? std::wstring()
                     : std::wstring(1, static_cast<wchar_t>(ch));
}

inline std::string makeString(const char *ch) {
    return isEos(ch) ? std::string() : std::string(ch, charLng(ch));
}

inline auto makeString(char ch) {
    if constexpr(std::is_same_v<Wawt::Char_t,wchar_t>) {
        return makeString(wchar_t(ch));
    }
    else {
        char u8[4] = {ch};
        return makeString(u8);
    }
}

inline int textLength(const std::wstring& text) {
    return text.length();
}

inline int textLength(const std::string& text) {
    int   length = 0;
    auto  p      = text.c_str();
    auto  end    = p + text.length();
    while (p < end && *p) {
        ++length;
        p += charLng(*p);
    }
    return length;
}

void outputXMLString(std::ostream& os, const std::string& text) {
    // NOTE: std::codecvt is deprecated. So, to avoid compiler warnings,
    // and until this is sorted out, lets roll our own.
    // For anticipated use (i.e. test drivers), this is expected to be
    // good enough.
    for (auto it = text.begin(); it != text.end(); ) {
        char ch = *it++;
        if (ch == u8'"') {
            os << "&quot;";
        }
        else if (ch == u'\'') {
            os << "&apos;";
        }
        else if (ch == u'<') {
            os << "&lt;";
        }
        else if (ch == u'>') {
            os << "&gt;";
        }
        else if (ch == u'&') {
            os << "&amp;";
        }
        else if ((ch & 0200) == 0) {
            os.put(ch);
        } // end of 7 bit ASCII
        else if ((ch & 0340) == 0300) {
            auto value = int(ch & 037) << 6;
            
            if (it != text.end()) {
                value |= int(*it++ & 077);
                os << "&#" << value << ';';
            }
        }
        else if ((ch & 0360) == 0340) {
            auto value = int(ch & 017) << 6;
            
            if (it != text.end()) {
                value |= int(*it++ & 077);

                if (it != text.end()) {
                    value <<= 6;
                    value  |= int(*it++ & 077);
                    os << "&#" << value << ';';
                }
            }
        }
        else if ((ch & 0370) == 0360) {
            auto value = int(ch & 07) << 6;
            
            if (it != text.end()) {
                value |= int(*it++ & 077);

                if (it != text.end()) {
                    value <<= 6;
                    value  |= int(*it++ & 077);

                    if (it != text.end()) {
                        value <<= 6;
                        value  |= int(*it++ & 077);
                        os << "&#" << value << ';';
                    }
                }
            }
        }
        else {
            break;
        }
    }
}

Wawt::FocusCb eatMouseUp(int, int, bool) {
    return Wawt::FocusCb();                                           // RETURN
}

inline
int Int(double value) {
    return int(std::round(value));
}

bool findBase(const Wawt::Base          **base,
              const Wawt::Panel&          panel,
              Wawt::WidgetId              id) {
    for (auto& nextWidget : panel.widgets()) {
        if (std::holds_alternative<Wawt::Panel>(nextWidget)) {
            auto& next = std::get<Wawt::Panel>(nextWidget);

            if (findBase(base, next, id)) {
                return true;                                          // RETURN
            }

            if (!next.d_widgetId.isSet() || id < next.d_widgetId) {
                return false;                                         // RETURN
            }

            if (id == next.d_widgetId) {
                *base = &next;
                return true;                                          // RETURN
            }
        }
        else {
            auto& tmp
                = std::visit([](const Wawt::Base& r)-> const Wawt::Base& {
                                        return r;
                                    }, nextWidget);

            if (tmp.d_widgetId == id) {
                *base = &tmp;
                return true;                                          // RETURN
            }

            if (tmp.d_widgetId > id) {
                return false;                                         // RETURN
            }
        }
    }
    return false;                                                     // RETURN
}

bool handleChar(Wawt::Base           *base,
                Wawt::EnterFn        *enterFn,
                uint16_t              maxChars,
                Wawt::Char_t          pressed)
{
    // Return 'true' if "focus" is lost; 'false' if it is retained.
    // Following handles utf8 and wchar_t
    auto text      = base->textView().getText();
    auto cursor    = makeString(Wawt::s_cursor);
    auto pos       = text.rfind(cursor);
    bool hasCursor = Wawt::String_t::npos != pos
                  && pos == (text.length() - cursor.length()); // at end

    if (hasCursor) {
        text.erase(pos, cursor.length());
        base->drawView().selected() = false;
    }

    if (pressed && !isEos(pressed)) {
        if (isBackspace(pressed)) {
            if (!text.empty()) {
                text.pop_back();
            }
        }
        else if (isEnter(pressed)) {
            // Note that callback can modify the string before returning.
            bool ret = (*enterFn) ? (*enterFn)(&text) : true;
            base->textView().setText(text);
            if (ret) { // lost focus? Return without cursor in string
                return true;                                          // RETURN
            }
        }
        else if (textLength(text) < maxChars) {
            text.append(makeString(pressed));
        }
        text.append(cursor);
        base->drawView().selected() = true;
    }
    else if (!hasCursor) {
        text.append(cursor);
        base->drawView().selected() = true;
    }
    base->textView().setText(text);
    return false;                                                     // RETURN
}

Wawt::DrawPosition makeAbsolute(const Wawt::Position& position,
                                const Wawt::Panel&    parent,
                                const Wawt::Panel&    root) {
    auto       base       = position.d_widgetRef.getBasePointer(parent, root);
    auto&      view       = base->adapterView();
    auto&      upperLeft  = view.d_upperLeft;
    auto&      lowerRight = view.d_lowerRight;
    auto       thickness  = view.d_borderThickness;

    auto xorigin = (upperLeft.d_x  + lowerRight.d_x)/2.0;
    auto yorigin = (upperLeft.d_y  + lowerRight.d_y)/2.0;
    auto xradius = lowerRight.d_x - xorigin;
    auto yradius = lowerRight.d_y - yorigin;

    switch (position.d_normalizeX) {
      case Wawt::Normalize::eOUTER:  { // Already have this.
        } break;
      case Wawt::Normalize::eMIDDLE: {
            xradius -= thickness/2;
        } break;
      case Wawt::Normalize::eINNER: {
            xradius -= thickness;
        } break;
      case Wawt::Normalize::eDEFAULT: {
            if (position.d_widgetRef.getWidgetId() == 0_wr) {
                xradius -= thickness; // eINNER
            }
        } break;
    };

    switch (position.d_normalizeY) {
      case Wawt::Normalize::eOUTER:  { // Already have this.
        } break;
      case Wawt::Normalize::eMIDDLE: {
            yradius -= thickness/2;
        } break;
      case Wawt::Normalize::eINNER: {
            yradius -= thickness;
        } break;
      case Wawt::Normalize::eDEFAULT: {
            if (position.d_widgetRef.getWidgetId() == 0_wr) {
                yradius -= thickness; // eINNER
            }
        } break;
    };

    auto x = xorigin + position.d_sX*xradius;
    auto y = yorigin + position.d_sY*yradius;

    return Wawt::DrawPosition{x,y};                                 // RETURN
}

void refreshTextMetric(Wawt::DrawDirective           *args,
                       Wawt::TextBlock               *block,
                       Wawt::DrawAdapter             *adapter_p,
                       FontIdMap&                     fontIdToSize,
                       const Wawt::TextMapper&        textLookup) {
    auto  id         = block->fontSizeGrp();
    auto  textHeight = args->interiorHeight();
    auto  fontSize   = id.has_value() ? fontIdToSize.find(*id)->second : 0;
    auto  charSize   = fontSize > 0   ? fontSize : textHeight;

    if (charSize != args->d_charSize || block->needRefresh()) {
        block->setText(textLookup);
        block->initTextMetricValues(args, adapter_p, charSize);

        if (fontSize == 0 && id.has_value()) {
            fontIdToSize[*id] = args->d_charSize;
        }
    }
    auto  iconSize   = args->d_bulletType != Wawt::BulletType::eNONE
                                      ? args->d_charSize
                                      : 0;
    args->d_startx= args->d_upperLeft.d_x + args->d_borderThickness + iconSize;

    if (block->alignment() != Wawt::Align::eLEFT) {
        auto margin = args->interiorWidth() - iconSize 
                                            - block->metrics().d_textWidth;
        assert(margin >= 0);

        args->d_startx += block->alignment() == Wawt::Align::eRIGHT ? margin
                                                                    : margin/2;
    }
    return;                                                           // RETURN
}

double scaleBorder(const Wawt::Panel& root, double thickness) {
    auto scalex = thickness * root.drawView().width()  / 1000.0;
    auto scaley = thickness * root.drawView().height() / 1000.0;
    return std::ceil(std::max(scalex, scaley));
}

Wawt::FocusCb scroll(Wawt::Panel *root, Wawt::WidgetId id, unsigned int delta)
{
    auto& panel = root->lookup<Wawt::Panel>(id,    "Scroll panel");
    auto& up    = panel.lookup<Wawt::Button>(1_wr, "Scroll up button");
    auto& down  = panel.lookup<Wawt::Button>(2_wr, "Scroll down button");
    auto& list  = panel.lookup<Wawt::List>(3_wr,   "Scroll list");
    list.setStartingRow(list.startRow() + delta, &up, &down);
    return Wawt::FocusCb();                                            // RETURN
}

void setAdapterValues(Wawt::DrawDirective      *args,
                      Wawt::Layout             *layout,
                      const Wawt::Panel&        parent,
                      const Wawt::Panel&        root) {
    args->d_borderThickness = scaleBorder(root, layout->d_thickness);

    args->d_upperLeft  = makeAbsolute(layout->d_upperLeft,  parent, root);
    args->d_lowerRight = makeAbsolute(layout->d_lowerRight, parent, root);

    if (layout->d_pin != Wawt::Vertex::eNONE) {
        auto  square = (args->width() + args->height())/2.0;

        if (square > args->width()) {
            square = args->width();
        }
        else if (square > args->height()) {
            square = args->height();
        }
        auto& ux     = args->d_upperLeft.d_x;
        auto& uy     = args->d_upperLeft.d_y;
        auto& lx     = args->d_lowerRight.d_x;
        auto& ly     = args->d_lowerRight.d_y;
        auto  deltaW = square - args->width();
        auto  deltaH = square - args->height();
        
        switch (layout->d_pin) {
          case Wawt::Vertex::eUPPER_LEFT: {
                ly += deltaH; lx += deltaW;
            } break;
          case Wawt::Vertex::eUPPER_CENTER: {
                ly += deltaH; lx += deltaW/2.0;
                              ux -= deltaW/2.0;
            } break;
          case Wawt::Vertex::eUPPER_RIGHT: {
                ly += deltaH;
                              ux -= deltaW;
            } break;
          case Wawt::Vertex::eCENTER_LEFT: {
                uy -= deltaH/2.0;
                ly += deltaH/2.0; lx += deltaW;
            } break;
          case Wawt::Vertex::eCENTER_CENTER: {
                uy -= deltaH/2.0; ux -= deltaW/2.0;
                ly += deltaH/2.0; lx += deltaW/2.0;
            } break;
          case Wawt::Vertex::eCENTER_RIGHT: {
                uy -= deltaH/2.0; ux -= deltaW;
                ly += deltaH/2.0;
            } break;
          case Wawt::Vertex::eLOWER_LEFT: {
                uy -= deltaH;
                              lx += deltaW;
            } break;
          case Wawt::Vertex::eLOWER_CENTER: {
                uy -= deltaH; ux -= deltaW/2.0;
                              lx += deltaW/2.0;
            } break;
          case Wawt::Vertex::eLOWER_RIGHT: {
                uy -= deltaH; ux -= deltaW;
            } break;
          default: break;
        }
    }
}

} // end unnamed namespace

                            //-----------------
                            // class  Wawt::Base
                            //-----------------

Wawt::Base::Base(const Base& copy)
: d_widgetLabel(copy.d_widgetLabel)
, d_layout(copy.d_layout)
, d_input(copy.d_input)
, d_text(copy.d_text)
, d_draw(copy.d_draw)
, d_widgetId(copy.d_widgetId)
{
    if (d_widgetLabel) {
        *d_widgetLabel = this;
    }
}


Wawt::Base::Base(Base&& copy)
: d_widgetLabel(copy.d_widgetLabel)
, d_layout(std::move(copy.d_layout))
, d_input(std::move(copy.d_input))
, d_text(std::move(copy.d_text))
, d_draw(std::move(copy.d_draw))
, d_widgetId(std::move(copy.d_widgetId))
{
    if (d_widgetLabel) {
        *d_widgetLabel = this;
    }
}

Wawt::Base::Base(Base            **widgetLabel,
                 Layout&&          layout,
                 InputHandler&&    input,
                 TextString&&      text,
                 DrawSettings&&    options)
: d_widgetLabel(widgetLabel)
, d_layout(std::move(layout))
, d_input(std::move(input))
, d_text(std::move(text))
, d_draw(std::move(options))
{
    if (d_widgetLabel) {
        *d_widgetLabel = this;
    }
}

Wawt::Base&
Wawt::Base::operator=(Base&& rhs)
{
    d_widgetLabel = std::move(rhs.d_widgetLabel);
    d_layout      = std::move(rhs.d_layout);
    d_input       = std::move(rhs.d_input);
    d_text        = std::move(rhs.d_text);
    d_draw        = std::move(rhs.d_draw);
    d_widgetId    = std::move(rhs.d_widgetId);

    if (d_widgetLabel) {
        *d_widgetLabel = this;
    }
    return *this;
}

void
Wawt::Base::setEnablement(Enablement newSetting)
{
    switch (newSetting) {
        case Enablement::eHIDDEN: { // hidden & enabled: two calls are needed
            d_draw.d_hidden       = true;
            d_input.d_disabled    = true;
        } break;
        case Enablement::eSHOWN: {
            d_draw.d_hidden       = false;
        } break;
        case Enablement::eDISABLED: {
            d_input.d_disabled    = true;
        } break;
        case Enablement::eENABLED: {
            d_input.d_disabled    = false;
        } break;
        case Enablement::eOFF: {
            d_draw.d_greyEffect   = true;
            d_draw.d_hidden       = false;
            d_input.d_disabled    = true;
        } break;
        case Enablement::eACTIVE: {
            d_draw.d_greyEffect   = false;
            d_draw.d_hidden       = false;
            d_input.d_disabled    = false;
        } break;
        default: abort();
    };
    return;                                                           // RETURN
}

void
Wawt::Base::serialize(std::ostream&  os,
                      const char    *widgetName,
                      bool           container,
                      unsigned int   indent) const
{
    WawtDump::Indent spaces(indent);
    os << spaces << "<" << widgetName << " id='" << d_widgetId;

    if (d_widgetLabel) {
        os << "' label='" << (*d_widgetLabel == this ? "this" : "?");
    }
    os << "'>\n";

    spaces += 2;
    os << spaces << "<layout border='";

    if (d_layout.d_thickness >= 0.0) {
        os << d_layout.d_thickness;
    }

    if (d_layout.d_pin != Vertex::eNONE) {
        os << "' pin='" << int(d_layout.d_pin);
    }
    os << "'>\n";
    spaces += 2;
    os << spaces
       <<     "<ul sx='" << d_layout.d_upperLeft.d_sX
       <<       "' sy='" << d_layout.d_upperLeft.d_sY
       <<   "' widget='" << d_layout.d_upperLeft.d_widgetRef.getWidgetId()
       <<   "' norm_x='" << int(d_layout.d_upperLeft.d_normalizeX)
       <<   "' norm_y='" << int(d_layout.d_upperLeft.d_normalizeX)
       << "'/>\n";
    os << spaces
       <<     "<lr sx='" << d_layout.d_lowerRight.d_sX
       <<       "' sy='" << d_layout.d_lowerRight.d_sY
       <<   "' widget='" << d_layout.d_lowerRight.d_widgetRef.getWidgetId()
       <<   "' norm_x='" << int(d_layout.d_lowerRight.d_normalizeX)
       <<   "' norm_y='" << int(d_layout.d_lowerRight.d_normalizeX)
       << "'/>\n";
    spaces -= 2;
    os << spaces << "</layout>\n";

    os << spaces
       << "<input action='" << int(d_input.d_action)
       << "' disabled='"    << int(d_input.d_disabled)
       << "' variant='"     << d_input.d_callback.index() << "'/>\n";

    os << spaces
       << "<text textId='"  << int(d_text.d_block.d_id)
       << "' align='"       << int(d_text.d_block.d_alignment)
       << "' group='";

    if (d_text.d_block.d_fontSizeGrp.has_value()) {
        os << d_text.d_block.d_fontSizeGrp.value();
    }
    os << "' string='";
    outputXMLString(os, d_text.d_block.d_string);
    os << "'/>\n";

    os << spaces
       << "<draw options='" << int(d_draw.d_options.has_value())
       << "' hidden='"      << int(d_draw.d_hidden)
       << "' paint='"       << (d_draw.d_paintFn ? "set" : "unset")
       << "' bullet='"      << int(d_draw.d_bulletType)
       << "' border='"      << d_draw.d_borderThickness
       << "'>\n";
    spaces += 2;
    os << spaces
       <<     "<ul x='" << d_draw.d_upperLeft.d_x
       <<       "' y='" << d_draw.d_upperLeft.d_y
       << "'/>\n";
    os << spaces
       <<     "<lr x='" << d_draw.d_upperLeft.d_x
       <<       "' y='" << d_draw.d_upperLeft.d_y
       << "'/>\n";
    spaces -= 2;
    os << spaces << "</draw>\n";

    if (!container) {
        spaces -= 2;
        os << spaces << "</" << widgetName << ">\n";
    }
    return;
}

                            //-------------------
                            // class  Wawt::Button
                            //-------------------


                            //----------------------
                            // class  Wawt::ButtonBar
                            //----------------------

void
Wawt::ButtonBar::draw(DrawAdapter *adapter) const
{
    if (Base::draw(adapter)) {
        for (auto& btn : d_buttons) {
            btn.draw(adapter);
        }
    }
}

Wawt::ButtonBar::ButtonBar(ButtonBar                       **indirect,
                           Layout&&                          layout,
                           double                            borderThickness,
                           std::initializer_list<Button>     buttons)
: Base(reinterpret_cast<Base**>(indirect),
       std::move(layout),
       InputHandler().defaultAction(ActionType::eCLICK),
       TextString(),
       DrawSettings())
, d_buttons(buttons)
{
    for (auto& btn : d_buttons) {
        btn.d_input.defaultAction(ActionType::eCLICK);
        btn.d_layout.d_thickness = borderThickness;
    }
}

Wawt::EventUpCb
Wawt::ButtonBar::downEvent(int x, int y)
{
    EventUpCb cb;

    if (!d_input.disabled()
     && d_input.contains(x, y, static_cast<Base*>(this))) {
        for (auto& button : d_buttons) {
            if (cb = button.downEvent(x, y)) {
                break;                                                 // BREAK
            }
        }
    }
    return cb;                                                        // RETURN
}

void
Wawt::ButtonBar::serialize(std::ostream& os, unsigned int indent) const
{
    Base::serialize(os, "bar", true, indent);
    indent += 2;

    for (auto& button : d_buttons) {
        button.serialize(os, "button", false, indent);
    }
    WawtDump::Indent spaces(indent-2);

    os << spaces << "</bar>\n";
    return;                                                           // RETURN
}


                            //-------------------
                            // class  Wawt::Canvas
                            //-------------------

                            //--------------------------
                            // class  Wawt::DrawSettings
                            //--------------------------

bool
Wawt::DrawSettings::draw(DrawAdapter *adapter, const String_t& text) const
{
    if (!d_hidden) {

        if (d_options.has_value() || !text.empty()) {
            adapter->draw(*static_cast<const DrawDirective*>(this), text);
        }

        if (d_paintFn) {
            d_paintFn(d_upperLeft.d_x,
                      d_upperLeft.d_y,
                      d_lowerRight.d_x,
                      d_lowerRight.d_y);
        }
        return true;
    }
    return false;
}

                            //-------------------------
                            // class  Wawt::InputHandler
                            //-------------------------

// PRIVATE METHODS
Wawt::EventUpCb
Wawt::InputHandler::callOnClickCb(int               x,
                                 int               y,
                                 Base             *base,
                                 const OnClickCb&  cb,
                                 bool              callOnDown)
{
    if (!cb) {
        return &eatMouseUp;                                           // RETURN
    }
    auto& draw      = base->drawView();
    bool previous   = draw.selected();
    draw.selected() = true;

    if (callOnDown) {
        cb(false, x, y, base);
    }
    return  [this, base, cb, callOnDown, previous](int xup, int yup, bool up) {
                if (callOnDown || up) {
                    if (up) {
                        base->drawView().selected() = previous;
                        return cb(up, xup, yup, base);
                    }
                    cb(false, xup, yup, base);
                }
                return Wawt::FocusCb();
            };                                                        // RETURN
}

Wawt::EventUpCb
Wawt::InputHandler::callSelectFn(Text            *text,
                                const SelectFn&  cb,
                                bool             callOnDown)
{
    bool previous   = text->d_draw.d_selected;
    bool finalvalue = d_action == ActionType::eCLICK
                        ? false
                        : (d_action == ActionType::eTOGGLE ? !previous : true);

    text->d_draw.d_selected = true;

    if (callOnDown && cb) {
        cb(text);
    }

    return  [this, cb, text, previous, finalvalue](int xup, int yup, bool up) {
                if (up) {
                    if (textcontains(xup, yup, text)) {
                        text->d_draw.d_selected = finalvalue;
                        return cb ? cb(text) : Wawt::FocusCb();
                    }
                    text->d_draw.d_selected = previous;
                }
                return Wawt::FocusCb();
            };                                                        // RETURN
}

Wawt::EventUpCb
Wawt::InputHandler::callSelectFn(Base *base, const SelectFn& cb, bool callOnDown)
{
    bool previous   = base->d_draw.d_selected;
    bool finalvalue = d_action == ActionType::eCLICK
                        ? false
                        : (d_action == ActionType::eTOGGLE ? !previous : true);

    base->d_draw.d_selected = true;

    if (callOnDown && cb) {
        cb(nullptr);
    }

    return  [this, cb, base, previous, finalvalue](int xup, int yup, bool up) {
                if (up) {
                    if (contains(xup, yup, base)) {
                        base->d_draw.d_selected = finalvalue;
                        return cb ? cb(nullptr) : Wawt::FocusCb();
                    }
                    base->d_draw.d_selected = previous;
                }
                return Wawt::FocusCb();
            };                                                        // RETURN
}

// PUBLIC METHODS
Wawt::FocusCb
Wawt::InputHandler::callSelectFn(Text *text)
{
    if (auto p1 = std::get_if<SelectFn>(&d_callback)) {
        if (*p1) {
            return (*p1)(text);                                       // RETURN
        }
    }
    else if (auto p2 = std::get_if<std::pair<SelectFn,bool>>(&d_callback)) {
        if (p2->first) {
            return (p2->first)(text);                                 // RETURN
        }
    }
    else if (auto p3 = std::get_if<std::pair<EnterFn,uint16_t>>(&d_callback)) {
        return  [text,
                 enterFn  = &p3->first,
                 maxChars = p3->second] (Wawt::Char_t c) {
                    return handleChar(text, enterFn, maxChars, c);
                };                                                    // RETURN
    }
    return Wawt::FocusCb();                                           // RETURN
}

bool
Wawt::InputHandler::contains(int x, int y, const Base *base) const
{
    return x >= base->d_draw.d_upperLeft.d_x
        && x <= base->d_draw.d_lowerRight.d_x
        && y >= base->d_draw.d_upperLeft.d_y
        && y <= base->d_draw.d_lowerRight.d_y;                        // RETURN
}

bool
Wawt::InputHandler::textcontains(int x, int y, const Text *text) const
{
    if (d_action == ActionType::eENTRY || text->d_layout.d_thickness > 0) {
        return contains(x, y, text);                                  // RETURN
    }
    auto& view = text->adapterView();
    auto  endx = view.d_startx + text->d_text.metrics().d_textWidth;

    if (y >= view.d_upperLeft.d_y && y <= view.d_lowerRight.d_y) {
        if (x <= endx) {
            if (view.d_bulletType != BulletType::eNONE) {
                return x >= view.d_upperLeft.d_x;                     // RETURN
            }
            return x >= view.d_startx;                                // RETURN
        }
    }
    return false;                                                     // RETURN
}

Wawt::EventUpCb
Wawt::InputHandler::downEvent(int x, int y, Text* text)
{
    if (!disabled() && textcontains(x, y, text)) {
        assert(d_action != ActionType::eINVALID);

        if (std::holds_alternative<std::monostate>(d_callback)) {
            return callSelectFn(text,
                                [](auto) {return FocusCb();},
                                false);                               // RETURN
        }

        if (auto p1 = std::get_if<SelectFn>(&d_callback)) {
            return callSelectFn(text, *p1, false);                    // RETURN
        }

        if (auto p2 = std::get_if<OnClickCb>(&d_callback)) {
            return callOnClickCb(x, y, text, *p2, false);             // RETURN
        }

        if (auto p3 = std::get_if<std::pair<OnClickCb,bool>>(&d_callback)) {
            return callOnClickCb(x, y, text, p3->first, p3->second);  // RETURN
        }

        if (auto p4 = std::get_if<std::pair<SelectFn,bool>>(&d_callback)) {
            return callSelectFn(text, p4->first, p4->second);         // RETURN
        }
        auto p5 = std::get_if<std::pair<EnterFn,uint16_t>>(&d_callback);
        assert(p5);
        auto focusCb = [text,
                        enterFn  = &p5->first,
                        maxChars = p5->second] (Wawt::Char_t c) {
                            return handleChar(text, enterFn, maxChars, c);
                        };
        return [focusCb](int, int, bool up) {
                    return up ? FocusCb(focusCb) : FocusCb();
                };
    }
    return EventUpCb();                                               // RETURN
}

Wawt::EventUpCb
Wawt::InputHandler::downEvent(int x, int y, Base* base)
{
    if (!disabled() && contains(x, y, base)) {
        assert(d_action != ActionType::eINVALID);

        if (std::holds_alternative<std::monostate>(d_callback)) {
            return &eatMouseUp;                                       // RETURN
        }

        if (auto p1 = std::get_if<OnClickCb>(&d_callback)) {
            return callOnClickCb(x, y, base, *p1, false);              // RETURN
        }

        if (auto p2 = std::get_if<std::pair<OnClickCb,bool>>(&d_callback)) {
            return callOnClickCb(x, y, base, p2->first, p2->second);   // RETURN
        }

        if (auto p3 = std::get_if<SelectFn>(&d_callback)) {
            return callSelectFn(base, *p3, false);                     // RETURN
        }
        auto p4 = std::get_if<std::pair<SelectFn,bool>>(&d_callback);
        assert(p4);
        return callSelectFn(base, p4->first, p4->second);              // RETURN
    }
    return EventUpCb();                                               // RETURN
}

                            //------------------
                            // class  Wawt::Label
                            //------------------


                            //--------------------
                            // class  Wawt::Layout
                            //--------------------

Wawt::Layout
Wawt::Layout::slice(bool vertical, double begin, double end)
{
    Layout layout;
    double begin_offset = begin < 0.0 || (begin == 0.0 &&   end < 0.0) ?  1.0
                                                                       : -1.0;
    double   end_offset =   end < 0.0 || (  end == 0.0 && begin < 0.0) ?  1.0
                                                                       : -1.0;

    if (vertical) {
        layout.d_upperLeft.d_sX  =  2.0*begin + begin_offset;
        layout.d_upperLeft.d_sY  = -1.0;
        layout.d_lowerRight.d_sX =  2.0*end   +   end_offset;
        layout.d_lowerRight.d_sY =  1.0;
    }
    else {
        layout.d_upperLeft.d_sX  = -1.0;
        layout.d_upperLeft.d_sY  =  2.0*begin + begin_offset;
        layout.d_lowerRight.d_sX =  1.0;
        layout.d_lowerRight.d_sY =  2.0*end   +   end_offset;
    }
    return layout;                                                    // RETURN
}


                            //-----------------
                            // class  Wawt::List
                            //-----------------

void
Wawt::List::draw(DrawAdapter *adapter) const
{
    if (Base::draw(adapter)) {
        for (auto& btn : d_buttons) {
            btn.draw(adapter);
        }
    }
}

Wawt::List::List(List                            **indirect,
                 Layout&&                          layout,
                 FontSizeGrp                       fontSizeGrp,
                 DrawSettings&&                    options,
                 ListType                          listType,
                 Labels                            labels,
                 const GroupCb&                    click,
                 Panel                            *root)
: Base(reinterpret_cast<Base**>(indirect),
       std::move(layout),
       InputHandler().defaultAction(ActionType::eCLICK),
       TextString(),
       std::move(options))
, d_buttons()
, d_root(root)
, d_rows(0)
, d_startRow(0)
, d_buttonClick(click)
, d_type(listType)
, d_fontSizeGrp(fontSizeGrp)
, d_rowHeight(0)
{
    auto dropDown = d_type == ListType::eDROPDOWNLIST;
    auto rows = labels.size() + (dropDown ? 1 : 0);
    d_buttons.reserve(rows);

    for (auto& [text, checked] : labels) {
        auto& button = d_buttons.emplace_back(Button());
        initButton(d_rows, d_rows == rows-1);
        button.textView().setText(text);
        button.drawView().selected() = checked;
        d_rows += 1;
    }

    if (dropDown) {
        assert(root != nullptr);
        d_buttons.emplace_back(Button());
        initButton(d_rows, true);
        d_rows = 1;
    }
}

Wawt::List::List(List                            **indirect,
                 Layout&&                          layout,
                 FontSizeGrp                       fontSizeGrp,
                 DrawSettings&&                    options,
                 ListType                          listType,
                 unsigned int                      rows,
                 const GroupCb&                    click,
                 Panel                            *root)
: Base(reinterpret_cast<Base**>(indirect),
       std::move(layout),
       InputHandler().defaultAction(ActionType::eCLICK),
       TextString(),
       std::move(options))
, d_buttons()
, d_root(root)
, d_rows(rows)
, d_startRow(0)
, d_buttonClick(click)
, d_type(listType)
, d_fontSizeGrp(fontSizeGrp)
, d_rowHeight(0)
{
    if (d_type == ListType::eDROPDOWNLIST) {
        assert(root != nullptr);
        d_buttons.emplace_back(Button());
        initButton(0, true); // only button in vector
        d_rows = 1;
    }
}

Wawt::List::List(const List& copy)
: Base(copy)
, d_buttons(copy.d_buttons)
, d_root(copy.d_root)
, d_rows(copy.d_rows)
, d_startRow(copy.d_startRow)
, d_buttonClick(copy.d_buttonClick)
, d_type(copy.d_type)
, d_fontSizeGrp(copy.d_fontSizeGrp)
, d_rowHeight(copy.d_rowHeight)
{
    for (auto i = 0u; i < d_buttons.size(); ++i) {
        // since callbacks capture 'this'
        initButton(i, i == d_buttons.size()-1);
    }
}

Wawt::List::List(List&& copy)
: Base(copy)
, d_buttons(std::move(copy.d_buttons))
, d_root(copy.d_root)
, d_rows(copy.d_rows)
, d_startRow(copy.d_startRow)
, d_buttonClick(copy.d_buttonClick)
, d_type(copy.d_type)
, d_fontSizeGrp(copy.d_fontSizeGrp)
, d_rowHeight(copy.d_rowHeight)
{
    for (auto i = 0u; i < d_buttons.size(); ++i) {
        // since callbacks capture 'this'
        initButton(i, i == d_buttons.size()-1);
    }
}

Wawt::List&
Wawt::List::operator=(List&& rhs)
{
    if (this != &rhs) {
        d_buttons       = std::move(rhs.d_buttons);
        d_root          = rhs.d_root;
        d_rows          = rhs.d_rows;
        d_startRow      = rhs.d_startRow;
        d_buttonClick   = rhs.d_buttonClick;
        d_type          = rhs.d_type;
        d_fontSizeGrp   = rhs.d_fontSizeGrp;
        d_rowHeight     = rhs.d_rowHeight;
        Base::operator=(std::move(rhs));

        for (auto i = 0u; i < d_buttons.size(); ++i) {
            // since callbacks capture 'this'
            initButton(i, i == d_buttons.size()-1);
        }
    }
    return *this;
}

Wawt::Button&
Wawt::List::row(unsigned int index)
{
    d_buttons.reserve(index);

    while (index >= d_buttons.size()) {
        if (d_type == ListType::eDROPDOWNLIST) {
            // insert new buttons before the drop-down in the last row
            d_buttons.emplace((d_buttons.rbegin()+1).base(), Button());
            initButton(d_buttons.size()-2, false);
        }
        else {
            d_buttons.emplace_back(Button());
            initButton(d_buttons.size()-1, false);
        }
    }

    if (d_type != ListType::eDROPDOWNLIST && d_buttons.size() > d_rows) {
        auto shownCount = 0u;

        for (auto& button : d_buttons) {
            if (!button.d_draw.d_hidden) {
                shownCount += 1;
            }

            if (shownCount > d_rows) {
                button.d_draw.d_hidden = button.d_input.d_disabled = true;
            }
        }
    }
    setButtonPositions();
    return d_buttons[index];                                          // RETURN
}

void
Wawt::List::itemEnablement(unsigned int index, Enablement setting)
{
    auto& btn = row(index);
    btn.setEnablement(setting);

    if (d_type == ListType::eDROPDOWNLIST && index < d_buttons.size()-1) {
        btn.setEnablement(Enablement::eHIDDEN);
    }
}

Wawt::EventUpCb
Wawt::List::downEvent(int x, int y)
{
    EventUpCb cb;

    if (!d_input.disabled() && d_input.contains(x, y, this)) {
        for (auto& button : d_buttons) {
            if (cb = button.downEvent(x, y)) break;                    // BREAK
        }

        if (!cb) {
            cb = &eatMouseUp;
        }
    }
    return cb;                                                        // RETURN
}

void
Wawt::List::popUpDropDown()
{
    // Append a canvas and a copy of the button-list to the
    // root panel's widget list.  If the canvas is clicked, these two
    // widgets are removed without any further changes.  If the button list
    // is clicked, the label is populated with the selected text, and
    // the root panel's last two widgets are discarded.
    auto clickCb = [root = d_root](bool up, int, int, Base*) {
        if (up) {
            Wawt::removePopUp(root);
        }
        return Wawt::FocusCb();
    };
    auto   nextId  = d_root->d_widgetId;
    auto&  widgets = d_root->d_widgets;
    Canvas canvas({{-1.0, -1.0, 0_wr},{1.0, 1.0, 0_wr}},
                  PaintFn(),
                  {clickCb});
    canvas.d_draw.d_upperLeft  = d_root->d_draw.d_upperLeft;
    canvas.d_draw.d_lowerRight = d_root->d_draw.d_lowerRight;
    canvas.d_draw.d_tracking   = std::make_tuple(kCANVAS, nextId.value(), -1);
    canvas.d_widgetId          = nextId++;
    widgets.emplace_back(canvas);

    auto& dropDown   = std::get<List>(widgets.emplace_back(*this));
    auto  dropDownId = nextId.value();
    dropDown.d_draw.d_tracking    = std::make_tuple(kLIST, dropDownId, -1);
    dropDown.d_widgetId           = nextId++;
    dropDown.d_buttonClick        =
        [this](auto, auto index) {
            d_root->d_widgets.pop_back();
            d_root->d_widgetId
                = std::get<Canvas>(d_root->d_widgets.back()).d_widgetId;
            d_root->d_widgets.pop_back();
            auto& button = d_buttons[index];
            button.callSelectFn();
            return d_buttonClick(this, index);
    };
    auto index = 0u;

    for (auto& btn : dropDown.d_buttons) {
        // The original button was hidden, now show it (possibly greyed out):
        btn.setEnablement(btn.textView().getText().empty()
                ? Enablement::eHIDDEN
                : (btn.adapterView().d_greyEffect ? Enablement::eOFF
                                                  : Enablement::eACTIVE));
        btn.d_input.d_callback = [&dropDown, index](Text*) {
            return dropDown.d_buttonClick(&dropDown, index);
        };
        btn.d_draw.d_tracking = std::make_tuple(kBUTTON, dropDownId, index);
        index += 1;
    }
    dropDown.d_buttons.back().setEnablement(Enablement::eHIDDEN);
    dropDown.d_type = ListType::eSELECTLIST; // no longer a drop-down.
    dropDown.d_draw.d_upperLeft.d_y = dropDown.d_draw.d_lowerRight.d_y;
    dropDown.setButtonPositions(true);

    auto width  = d_root->d_draw.width();
    auto height = d_root->d_draw.height();
    auto ul = dropDown.adapterView().d_upperLeft;
    auto lr = dropDown.adapterView().d_lowerRight;

    // The parent has changed so redefine the relative positions.

    dropDown.d_layout.d_upperLeft  = { 2.0*ul.d_x/width  - 1.0,
                                       2.0*ul.d_y/height - 1.0,
                                       0_wr };
    dropDown.d_layout.d_lowerRight = { 2.0*lr.d_x/width  - 1.0,
                                       2.0*lr.d_y/height - 1.0,
                                       0_wr };

    d_root->d_widgetId = nextId;
    return;                                                           // RETURN
}

void
Wawt::List::initButton(unsigned int index, bool lastButton)
{
    auto& button = d_buttons[index];

    if (d_type == ListType::eRADIOLIST
     || d_type == ListType::eSELECTLIST) {
        button.d_input.d_callback = [p=this, index](Text *clicked) {
            for (auto& nxt : p->d_buttons) {
                nxt.d_draw.d_selected = false;
            }
            clicked->d_draw.d_selected = true;

            if (p->d_buttonClick) {
                return p->d_buttonClick(p, index);
            }
            return FocusCb();
        };
    }
    else if (d_type == ListType::eDROPDOWNLIST) {
        if (lastButton) {
            button.d_input.d_callback = [this](Text*) {
                popUpDropDown();
                return FocusCb();
            };
        }
        else {
            // Use to initialize drop-down selection:
            button.d_input.d_callback = [this](Text *clicked) {
                d_buttons.back()
                         .textView().setText(makeString(s_downArrow)
                                             + makeString(' ')
                                             + clicked->textView().getText());
                return FocusCb();
            };
        }
    }
    else { // "toggle" a check list or pick list button
        button.d_input.d_callback = [p=this, index](Text *) {
            if (p->d_buttonClick) {
                return p->d_buttonClick(p, index);
            }
            return FocusCb();
        };
    }
    button.d_text.fontSizeGrp()       = d_fontSizeGrp;
    button.d_layout.d_thickness       = 1.0; // for click box - do not scale
    button.d_draw.d_bulletType        = BulletType::eNONE;

    switch (d_type) {
        case ListType::eCHECKLIST:   {
            button.d_layout.d_thickness       = 0.0;
            button.d_input.d_action           = ActionType::eTOGGLE;
            button.d_input.d_disabled         = false;
            button.d_text.alignment()         = Align::eLEFT;
            button.d_draw.d_bulletType        = BulletType::eCHECK;
        } break;
        case ListType::eRADIOLIST:   {
            button.d_layout.d_thickness       = 0.0;
            button.d_input.d_action           = ActionType::eBULLET;
            button.d_input.d_disabled         = false;
            button.d_text.alignment()         = Align::eLEFT;
            button.d_draw.d_bulletType        = BulletType::eRADIO;
        } break;
        case ListType::ePICKLIST:    {
            button.d_input.d_action           = ActionType::eTOGGLE;
            button.d_input.d_disabled         = false;
            button.d_text.alignment()         = Align::eCENTER;
        } break;
        case ListType::eSELECTLIST:  {
            button.d_input.d_action           = ActionType::eCLICK;
            button.d_input.d_disabled         = false;
            button.d_text.alignment()         = Align::eCENTER;
        } break;
        case ListType::eVIEWLIST:    {
            button.d_input.d_action           = ActionType::eCLICK;
            button.d_input.d_disabled         = true;
            button.d_text.alignment()         = Align::eLEFT;
        } break;
        case ListType::eDROPDOWNLIST:{
            button.d_input.d_action           = ActionType::eCLICK;
            button.d_input.d_disabled         = !lastButton;
            button.d_text.alignment()         = Align::eCENTER;
            button.d_draw.d_hidden            = !lastButton;
        } break;
        default: abort();
    };
    return;                                                           // RETURN
}

std::vector<uint16_t>
Wawt::List::selectedRows()
{
    std::vector<uint16_t> selected;
    for (auto row = 0u; row < d_buttons.size(); ++row) {
        if (d_buttons[row].drawView().selected()) {
            selected.push_back(row);
        }
    }
    return selected;
}

Wawt::List::Scroll
Wawt::List::setStartingRow(int           row,
                          Button       *upButton,
                          Button       *downButton)
{
    if (d_buttons.size() > d_rows) {
        bool scrollUp = false, scrollDown = false;
        
        if (row <= 0) {
            row = 0;
        }
        else {
            row = std::min(row, int(d_buttons.size()-d_rows));
        }

        for (auto i = 0u; i < d_buttons.size(); ++i) {
            auto& btn = d_buttons[i];

            if (i < unsigned(row)) {
                btn.d_draw.d_hidden    = btn.d_input.d_disabled = true;
                scrollUp               = true;
            }
            else if (i >= row+d_rows) {
                btn.d_draw.d_hidden    = btn.d_input.d_disabled = true;
                scrollDown             = true;
            }
            else {
                btn.d_draw.d_hidden    = false;
                btn.d_input.d_disabled = d_type == ListType::eVIEWLIST;
            }
        }

        if (upButton) {
            upButton->setEnablement(scrollUp ? Enablement::eACTIVE
                                             : Enablement::eHIDDEN);
        }

        if (downButton) {
            downButton->setEnablement(scrollDown ? Enablement::eACTIVE
                                                 : Enablement::eHIDDEN);
        }
        setButtonPositions();
        d_startRow = row;
        return {scrollUp, scrollDown};                                // RETURN
    }

    if (upButton) {
        upButton->setEnablement(Enablement::eHIDDEN);
    }

    if (downButton) {
        downButton->setEnablement(Enablement::eHIDDEN);
    }
    d_startRow = 0;
    return {false, false};                                            // RETURN
}

void
Wawt::List::resetRows()
{
    d_startRow = 0;

    if (d_type == ListType::eDROPDOWNLIST) {
        d_buttons.erase(d_buttons.begin(),
                        (d_buttons.rbegin()+1).base());
    }
    else {
        d_buttons.clear();
    }
    return;                                                           // RETURN
}

void
Wawt::List::setButtonPositions(bool resizeListBox)
{
    auto left_x  = d_draw.d_upperLeft.d_x  + d_draw.d_borderThickness;
    auto right_x = d_draw.d_lowerRight.d_x - d_draw.d_borderThickness;

    auto y       = d_draw.d_upperLeft.d_y  + d_draw.d_borderThickness;
    auto rows    = 0u;

    for (auto& button : d_buttons) {
        button.d_draw.d_upperLeft.d_x   = left_x;
        button.d_draw.d_lowerRight.d_x  = right_x;

        button.d_draw.d_upperLeft.d_y   = y;
        button.d_draw.d_lowerRight.d_y  = y + d_rowHeight;

        // The List's button borders are not scaled.
        button.d_draw.d_borderThickness = button.d_layout.d_thickness;

        assert(button.adapterView().verify());

        if (!button.d_draw.d_hidden) {
            y    += d_rowHeight;
            rows += 1;
        }
    }

    if (resizeListBox) {
        d_draw.d_lowerRight.d_y = y+d_draw.d_borderThickness;
        d_rows = rows; // so d_rowHeight remains the same if recalculated.
    }
    return;                                                           // RETURN
}

void
Wawt::List::serialize(std::ostream& os, unsigned int indent) const
{
    std::string list = "list rows='" + std::to_string(d_rows) + "'";
    Base::serialize(os, list.c_str(), true, indent);
    indent += 2;

    for (auto& button : d_buttons) {
        button.serialize(os, "button", false, indent);
    }
    WawtDump::Indent spaces(indent-2);

    os << spaces << "</list>\n";
    return;                                                           // RETURN
}


                            //----------------------
                            // class  Wawt::TextBlock
                            //----------------------

void
Wawt::TextBlock::initTextMetricValues(Wawt::DrawDirective      *args,
                                      Wawt::DrawAdapter        *adapter,
                                      uint16_t                  upperLimit)
{
    int  width  = args->interiorWidth();
    int  height = args->interiorHeight();

    d_metrics.d_textWidth  = width  - 2;  // these are upper limits
    d_metrics.d_textHeight = height - 2;  // 1 pixel spacing around border.

    auto charSizeLimit
        = (upperLimit > 0 && upperLimit < height) ? upperLimit+1 : height;

    // Note: adapter will factor in the size of any "bullet" if needed.
    adapter->getTextMetrics(args,
                            &d_metrics,
                            d_block.d_string,
                            charSizeLimit);
    return;                                                           // RETURN
}
void
Wawt::TextBlock::setText(TextId id)
{
    d_needRefresh = true;
    d_block.d_id = id;

    if (id != kNOID) {
        d_block.d_string.clear();
    }
}

void
Wawt::TextBlock::setText(Wawt::String_t string)
{
    d_needRefresh = true;
    d_block.d_string = std::move(string);
    d_block.d_id     = kNOID;
}

void
Wawt::TextBlock::setText(const TextMapper& mappingFn)
{
    if (d_block.d_id != kNOID && mappingFn) {
        d_block.d_string      = mappingFn(d_block.d_id);
    }
    d_needRefresh = false; // since refresh ALWAYS follows remap
}

                            //----------------------
                            // class  Wawt::TextEntry
                            //----------------------

                            //------------------
                            // class  Wawt::Panel
                            //------------------

bool
Wawt::Panel::findWidget(Widget **widget, Wawt::WidgetId widgetId)
{
    if (widgetId.isRelative()) {
        if (widgetId.value() > 0) {
            uint16_t offset = widgetId.value();

            for (auto& nextWidget : d_widgets) {
                if (--offset == 0) {
                    *widget = &nextWidget;
                    return true;                                      // RETURN
                }
            }
        }
        return false;                                                 // RETURN
    }

    for (auto& nextWidget : d_widgets) {
        if (std::holds_alternative<Panel>(nextWidget)) {
            auto& panel = std::get<Panel>(nextWidget);

            if (widgetId == panel.d_widgetId) {
                *widget = &nextWidget;
                return true;                                          // RETURN
            }

            if (widgetId < panel.d_widgetId) {
                return panel.findWidget(widget, widgetId);            // RETURN
            }
        }
        else {
            WidgetId id = std::visit([](const Wawt::Base& base) {
                                            return base.d_widgetId;
                                         }, nextWidget);

            if (id == widgetId) {
                *widget = &nextWidget;
                return true;                                          // RETURN
            }

            if (widgetId < id) {
                return false;                                         // RETURN
            }
        }
    }
    return false;                                                     // RETURN
}

Wawt::EventUpCb
Wawt::Panel::downEvent(int x, int y)
{
    EventUpCb cb;

    if (!d_input.disabled() && d_input.contains(x, y, this)) {
        for (auto rit = d_widgets.rbegin(); rit != d_widgets.rend(); ++rit) {
            auto& widget = *rit;

            switch (widget.index()) {
                case kCANVAS: { // Canvas
                    auto& obj = std::get<Canvas>(widget);
                    cb = obj.downEvent(x, y);
                } break;                                               // BREAK
                case kTEXTENTRY: { // TextEntry
                    auto& obj = std::get<TextEntry>(widget);
                    cb = obj.downEvent(x, y);
                } break;                                               // BREAK
                case kLABEL: { // Label
                    auto& obj = std::get<Label>(widget);
                    cb = obj.downEvent(x, y);
                } break;                                               // BREAK
                case kBUTTON: { // Button
                    auto& obj = std::get<Button>(widget);
                    cb = obj.downEvent(x, y);
                } break;                                               // BREAK
                case 4: { // ButtonBar
                    auto& obj = std::get<ButtonBar>(widget);
                    cb = obj.downEvent(x, y);
                } break;                                               // BREAK
                case kLIST: { // List
                    auto& obj = std::get<List>(widget);
                    cb = obj.downEvent(x, y);
                } break;                                               // BREAK
                case kPANEL: { // Panel
                    auto& obj = std::get<Panel>(widget);
                    cb = obj.downEvent(x, y);
                } break;                                               // BREAK
                default: abort();
            }

            if (cb) break;                                             // BREAK
        }
    }
    return cb;                                                        // RETURN
}

void
Wawt::Panel::serialize(std::ostream& os, unsigned int indent) const
{
    Base::serialize(os, "panel", true, indent);
    indent += 2;

    for (auto& widget : d_widgets) {
        switch (widget.index()) {
            case kCANVAS: { // Canvas
                auto& obj = std::get<Canvas>(widget);
                obj.serialize(os, "canvas", false, indent);
            } break;                                               // BREAK
            case kTEXTENTRY: { // TextEntry
                auto& obj = std::get<TextEntry>(widget);
                obj.serialize(os, "entry", false, indent);
            } break;                                               // BREAK
            case kLABEL: { // Label
                auto& obj = std::get<Label>(widget);
                obj.serialize(os, "label", false, indent);
            } break;                                               // BREAK
            case kBUTTON: { // Button
                auto& obj = std::get<Button>(widget);
                obj.serialize(os, "button", false, indent);
            } break;                                               // BREAK
            case 4: { // ButtonBar
                auto& obj = std::get<ButtonBar>(widget);
                obj.serialize(os, indent);
            } break;                                               // BREAK
            case kLIST: { // List
                auto& obj = std::get<List>(widget);
                obj.serialize(os, indent);
            } break;                                               // BREAK
            case kPANEL: { // Panel
                auto& obj = std::get<Panel>(widget);
                obj.serialize(os, indent);
            } break;                                               // BREAK
            default: abort();
        }
    }
    WawtDump::Indent spaces(indent-2);

    os << spaces << "</panel>\n";
    return;                                                           // RETURN
}

                            //-----------------------
                            // class  Wawt::WidgetRef
                            //-----------------------


const Wawt::Base*
Wawt::WidgetRef::getBasePointer(const Panel&      parent,
                                const Panel&      root) const
{
    if (d_base != nullptr) {
        assert(*d_base);
        return *d_base;                                               // RETURN
    }
    const Wawt::Base *base = nullptr;

    if (d_widgetId.isSet()) {
        if (d_widgetId.isRelative()) {
            if (d_widgetId.value() == 0) { // PARENT
                base = &parent;
            }
            else if (d_widgetId.value() == UINT16_MAX-1) { // see: kROOT
                base = &root;
            }
            else {
                uint16_t offset = d_widgetId.value();

                for (auto const& nextWidget : parent.widgets()) {
                    if (--offset == 0) {
                        base = std::visit([](const Wawt::Base& r) {
                                             return &r;
                                          }, nextWidget);
                        break;                                         // BREAK
                    }
                }
            }
        }
        else {
            findBase(&base, root, d_widgetId);
        }
    }

    if (!base) {
        throw Wawt::Exception("Context of widget not found.", d_widgetId);
    }
    return base;                                                      // RETURN
}

Wawt::WidgetId
Wawt::WidgetRef::getWidgetId() const
{
    WidgetId ret = d_widgetId;

    if (!ret.isSet()) {
        if (d_base != nullptr) {
            ret = (*d_base)->d_widgetId;
        }
    }
    return ret;                                                       // RETURN
}

                                //-----------
                                // class  Wawt
                                //-----------

// PRIVATE CLASS METHODS
void
Wawt::setIds(Panel::Widget *widget, Wawt::WidgetId& id)
{
    auto index = widget->index();
    switch (index) {
        case kCANVAS: { // Canvas
            auto& canvas = std::get<Canvas>(*widget);
            canvas.d_widgetId        = id++;
            canvas.d_draw.d_tracking = {index, canvas.d_widgetId.value(), -1};
        } break;                                                   // BREAK
        case kTEXTENTRY: { // TextEntry
            auto& entry = std::get<TextEntry>(*widget);
            entry.d_widgetId        = id++;
            entry.d_draw.d_tracking = {index, entry.d_widgetId.value(), -1};
        } break;                                                   // BREAK
        case kLABEL: { // Label
            auto& label = std::get<Label>(*widget);
            label.d_widgetId        = id++;
            label.d_draw.d_tracking = {index, label.d_widgetId.value(), -1};
        } break;                                                   // BREAK
        case kBUTTON: { // Button
            auto& btn = std::get<Button>(*widget);
            btn.d_widgetId        = id++;
            btn.d_draw.d_tracking = {index, btn.d_widgetId.value(), -1};
        } break;                                                   // BREAK
        case kBUTTONBAR: { // ButtonBar
            auto& bar   = std::get<ButtonBar>(*widget);
            auto  row = 0;
            bar.d_widgetId        = id++;
            bar.d_draw.d_tracking = {index, bar.d_widgetId.value(), -1};

            for (auto& btn : bar.d_buttons) {
                btn.d_draw.d_tracking = {index, bar.d_widgetId.value(), row++};
            }
        } break;                                                   // BREAK
        case kLIST: { // List
            auto& list = std::get<List>(*widget);
            auto  row = 0;
            list.d_widgetId        = id++;
            list.d_draw.d_tracking = {index, list.d_widgetId.value(), -1};

            for (auto& btn : list.d_buttons) {
                btn.d_draw.d_tracking= {index, list.d_widgetId.value(), row++};
            }
        } break;                                                   // BREAK
        case kPANEL: { // Panel
            auto& panel = std::get<Wawt::Panel>(*widget);

            for (auto& nextWidget : panel.d_widgets) {
                setIds(&nextWidget, id);
            }
            panel.d_widgetId        = id++;
            panel.d_draw.d_tracking = {index, panel.d_widgetId.value(), -1};
        } break;                                                   // BREAK
        default: abort();
    }
    return;                                                           // RETURN
}

void
Wawt::setWidgetAdapterPositions(Panel::Widget                  *widget,
                                Panel                          *root,
                                const Panel&                    panel,
                                const BorderThicknessDefaults&  border,
                                const WidgetOptionDefaults&     option)
{
    switch (widget->index()) {
        case kCANVAS: { // Canvas
            auto& canvas = std::get<Wawt::Canvas>(*widget);
            auto& base   = static_cast<Wawt::Base&>(canvas);
            auto& layout = base.d_layout;
            auto& draw   = base.d_draw;

            if (layout.d_thickness < 0.0) {
                layout.d_thickness = border.d_canvasThickness;
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_canvasOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root);

            if (!canvas.adapterView().verify()) {
                throw Wawt::Exception("'Canvas' corners are inverted.",
                                canvas.d_widgetId);                    // THROW
            }
        } break;                                                       // BREAK
        case kTEXTENTRY: { // TextEntry
            auto& entry  = std::get<Wawt::TextEntry>(*widget);
            auto& base   = static_cast<Wawt::Base&>(entry);
            auto& layout = base.d_layout;
            auto& draw   = base.d_draw;

            if (layout.d_thickness < 0.0) {
                layout.d_thickness = border.d_textEntryThickness;
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_textEntryOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root);

            if (!entry.adapterView().verify()) {
                throw Wawt::Exception("'TextEntry' corners are inverted.",
                                entry.d_widgetId);                     // THROW
            }
        } break;                                                       // BREAK
        case kLABEL: { // Label
            auto& label  = std::get<Wawt::Label>(*widget);
            auto& base   = static_cast<Wawt::Base&>(label);
            auto& layout = base.d_layout;
            auto& draw   = base.d_draw;

            if (layout.d_thickness < 0.0) {
                layout.d_thickness = border.d_labelThickness;
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_labelOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root);

            if (!label.adapterView().verify()) {
                throw Wawt::Exception("'Label' corners are inverted.",
                                label.d_widgetId);                     // THROW
            }
        } break;                                                       // BREAK
        case kBUTTON: { // Button
            auto& button = std::get<Wawt::Button>(*widget);
            auto& base   = static_cast<Wawt::Base&>(button);
            auto& layout = base.d_layout;
            auto& draw   = base.d_draw;

            if (layout.d_thickness < 0.0) {
                layout.d_thickness = border.d_buttonThickness;
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_buttonOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root);

            if (!button.adapterView().verify()) {
                throw Wawt::Exception("'Button' corners are inverted.",
                                button.d_widgetId);                    // THROW
            }
        } break;                                                       // BREAK
        case kBUTTONBAR: { // ButtonBar
            auto& bar    = std::get<Wawt::ButtonBar>(*widget);
            auto& base   = static_cast<Wawt::Base&>(bar);
            auto& layout = base.d_layout;
            auto& draw   = base.d_draw;

            if (layout.d_thickness < 0.0) {
                layout.d_thickness = border.d_buttonBarThickness;
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_buttonBarOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root);

            if (!bar.adapterView().verify()) {
                throw Wawt::Exception("'ButtonBar' corners are inverted.",
                                bar.d_widgetId);                       // THROW
            }
            int    count  = bar.d_buttons.size();

            if (0 == count) {
                break;                                                 // BREAK
            }
            // At this point we can only slice the bar up so that each
            // button is close to the same size, and next to each other.  The
            // call to Wawt::refreshTextMetrics() will finish the job of
            // shrinking each button to the same width (height is ok) such
            // that the longest label still fits.
            auto& view        = bar.adapterView();
            auto  upperLeft   = view.d_upperLeft;
            auto  lowerRight  = view.d_lowerRight;
            auto  width       = view.interiorWidth();

            auto thickness = bar.d_buttons.front().d_layout.d_thickness;

            if (thickness < 0.0) {
                thickness = double(border.d_buttonThickness);
            }

            auto scaledThickness   = scaleBorder(*root, thickness);

            auto  overhead    = 2.*view.d_borderThickness;

            if (overhead + 2*scaledThickness+4         > view.height()
             || overhead + count*(2*scaledThickness+4) > view.width()) {
                throw Wawt::Exception("'ButtonBar' is too small.",
                                     bar.d_widgetId);                  // THROW
            }

            upperLeft.d_y    += view.d_borderThickness;
            lowerRight.d_x    = upperLeft.d_x + view.d_borderThickness - 1;
            lowerRight.d_y   -= view.d_borderThickness;

            for (auto& button : bar.d_buttons) {
                auto& buttonView             = button.d_draw;
                auto  delta                  = int(std::round(width/count));
                upperLeft.d_x                = lowerRight.d_x + 1;
                lowerRight.d_x              += delta;
                width                       -= delta;
                buttonView.d_borderThickness = scaledThickness;
                buttonView.d_upperLeft       = upperLeft;
                buttonView.d_lowerRight      = lowerRight;
                count                       -= 1;

                button.d_layout.d_thickness  = thickness;

                if (!button.d_draw.d_options.has_value()) {
                    button.d_draw.d_options = option.d_buttonOptions;
                }
                assert(button.adapterView().verify());
            }
        } break;                                                       // BREAK
        case kLIST: { // List
            auto& list     = std::get<Wawt::List>(*widget);
            auto& base     = static_cast<Wawt::Base&>(list);
            auto& layout   = base.d_layout;
            auto& draw     = base.d_draw;
            auto  usePanel = (list.d_type == ListType::eCHECKLIST
                           || list.d_type == ListType::eRADIOLIST);

            if (layout.d_thickness < 0.0) {
                layout.d_thickness
                    = double(usePanel ? border.d_panelThickness
                                      : border.d_listThickness);
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = usePanel ? option.d_panelOptions
                                          : option.d_listOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root);

            if (!list.adapterView().verify()) {
                throw Wawt::Exception("'List' corners are inverted.",
                                list.d_widgetId);                      // THROW
            }
            list.d_rowHeight = double(list.adapterView().interiorHeight())
                                                         / list.windowSize();
            list.setButtonPositions();
        } break;                                                       // BREAK
        case kPANEL: { // Panel
            auto& next   = std::get<Wawt::Panel>(*widget);
            auto& base   = static_cast<Wawt::Base&>(next);
            auto& layout = base.d_layout;
            auto& draw   = base.d_draw;

            if (layout.d_thickness < 0.0) {
                layout.d_thickness = border.d_panelThickness;
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_panelOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root);

            if (!next.adapterView().verify()) {
                throw Wawt::Exception("'Panel' corners are inverted.",
                                     next.d_widgetId);                 // THROW
            }

            for (auto& nextWidget : next.d_widgets) {
                setWidgetAdapterPositions(&nextWidget,
                                          root,
                                          next,
                                          border,
                                          option);
            }
        } break;                                                       // BREAK
        default: abort();
    }
    return;                                                           // RETURN
}

// PUBLiC CLASS METHODS
Wawt::Panel
Wawt::scrollableList(List          list,
                    bool          buttonsOnLeft,
                    unsigned int  lines)
{
    if (!list.d_root) {
        throw
            Wawt::Exception("Scrollable list does not have 'root' set.");//THROW
    }
    // Copy over only the positioning, not border or options.
    Panel container({list.d_layout.d_upperLeft, list.d_layout.d_lowerRight});
    auto  border = list.d_layout.d_thickness;

    auto      scrollUpCb   = [root=list.d_root,lines](auto btn) {
        WidgetId id(btn->d_widgetId.value()+3, true, false);
        return scroll(root, id, -lines);
    };
    auto      scrollDownCb = [root=list.d_root,lines](auto btn) {
        WidgetId id(btn->d_widgetId.value()+2, true, false);
        return scroll(root, id,  lines);
    };
    auto&     listView  = list.adapterView();
    auto&     options   = listView.d_options;
    Button    scrollUp({{},{},   Vertex::eUPPER_LEFT,  border},
                       {scrollUpCb},
                       {makeString(s_upArrow)},
                       {options});
    Button  scrollDown({{},{}, Vertex::eLOWER_RIGHT, border},
                       {scrollDownCb},
                       {makeString(s_downArrow)},
                       {options});

    auto rowHeight  = 2.0/double(list.windowSize()); // as a scale factor
    auto scale      = 1.0 - rowHeight;

    if (buttonsOnLeft) {
        scrollDown.d_layout.d_pin        = Vertex::eLOWER_LEFT;

        scrollUp.d_layout.d_upperLeft    = Position(-1.0,    -1.0);
        scrollUp.d_layout.d_lowerRight   = Position(-scale,-scale);

        scrollDown.d_layout.d_upperLeft  = Position(-1.0,   scale);
        scrollDown.d_layout.d_lowerRight = Position(-scale,   1.0);

        list.d_layout.d_upperLeft        = Position(-1.0, 1.0, 1_wr);
        list.d_layout.d_lowerRight       = Position( 1.0, 1.0);
    }
    else {
        scrollUp.d_layout.d_pin          = Vertex::eUPPER_RIGHT;

        scrollUp.d_layout.d_upperLeft    = Position( scale, -1.0  );
        scrollUp.d_layout.d_lowerRight   = Position( 1.0,   -scale);

        scrollDown.d_layout.d_upperLeft  = Position( scale,  scale);
        scrollDown.d_layout.d_lowerRight = Position( 1.0,    1.0  );

        list.d_layout.d_upperLeft        = Position(-1.0, -1.0);
        list.d_layout.d_lowerRight       = Position(-1.0,  1.0, 2_wr);
    }
    container.d_widgets.emplace_back(std::move(scrollUp));
    container.d_widgets.emplace_back(std::move(scrollDown));
    container.d_widgets.emplace_back(std::move(list));
    return container;                                                 // RETURN
}

void
Wawt::removePopUp(Panel *root)
{
    root->d_widgets.pop_back();
    root->d_widgetId
        = std::get<Canvas>(root->d_widgets.back()).d_widgetId;
    root->d_widgets.pop_back();
    return;                                                           // RETURN
}

void
Wawt::setScrollableListStartingRow(Wawt::List *list, unsigned int row)
{
    WidgetId id(list->d_widgetId.value()-2, true, false);
    auto& up    = list->d_root->lookup<Wawt::Button>(id, "Scroll up button");
    id++;
    auto& down  = list->d_root->lookup<Wawt::Button>(id, "Scroll down button");
    list->setStartingRow(row, &up, &down);
    return;                                                           // RETURN
}

// PUBLIC DATA MEMBERS
const Wawt::WidgetId   Wawt::kROOT(UINT16_MAX-1, true, true);

const std::any        Wawt::s_noOptions;

Wawt::Char_t Wawt::s_downArrow = {'v'};
Wawt::Char_t Wawt::s_upArrow   = {'^'};
Wawt::Char_t Wawt::s_cursor    = {'|'};

// PUBLIC CONSTRUCTOR
Wawt::Wawt(const TextMapper& mappingFn, DrawAdapter *adapter)
: d_adapter_p(adapter ? adapter : &s_defaultAdapter)
, d_idToString(mappingFn)
, d_fontIdToSize()
, d_borderDefaults()
{
}

// PUBLIC METHODS

void
Wawt::draw(const Panel& panel)
{
    auto ptr = d_adapter_p;

    if (panel.draw(ptr)) {
        for (auto const& widget : panel.d_widgets) {
            switch (widget.index()) {
                case kCANVAS:    std::get<Canvas>(widget).draw(ptr);    break;
                case kTEXTENTRY: std::get<TextEntry>(widget).draw(ptr); break;
                case kLABEL:     std::get<Label>(widget).draw(ptr);     break;
                case kBUTTON:    std::get<Button>(widget).draw(ptr);    break;
                case kBUTTONBAR: std::get<ButtonBar>(widget).draw(ptr); break;
                case kLIST:      std::get<List>(widget).draw(ptr);      break;
                case kPANEL:     draw(std::get<Panel>(widget));         break;
                default: abort();
            }
        }
    }
    return;                                                           // RETURN
}

Wawt::WidgetId
Wawt::popUpModalDialogBox(Panel *root, Panel&& dialogBox)
{
    auto   nextId  = root->d_widgetId;
    auto&  widgets = root->d_widgets;
    Canvas canvas({{-1.0, -1.0, 0_wr},{1.0, 1.0, 0_wr}},
                  PaintFn(),
                  {OnClickCb()}); // transparent
    canvas.d_widgetId          = nextId++;
    canvas.d_draw.d_upperLeft  = root->d_draw.d_upperLeft;
    canvas.d_draw.d_lowerRight = root->d_draw.d_lowerRight;
    widgets.emplace_back(canvas);

    auto& dialog   = widgets.emplace_back(std::move(dialogBox));
    auto  returnId = nextId;
    setIds(&dialog, nextId);
    root->d_widgetId = nextId;

    setWidgetAdapterPositions(&dialog,
                              root,
                              *root,
                              d_borderDefaults,
                              d_optionDefaults);

    auto& panel = std::get<Panel>(dialog);
    setTextAndFontValues(&panel);
    refreshTextMetrics(&panel); // since font size entries may have changed.
    return returnId;                                                  // RETURN
}

void
Wawt::refreshTextMetrics(Panel *panel)
{
    for (auto& widget : panel->d_widgets) {
        switch (widget.index()) {
            case kCANVAS: { // Canvas
            } break;                                                   // BREAK
            case kTEXTENTRY: { // TextEntry
                auto& entry = std::get<TextEntry>(widget);
                refreshTextMetric(&entry.d_draw,
                                  &entry.d_text,
                                  d_adapter_p,
                                  d_fontIdToSize,
                                  d_idToString);
            } break;                                                   // BREAK
            case kLABEL: { // Label
                auto& label = std::get<Label>(widget);
                refreshTextMetric(&label.d_draw,
                                  &label.d_text,
                                  d_adapter_p,
                                  d_fontIdToSize,
                                  d_idToString);
            } break;                                                   // BREAK
            case kBUTTON: { // Button
                auto& button = std::get<Button>(widget);
                refreshTextMetric(&button.d_draw,
                                  &button.d_text,
                                  d_adapter_p,
                                  d_fontIdToSize,
                                  d_idToString);
            } break;                                                   // BREAK
            case kBUTTONBAR: { // ButtonBar
                auto& bar = std::get<ButtonBar>(widget);

                if (bar.d_buttons.empty()) {
                    break;                                             // BREAK
                }
                auto count = int(bar.d_buttons.size());
                std::optional<int> maxWidth;

                for (auto& button : bar.d_buttons) {
                    refreshTextMetric(&button.d_draw,
                                      &button.d_text,
                                      d_adapter_p,
                                      d_fontIdToSize,
                                      d_idToString);
                    auto width = button.d_text.d_metrics.d_textWidth
                                    + 2*button.d_draw.d_borderThickness + 4;

                    if (!maxWidth.has_value() || maxWidth.value() < width) {
                        maxWidth = width;
                    }
                }
                auto barWidth = bar.d_draw.interiorWidth();
                auto spacing = count == 1
                    ? 0
                    : (barWidth - count*maxWidth.value())/(count - 1);

                if (spacing > maxWidth.value()/2) {
                    spacing = maxWidth.value()/2;
                }
                auto margin = (barWidth - count * maxWidth.value()
                                        - (count-1) * spacing)/2;
                auto startx = bar.d_draw.d_upperLeft.d_x + margin;

                for (auto& button : bar.d_buttons) {
                    auto& view  = button.d_draw;
                    view.d_upperLeft.d_x  = startx;
                    view.d_lowerRight.d_x = startx + maxWidth.value();
                    startx += maxWidth.value() + spacing;
                    refreshTextMetric(&view,
                                      &button.d_text,
                                      d_adapter_p,
                                      d_fontIdToSize,
                                      d_idToString);
                }
            } break;                                                   // BREAK
            case kLIST: { // List
                auto& list = std::get<List>(widget);
                for (auto& button : list.d_buttons) {
                    refreshTextMetric(&button.d_draw,
                                      &button.d_text,
                                      d_adapter_p,
                                      d_fontIdToSize,
                                      d_idToString);
                }
            } break;                                                   // BREAK
            case kPANEL: { // Panel
                refreshTextMetrics(&std::get<Panel>(widget));
            } break;                                                   // BREAK
            default: abort();
        }
    }
    return;                                                           // RETURN
}

void
Wawt::resizeRootPanel(Panel *root, double width, double  height)
{
    if (!root->d_widgetId.isSet()) {
        throw Exception("Root 'Panel' widget IDs not resolved.");      // THROW
    }

    if (!root->drawView().options().has_value()) {
         root->drawView().options() = d_optionDefaults.d_screenOptions;
    }

    root->d_draw.d_upperLeft                = {0, 0};
    root->d_draw.d_lowerRight.d_x           = width  - 1;
    root->d_draw.d_lowerRight.d_y           = height - 1;
    root->d_draw.d_borderThickness          = 0.0;
    root->d_layout.d_lowerRight.d_sX        = width;
    root->d_layout.d_lowerRight.d_sY        = height;
    root->d_layout.d_thickness              = 0.0;

    for (auto& widget : root->d_widgets) {
        setWidgetAdapterPositions(&widget,
                                  root,
                                  *root,
                                  d_borderDefaults,
                                  d_optionDefaults);
    }
    d_fontIdToSize.clear();
    setTextAndFontValues(root);
    refreshTextMetrics(root);
    return;                                                           // RETURN
}

void
Wawt::resolveWidgetIds(Panel *root)
{
    WidgetId nextId = 1_w;

    for (auto& widget : root->d_widgets) {
        setIds(&widget, nextId);
    }
    root->d_widgetId        = nextId;
    root->d_draw.d_tracking = std::make_tuple(kPANEL, nextId.value(), -1);
    return;                                                           // RETURN
}

void
Wawt::setFontSizeEntry(Base *base)
{
    auto& id = base->textView().fontSizeGrp();

    if (id.has_value()) {
        FontIdMap::iterator it = d_fontIdToSize.insert({id, 0}).first;
        base->textView().initTextMetricValues(&base->d_draw,
                                              d_adapter_p,
                                              it->second);
        auto charSize = base->d_draw.d_charSize;

        if (it->second == 0 || it->second > charSize) {
            it->second = charSize;
        }
    }
    return;                                                           // RETURN
}

void
Wawt::setTextAndFontValues(Panel *root)
{
    for (auto& widget : root->d_widgets) {
        switch (widget.index()) {
            case kCANVAS: {
                } break;
            case kTEXTENTRY: {
                    auto& entry = std::get<TextEntry>(widget);
                    setFontSizeEntry(&entry);
                } break;
            case kLABEL: {
                    auto& label = std::get<Label>(widget);
                    label.textView().setText(d_idToString);
                    setFontSizeEntry(&label);
                } break;
            case kBUTTON: {
                    auto& btn = std::get<Button>(widget);
                    btn.textView().setText(d_idToString);
                    setFontSizeEntry(&btn);
                } break;
            case kBUTTONBAR: {
                    for (auto& btn : std::get<ButtonBar>(widget).d_buttons) {
                        btn.textView().setText(d_idToString);
                        setFontSizeEntry(&btn);
                    }
                } break;
            case kLIST: {
                    auto& list = std::get<List>(widget);

                    for (auto& btn : list.d_buttons) {
                        btn.textView().setText(d_idToString);
                        setFontSizeEntry(&btn);
                    }
                } break;
            case kPANEL: {
                    setTextAndFontValues(&std::get<Panel>(widget));
                } break;
            default: abort();
        }
    }
    return;                                                           // RETURN
}

                                //---------------
                                // class  WawtDump
                                //---------------

void
WawtDump::draw(const Wawt::DrawDirective&  widget,
               const Wawt::String_t&       text)
{
    auto [type, id, row] = widget.d_tracking;
    d_dumpOs << std::boolalpha << d_indent << "<widget id='" << type
             << "," << id;

    if (row >= 0) {
        d_dumpOs  << "," << row;
    }
    d_dumpOs  << "' borderThickness='"   << widget.d_borderThickness
              << "' greyEffect='"        << widget.d_greyEffect
              << "' options='"           << widget.d_options.has_value()
              << "'>\n";
    d_indent += 2;
    d_dumpOs  << d_indent
              << "<upperLeft x='"        << widget.d_upperLeft.d_x
              << "' y='"                 << widget.d_upperLeft.d_y
              << "'/>\n"
              << d_indent
              << "<lowerRight x='"       << widget.d_lowerRight.d_x
              << "' y='"                 << widget.d_lowerRight.d_y
              << "'/>\n";

    if (widget.d_charSize > 0) {
        d_dumpOs  << d_indent
                  << "<text startx='"    << Int(widget.d_startx)
                  << "' selected='"      << widget.d_selected;

        if (widget.d_bulletType      == Wawt::BulletType::eCHECK) {
            d_dumpOs  << "' bulletType='Check";
        }
        else if (widget.d_bulletType == Wawt::BulletType::eRADIO) {
            d_dumpOs  << "' bulletType='Radio";
        }
        d_dumpOs  << "'>\n";
        d_indent += 2;
        d_dumpOs  << d_indent
                  << "<string charSize='" << widget.d_charSize
                  << "'>";
        outputXMLString(d_dumpOs, text);
        d_dumpOs  << "</string>\n";
        d_indent -= 2;
        d_dumpOs  << d_indent
                  << "</text>\n";
    }
    d_indent -= 2;
    d_dumpOs  << d_indent << "</widget>\n" << std::noboolalpha;
}

void
WawtDump::getTextMetrics(Wawt::DrawDirective    *parameters,
                         Wawt::TextMetrics      *metrics,
                         const Wawt::String_t&   text,
                         double                  upperLimit)
{
    assert(metrics->d_textHeight >= upperLimit);    // these are upper limits
    assert(metrics->d_textWidth > 0);               // bullet size excluded
    
    if (upperLimit > 0) {
        auto  charCount = text.length();
        auto  charSize  = charCount > 0 ? metrics->d_textWidth/charCount
                                        : metrics->d_textHeight-1;

        parameters->d_charSize = charSize >= upperLimit ? upperLimit - 1
                                                        : charSize;
    }
    return;                                                           // RETURN
}

}  // namespace BDS
// vim: ts=4:sw=4:et:ai
