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

namespace Wawt {

namespace {

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
    return isEos(ch) ? std::string() : std::string(ch, Wawt::sizeOfChar(ch));
}

inline auto makeString(char ch) {
    if constexpr(std::is_same_v<Wawt::Char_t,wchar_t>) {
        return makeString(wchar_t(ch));
    }
    else {
        return makeString(&ch);
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
        p += Wawt::sizeOfChar(p);
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

void outputXMLString(std::ostream& os, const std::wstring& text) {
    // NOTE: std::codecvt is deprecated. So, to avoid compiler warnings,
    // and until this is sorted out, lets roll our own.
    // For anticipated use (i.e. test drivers), this is expected to be
    // good enough.
    for (auot c : text) {
        unsigned char ch = static_cast<char>(c);

        if (static_cast<decltype(c)>(ch) == c) {
            if (ch == L'"') {
                os << "&quot;";
            }
            else if (ch == L'\'') {
                os << "&apos;";
            }
            else if (ch == L'<') {
                os << "&lt;";
            }
            else if (ch == L'>') {
                os << "&gt;";
            }
            else if (ch == L'&') {
                os << "&amp;";
            }
            else {
                os(put(ch));
            }
            else if ((ch & 0200) == 0) {
                os.put(ch);
            } // end of 7 bit ASCII
            else {
                os << "&#" << unsigned(ch) << ';';
            }
        }
        else  {
            os << "&#" << unsigned(c) << ';';
        }
    }
}

bool findWidget(const Widget              **widget,
                const Widget&               parent,
                WidgetId                    id) {
    for (auto& next : parent.children()) {
        if (next.widgetId() == id) {
            *widget = &next;
            return true;                                              // RETURN
        }

        if (!next.children().empty()) {
            if (findWidget(widget, next, id)) {
                return true;                                          // RETURN
            }

            if (!next.widgetId().isSet() || id < next.d_widgetId) {
                return false;                                         // RETURN
            }
        }
        else if (next.widgetId() > id) {
            return false;                                             // RETURN
        }
    }
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
                       Wawt::DrawProtocol            *adapter_p,
                       FontIdMap&                     fontIdToSize,
                       const Wawt::TextMapper&        textLookup) {
    auto  id         = block->fontSizeGrp();
    auto  textHeight = args->interiorHeight();
    auto  fontSize   = id ? fontIdToSize.find(*id)->second : 0;
    auto  charSize   = fontSize > 0   ? fontSize : textHeight;

    if (charSize != args->d_charSize || block->needRefresh()) {
        block->setText(textLookup);
        block->initTextMetricValues(args, adapter_p, charSize);

        if (fontSize == 0 && id) {
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


                            //-------------------------
                            // class  Layout::WidgetRef
                            //-------------------------


const Widget *
Layout::WidgetRef::getWidgetPointer(const Panel&      parent,
                                    const Panel&      root) const
{
    if (d_widget != nullptr) {
        assert(*d_widget);
        return *d_widget;                                             // RETURN
    }
    const Widget *widget = nullptr;

    if (d_widgetId.isSet()) {
        if (d_widgetId.isRelative()) {
            if (d_widgetId == kPARENT) {
                widget = &parent;
            }
            else if (d_widgetId() == kROOT) {
                widget = &root;
            }
            else {
                auto offset = d_widgetId.value();

                if (offset < parent.children().size()) {
                    widget = &parent.children()[offset];
                }
            }
        }
        else if (root.widgetId() == d_widgetId) {
            widget = &root;
        }
        else {
            findWidget(&widget, root, d_widgetId);
        }
    }

    if (!widget) {
        throw WawtException("Context of widget not found.", d_widgetId);
    }
    return widget;                                                    // RETURN
}

WidgetId
WidgetRef::getWidgetId() const noexcept
{
    WidgetId ret = d_widgetId;

    if (!ret.isSet()) { // if widget ID is not available
        if (d_widget != nullptr) { // but a forwarding pointer is
            ret = (*d_widget)->widgetId(); // fetch the value and keep it
        }
    }
    return ret;                                                       // RETURN
}

                                //-------------
                                // class Widget
                                //-------------

// PUBLIC MEMBERS
WidgetId
Widget::assignWidgetIds(WidgetId next, Widget *root) noexcept
{
    for (auto& widget : d_widgets) {
        next = widget.assignWidgetIds(next, root);
    }
    d_drawData.d_widgetId   = next++;
    d_root                  = root;

    if (*d_widgetLabel) {
        *d_widgetLabel = this;
    }
    return next;
}

const Widget*
Widget::lookup(Wawt::WidgetId widgetId) const noexcept
{
    Widget *widget = nullptr;

    if (widgetId.isSet()) {
        if (widgetId.isRelative()) {
            if (widgetId.value() < children().size()) {
                widget = &children()[widgetId.value()];
            }
        }
        else if (widgetId == widgetId()) {
            widget = this;
        }
        else {
            findWidget(&widget, *this, widgetId);
        }
    }
    return widget;                                                    // RETURN
}

// PRIVATE MEMBERS
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

    if (d_text.d_block.d_fontSizeGrp) {
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
