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

#include "wawt/wawtenv.h"
#include "wawt/widget.h"
#include "wawt/drawprotocol.h"

#include <cassert>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <ios>
#include <iostream> // For Draw() default constructor
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#ifdef WAWT_WIDECHAR
#define S(str) (L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) (u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace Wawt {

namespace {

const Widget::Children s_emptyList;
const Text             s_emptyText;

struct Indent {
    unsigned int d_indent;

    Indent& operator+=(unsigned int change) {
        d_indent += change;
        return *this;
    }

    Indent& operator-=(unsigned int change) {
        d_indent -= change;
        return *this;
    }

    Indent(unsigned int indent = 0) : d_indent(indent) { }
};

std::ostream& operator<<(std::ostream& os, Indent indent) {
    if (indent.d_indent > 0) {
        os << std::setw(indent.d_indent) << ' ';
    }
    return os;                                                        // RETURN
}

std::ostream& operator<<(std::ostream& os, const WidgetId id) {
    if (id == WidgetId::kPARENT) {
        os << "parent";
    }
    else if (id == WidgetId::kROOT) {
        os << "root";
    }
    else if (id.isSet()) {
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

void appendChar(String_t *str, Char_t ch) {
    if (ch != '\0') {
        if constexpr(std::is_same_v<Char_t, wchar_t>) {
            str->push_back(ch);
        }
        else {
            auto c   = static_cast<uint32_t>(ch);
            auto lng = (c & 0x1FF800) ? ((c & 0x1F0000) ? 4 : 3)
                                      : ((c & 03600)    ? 2 : 1);
            switch (lng) {
              case 4: str->push_back(char(0360|((c >> 18)&007)));
                      str->push_back(char(0200|((c >> 12)&077)));
                      str->push_back(char(0200|((c >>  6)&077)));
                      str->push_back(char(0200|( c       &077)));
                      break;
              case 3: str->push_back(char(0340|((c >> 12)&017)));
                      str->push_back(char(0200|((c >>  6)&077)));
                      str->push_back(char(0200|( c       &077)));
                      break;
              case 2: str->push_back(char(0300|((c >>  6)&037)));
                      str->push_back(char(0200|( c       &077)));
                      break;
              default:str->push_back(char(       c    &  0177));
                      break;
            };
        }
    }
    return;                                                           // RETURN
}

// Only one of the following is used, suppress complaints about that with 
// "inline".

inline
wchar_t backChar(const std::wstring_view& s) {
    return s.empty() ? '\0' : s.back();                               // RETURN
}

inline
char32_t backChar(const std::string_view& text) {
    if (text.empty()) {
        return U'\0';                                                 // RETURN
    }
    auto it     = text.rbegin();
    auto ch     = *it++;
    auto value  = int{};
    auto shift  = 0;

    while ((ch & 0300) == 0200) {
        if (shift >= 18 || it == text.rend()) {
            return U'\0';
        }
        value |= int(ch & 077) << shift;
        shift += 6;
        ch     = *it++;
    }

    if ((ch & 0200) == 0) { // 7 bit ASCII
        if (shift !=  0) return U'\0';                                // RETURN
        ch &= 0177;
    }
    else if ((ch & 0340) == 0300) {
        if (shift !=  6) return U'\0';                                // RETURN
        ch &= 037;
    }
    else if ((ch & 0360) == 0340) {
        if (shift != 12) return U'\0';                                // RETURN
        ch &= 017;
    }
    else if ((ch & 0370) == 0360) {
        if (shift != 18) return U'\0';                                // RETURN
        ch &= 07;
    }
    else {
        return U'\0';                                                 // RETURN
    }
    return char32_t((int(ch) << shift) | value);                      // RETURN
}

bool clearCharSizeMap(Widget *widget) {
    if (widget->hasText()) {
        widget->text().d_layout.d_charSizeMap->clear();
        return true;                                                  // RETURN
    }

    if (widget->hasChildren()) {
        for (auto& child : widget->children()) {
            if (clearCharSizeMap(&child)) {
                return true;                                          // RETURN
            }
        }
    }
    return false;                                                     // RETURN
}

// Only one of the following is used, suppress complaints about that with 
// "inline".

inline
wchar_t frontChar(const std::wstring_view& s) {
    return s.empty() ? '\0' : s.front();                              // RETURN
}

inline
char32_t frontChar(const std::string_view& text) {
    if (text.empty()) {
        return U'\0';                                                 // RETURN
    }
    auto it     = text.begin();
    auto ch     = *it++;
    auto value  = int{};

    if ((ch & 0200) == 0) { // 7 bit ASCII
        return ch;                                                    // RETURN
    }

    if ((ch & 0340) == 0300) {
        value = int(ch & 037); // 1 + 1 bytes
    }
    else if ((ch & 0360) == 0340) {
        value = int(ch & 017) << 6;

        if (it != text.end()) {
            value |= int(*it++ & 077); // 2 + 1 bytes
        }
    }
    else if ((ch & 0370) == 0360) {
        value = int(ch & 07) << 6;

        if (it != text.end()) {
            value |= int(*it++ & 077);

            if (it != text.end()) {
                value <<= 6;
                value  |= int(*it++ & 077); // 3 + 1 bytes
            }
        }
    }
    return it == text.end() ? U'\0'
                            : char32_t((value << 6) | int(*it & 077));// RETURN
}

Widget::DownEventMethod eatDownEvents() {
    return
        [] (auto, auto, auto, auto) -> EventUpCb {
            return  [](double, double, bool) -> void {
                return;
            };
        };                                                            // RETURN
}

bool findWidget(const Widget              **widget,
                const Widget&               parent,
                uint16_t                    id) {
    for (auto& next : parent.children()) {
        if (next.widgetIdValue() == id) {
            *widget = &next;
            return true;                                              // RETURN
        }

        if (!next.children().empty()) {
            if (findWidget(widget, next, id)) {
                return true;                                          // RETURN
            }

            if (!next.widgetIdValue() || id < next.widgetIdValue()) {
                return false;                                         // RETURN
            }
        }
        else if (next.widgetIdValue() > id) {
            return false;                                             // RETURN
        }
    }
    return false;                                                     // RETURN
}

} // end unnamed namespace

std::atomic_flag    WawtEnv::_atomicFlag   = ATOMIC_FLAG_INIT;
WawtEnv            *WawtEnv::_instance     = nullptr;
std::any            WawtEnv::_any;
WawtEnv::Translator WawtEnv::_translator;

// The following defaults should be found in all fonts.
Char_t   WawtEnv::kFocusChg   = C('\0');

char     WawtEnv::sButton[]   = "button";
char     WawtEnv::sCanvas[]   = "canvas";
char     WawtEnv::sDialog[]   = "dialog";
char     WawtEnv::sEntry[]    = "entry";
char     WawtEnv::sItem[]     = "item";
char     WawtEnv::sLabel[]    = "label";
char     WawtEnv::sList[]     = "list";
char     WawtEnv::sPanel[]    = "panel";
char     WawtEnv::sScreen[]   = "screen";
char     WawtEnv::sScrollbox[]= "scrollbox";

LayoutGenerator gridLayoutGenerator(double       borderThickness,
                                    std::size_t  widgetCount,
                                    std::size_t  columns,
                                    std::size_t *rowsOut)              noexcept
{
    if (widgetCount == 0) {
        return std::function<Layout()>();                             // RETURN
    }

    if (columns == 0) {
        columns = widgetCount;
    }
    auto rows = widgetCount/columns;

    if (widgetCount % columns > 0) {
        rows += 1;
    }

    if (rowsOut) {
        *rowsOut = rows;
    }

    auto yinc = 2.0/double(rows);
    auto xinc = 2.0/double(columns);
    return
        [columns, xinc, yinc, borderThickness, c = 0u, r = 0u]() mutable
        {
            auto layout = Layout({-1.0+  c  *xinc, -1.0+  r  *yinc},
                                 {-1.0+(c+1)*xinc, -1.0+(r+1)*yinc},
                                 borderThickness);
            if (++c == columns) {
                c  = 0;
                r += 1;
            }
            return layout;
        };                                                            // RETURN
}

std::ostream& outputXMLescapedChar(std::ostream& os, Char_t ch)
{
    if (ch == C('"')) {
        os << "&quot;";
    }
    else if (ch == C('\'')) {
        os << "&apos;";
    }
    else if (ch == C('<')) {
        os << "&lt;";
    }
    else if (ch == C('>')) {
        os << "&gt;";
    }
    else if (ch == C('&')) {
        os << "&amp;";
    }
    else if (static_cast<int>(ch) > 0x1f && static_cast<int>(ch) < 0x7f) {
        os.put(ch);
    }
    else {
        os << "&#" << static_cast<int>(ch) << ';';
    }
    return os;                                                        // RETURN
}

std::ostream& outputXMLescapedString(std::ostream& os, StringView_t text)
{
    // NOTE: std::codecvt is deprecated. So, to avoid compiler warnings,
    // and until this is sorted out, lets roll our own. Avoid multibyte
    // routines which are dependent on locale too.
    // For anticipated use (i.e. test drivers), this is expected to be
    // good enough.

    while (!text.empty()) {
        auto ch = popFrontChar(text);

        if (ch == '\0') { // Invalid  string
            break;
        }
        outputXMLescapedChar(os, ch);
    }
    return os;                                                        // RETURN
}

Char_t popBackChar(StringView_t& view)
{
    auto back = backChar(view);
    if (back != '\0') {
        if constexpr(std::is_same_v<Char_t, wchar_t>) {
            view.remove_suffix(1);
        }
        else {
            view.remove_suffix(sizeOfChar(back));
        }
    }
    return back;                                                      // RETURN
}

Char_t popFrontChar(StringView_t& view)
{
    auto front = frontChar(view);
    if (front != '\0') {
        if constexpr(std::is_same_v<Char_t, wchar_t>) {
            view.remove_prefix(1);
        }
        else {
            view.remove_prefix(sizeOfChar(front));
        }
    }
    return front;                                                     // RETURN
}

String_t toString(Char_t *charArray, std::size_t length)
{
    auto result = String_t{};
    result.reserve(length);

    for (auto i = 0u; i < length; ++i) {
        auto ch = charArray[i];

        if (ch == '\0') {
            break;                                                     // BREAK
        }
        appendChar(&result, ch);
    }
    return result;                                                    // RETURN
}

                                //--------
                                // Tracker
                                //--------

Tracker::Tracker(Tracker&& move)
{
    d_widget        = move.d_widget;
    d_label         = move.d_label;
    move.d_widget   = nullptr;
    move.d_label    = nullptr;

    if (d_label) {
        d_label->d_backPtr = this;
    }
}

Tracker::~Tracker()
{
    if (d_label) {
        d_label->d_backPtr  = nullptr;
    }
    d_widget    = nullptr;
    d_label     = nullptr;
}

Tracker&
Tracker::operator=(Tracker&& rhs)
{
    if (this != &rhs) {
        d_widget        = rhs.d_widget;
        d_label         = rhs.d_label;
        rhs.d_widget    = nullptr;
        rhs.d_label     = nullptr;

        if (d_label) {
            d_label->d_backPtr = this;
        }
    }
    return *this;
}

Tracker::operator Trackee()
{
    if (d_label) {
        throw WawtException("Tracker already assigned a trackee.");
    }
    return Trackee(this);
}

                            //-----------------
                            // class  WidgetRef
                            //-----------------

const Widget *
WidgetRef::getWidgetPointer(const Widget& parent) const
{
    if (d_tracker != nullptr && *d_tracker) {
        return &**d_tracker;                                          // RETURN
    }
    const Widget *widget = nullptr;

    if (d_widgetId.isSet()) {
        if (d_widgetId.isRelative()) {
            if (d_widgetId == WidgetId::kPARENT) {
                widget = &parent;
            }
            else if (d_widgetId == WidgetId::kROOT) {
                widget = parent.screen();
            }
            else {
                auto offset = d_widgetId.value();

                if (offset < parent.children().size()) {
                    widget = &parent.children()[offset];
                }
            }
        }
        else if (parent.screen()->widgetIdValue() == d_widgetId.value()) {
            widget = parent.screen();
        }
        else {
            findWidget(&widget, *parent.screen(), d_widgetId.value());
        }
    }

    if (!widget) {
        throw WawtException("Widget ID is invalid.", d_widgetId);
    }
    return widget;                                                    // RETURN
}

WidgetId
WidgetRef::getWidgetId() const noexcept
{
    WidgetId ret = d_widgetId;

    if (!ret.isSet()) { // if widget ID is not available
        if (d_tracker && *d_tracker) { // but a forwarding pointer is
            ret = WidgetId((*d_tracker)->widgetIdValue(), false); 
        }
    }
    return ret;                                                       // RETURN
}


                            //------------------------
                            // class  Layout::Position
                            //------------------------

Coordinates
Layout::Position::resolvePosition(const Widget& parent) const
{
    auto       reference  = d_widgetRef.getWidgetPointer(parent);
    auto&      rectangle  = reference->layoutData();
    auto&      ul_x       = rectangle.d_upperLeft.d_x;
    auto&      ul_y       = rectangle.d_upperLeft.d_y;
    auto       lr_x       = ul_x + rectangle.d_bounds.d_width;
    auto       lr_y       = ul_y + rectangle.d_bounds.d_height;

    auto xorigin = (ul_x  + lr_x)/2.0;
    auto yorigin = (ul_y  + lr_y)/2.0;
    auto xradius = lr_x - xorigin;
    auto yradius = lr_y - yorigin;

    if (reference == &parent || reference == parent.screen()) {
        auto       thickness  = rectangle.d_border;
        xradius -= thickness;
        yradius -= thickness;
    }

    auto x = xorigin + d_sX*xradius;
    auto y = yorigin + d_sY*yradius;

    return Coordinates{float(x), float(y)};                           // RETURN
}

                            //--------------
                            // class  Layout
                            //--------------

Layout::Result
Layout::resolveLayout(const Widget& parent) const
{
    auto [ux, uy]   = d_upperLeft.resolvePosition(parent);
    auto [lx, ly]   = d_lowerRight.resolvePosition(parent);
    auto width      = lx - ux;
    auto height     = ly - uy;

    if (d_pin != Vertex::eNONE) {
        auto  square = (width + height)/2.0;

        if (square > width) {
            square = width;
        }
        else if (square > height) {
            square = height;
        }
        auto  deltaW = square - width;
        auto  deltaH = square - height;

        switch (d_pin) {
          case Vertex::eUPPER_LEFT: {
                ly += deltaH; lx += deltaW;
            } break;
          case Vertex::eUPPER_CENTER: {
                ly += deltaH; lx += deltaW/2.0;
                              ux -= deltaW/2.0;
            } break;
          case Vertex::eUPPER_RIGHT: {
                ly += deltaH;
                              ux -= deltaW;
            } break;
          case Vertex::eCENTER_LEFT: {
                uy -= deltaH/2.0;
                ly += deltaH/2.0; lx += deltaW;
            } break;
          case Vertex::eCENTER_CENTER: {
                uy -= deltaH/2.0; ux -= deltaW/2.0;
                ly += deltaH/2.0; lx += deltaW/2.0;
            } break;
          case Vertex::eCENTER_RIGHT: {
                uy -= deltaH/2.0; ux -= deltaW;
                ly += deltaH/2.0;
            } break;
          case Vertex::eLOWER_LEFT: {
                uy -= deltaH;
                              lx += deltaW;
            } break;
          case Vertex::eLOWER_CENTER: {
                uy -= deltaH; ux -= deltaW/2.0;
                              lx += deltaW/2.0;
            } break;
          case Vertex::eLOWER_RIGHT: {
                uy -= deltaH; ux -= deltaW;
            } break;
          default: break;
        }
    }
    auto min    = std::min(parent.screen()->layoutData().d_bounds.d_width,
                           parent.screen()->layoutData().d_bounds.d_height);
    auto border = float(min * d_thickness / 1000.0);

    return { ux, uy, lx - ux, ly - uy, border };                      // RETURN
}

Layout&
Layout::scale(double sx, double sy)
{
    if (d_upperLeft.d_widgetRef != d_lowerRight.d_widgetRef) {
        // No common coordinate system, so:
        throw WawtException("'scale' requires compatible widget references.");
    }
    auto dx  = d_lowerRight.d_sX - d_upperLeft.d_sX;
    auto dy  = d_lowerRight.d_sY - d_upperLeft.d_sY;
    auto sdx = sx * (d_lowerRight.d_sX - d_upperLeft.d_sX);
    auto sdy = sy * (d_lowerRight.d_sY - d_upperLeft.d_sY);
    d_upperLeft.d_sX  -= (sdx - dx)/2.0;
    d_upperLeft.d_sY  -= (sdy - dy)/2.0;
    d_lowerRight.d_sX += (sdx - dx)/2.0;
    d_lowerRight.d_sY += (sdy - dy)/2.0;
    return *this;
}

                            //------------------
                            // class  Text::Data
                            //------------------

bool
Text::Data::resolveSizes(const Wawt::Layout::Result&  container,
                         uint16_t                     upperLimit,
                         DrawProtocol                *adapter,
                         const std::any&              options) noexcept
{
    auto constraint       = container.d_bounds;
    auto borderAdjustment = 2*(container.d_border + 1);
    constraint.d_width   -= borderAdjustment;
    constraint.d_height  -= borderAdjustment;
    return adapter->getTextValues(*this, constraint, upperLimit, options);
}

                            //--------------------
                            // class  Text::Layout
                            //--------------------

// PUBLIC METHODS
Coordinates
Text::Layout::position(const Bounds&                bounds,
                       const Wawt::Layout::Result&  container) noexcept
{
    Coordinates position;
    auto borderAdjustment = 2*(container.d_border + 1);
    position.d_x = container.d_upperLeft.d_x + container.d_border + 1;
    position.d_y = container.d_upperLeft.d_y + container.d_border + 1;

    if (horizontalAlign() != TextAlign::eLEFT) {
        auto space = container.d_bounds.d_width
                   - bounds.d_width
                   - borderAdjustment;

        if (horizontalAlign() == TextAlign::eCENTER) {
            space /= 2.0f;
        }
        position.d_x += space;
    }

    if (verticalAlign() != TextAlign::eTOP
     && verticalAlign() != TextAlign::eBASELINE) {
        auto space = container.d_bounds.d_height
                   - bounds.d_height
                   - borderAdjustment;

        if (verticalAlign() == TextAlign::eCENTER) {
            space /= 2.0f;
        }
        position.d_y += space;
    }
    return position;                                                  // RETURN
}

uint16_t
Text::Layout::upperLimit(const Wawt::Layout::Result& container) noexcept
{
    auto charSizeLimit    = container.d_bounds.d_height;
    auto borderAdjustment = 2*(container.d_border + 1);

    if (charSizeLimit < borderAdjustment) {
        return 0;                                                     // RETURN
    }
    // The 'd_charSize' value is the distance between the upper left y value
    // and the baseline of the text string.
    charSizeLimit -= borderAdjustment;
    charSizeLimit  = (5*charSizeLimit)/6;

    if (d_charSizeGroup) {
        if (auto it  = d_charSizeMap->find(*d_charSizeGroup);
                 it != d_charSizeMap->end()) {
            if (charSizeLimit > it->second + 1) {
                charSizeLimit = it->second + 1;
            }
        }
    }
    return charSizeLimit;                                             // RETURN
}

                                //------------
                                // class  Text
                                //------------

bool
Text::resolveLayout(const Wawt::Layout::Result&  container,
                    DrawProtocol                *adapter,
                    const std::any&              options) noexcept
{
    auto charSizeLimit    = d_layout.upperLimit(container);

    if (!d_data.resolveSizes(container, charSizeLimit, adapter, options)) {
        return false;                                             // RETURN
    }

    if (d_layout.d_charSizeGroup) {
        (*d_layout.d_charSizeMap)[*d_layout.d_charSizeGroup]
            = d_data.d_charSize;
    }
    // Although the size may not have changed, the position might have:
    d_data.d_baselineAlign = d_layout.verticalAlign() == TextAlign::eBASELINE;
    d_data.d_upperLeft     = d_layout.position(d_data.d_bounds, container);
    return true;                                                      // RETURN
}


                                //-------------
                                // class  Widget
                                //-------------

// PRIVATE MEMBERS
void
Widget::layout(DrawProtocol       *adapter,
               bool                firstPass,
               const Widget&       parent)
{
    d_settings.d_successfulLayout = true;

    call(&defaultLayout,
         &Methods::d_layoutMethod,
         this,
         parent,
         firstPass,
         adapter);

    if (hasText()) {
        text().d_layout.d_refreshBounds = false;
    }

    auto space = std::min(d_rectangle.d_bounds.d_height,
                          d_rectangle.d_bounds.d_width)-2*d_rectangle.d_border;

    if (space > 8 && hasChildren()) {
        for (auto& child : children()) {
            child.layout(adapter, firstPass, *this);
        }
    }
}

// PUBLIC CLASS MEMBERS
void
Widget::defaultDraw(Widget *widget, DrawProtocol *adapter)
{
    adapter->draw(widget->layoutData(), widget->settings());

    if (widget->hasText() && widget->settings().d_successfulLayout) {
        adapter->draw(widget->text().d_data, widget->settings());
    }
}

void
Widget::defaultLayout(Widget                  *widget,
                      const Widget&            parent,
                      bool                     firstPass,
                      DrawProtocol            *adapter) noexcept
{
    if (firstPass) {
        widget->layoutData() = widget->layout().resolveLayout(parent);
    }

    if (widget->hasText()
     && (firstPass || widget->text().d_layout.d_charSizeGroup)) {
        widget->d_settings.d_successfulLayout =
            widget->text().resolveLayout(widget->layoutData(),
                                         adapter,
                                         widget->options());
    }
    return;                                                           // RETURN
}

void
Widget::defaultSerialize(std::ostream&      os,
                         std::string       *closeTag,
                         const Widget&      widget,
                         unsigned int       indent)
{
    auto  spaces     = Indent(indent);
    auto& layout     = widget.layout();
    auto& settings   = widget.settings();
    auto& widgetName = settings.d_optionName;
    auto& widgetId   = settings.d_widgetIdValue;

    auto fmtflags = os.flags();
    os.setf(std::ios::boolalpha);

    std::ostringstream ss;
    ss << spaces << "</" << widgetName << ">\n";
    os << spaces << "<" << widgetName << " id='" << widgetId
       << "' rid='" << widget.settings().d_relativeId << "'>\n";

    *closeTag = ss.str();

    spaces += 2;
    os << spaces << "<layout border='";

    if (layout.d_thickness >= 0.0) {
        os << layout.d_thickness;
    }

    if (layout.d_pin != Layout::Vertex::eNONE) {
        os << "' pin='" << int(layout.d_pin);
    }
    os << "'>\n";
    spaces += 2;
    os << spaces
       <<     "<ul sx='" << layout.d_upperLeft.d_sX
       <<       "' sy='" << layout.d_upperLeft.d_sY
       <<   "' widget='" << layout.d_upperLeft.d_widgetRef.getWidgetId()
       << "'/>\n";
    os << spaces
       <<     "<lr sx='" << layout.d_lowerRight.d_sX
       <<       "' sy='" << layout.d_lowerRight.d_sY
       <<   "' widget='" << layout.d_lowerRight.d_widgetRef.getWidgetId()
       << "'/>\n";
    spaces -= 2;
    os << spaces << "</layout>\n";

    if (widget.hasText()) {
        auto& textData   = widget.text().d_data;
        auto& textLayout = widget.text().d_layout;
        os << spaces
           << "<text horizontalAlign='"   << int(textLayout.d_horizontalAlign)
           << "' verticalAlign='"   << int(textLayout.d_verticalAlign)
           << "' group='";

        if (textLayout.d_charSizeGroup) {
            os << *textLayout.d_charSizeGroup;
        }

        if (textData.d_labelMark != Text::BulletMark::eNONE) {
            os << "' mark='"    << int(textData.d_labelMark)
               << "' left='"    << bool(textData.d_leftAlignMark);
        }
        os << "'>";
        outputXMLescapedString(os, textData.view());
        os << "</text>\n";
    }
    else {
        os << spaces << "<text/>\n";
    }
    std::stringstream im;
    spaces += 2;

    if (widget.downEventMethod()) {
        im << spaces << "<downMethod type='functor'/>\n";
    }
    
    if (widget.drawMethod()) {
        im << spaces << "<drawMethod type='functor'/>\n";
    }

    if (widget.layoutMethod()) {
        im << spaces << "<layoutMethod type='functor'/>\n";
    }
    
    if (widget.newChildMethod()) {
        im << spaces << "<newChildMethod type='functor'/>\n";
    }
    
    if (widget.serializeMethod()) {
        im << spaces << "<serializeMethod type='functor'/>\n";
    }
    spaces -= 2;

    if (im.str().empty()) {
        os << spaces << "<installedMethods/>\n";
    }
    else {
        os << spaces << "<installedMethods>\n" << im.str()
           << spaces << "</installedMethods>\n";
    }
    os.flags(fmtflags);
    return;                                                           // RETURN
}

// PUBLIC Reference Qualified MANIPULATORS

Widget
Widget::addChild(Widget&& child) &&
{
    children().push_back(std::move(child));

    if (d_methods && d_methods->d_newChildMethod) {
        d_methods->d_newChildMethod(this, &children().back());
    }
    return std::move(*this);                                          // RETURN
}

Widget
Widget::downEventMethod(DownEventMethod&& newMethod) && noexcept
{
    d_downMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::drawMethod(DrawMethod&& newMethod) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_drawMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::inputMethod(InputMethod&& newMethod) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_inputMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::layoutMethod(LayoutMethod&& newMethod) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_layoutMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::newChildMethod(NewChildMethod&& newMethod) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_newChildMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::serializeMethod(SerializeMethod&& newMethod) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_serializeMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget&
Widget::downEventMethod(DownEventMethod&& newMethod) & noexcept
{
    d_downMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::drawMethod(DrawMethod&& newMethod) & noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_drawMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::inputMethod(InputMethod&& newMethod) & noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_inputMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::layoutMethod(LayoutMethod&& newMethod) & noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_layoutMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::newChildMethod(NewChildMethod&& newMethod) & noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_newChildMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::serializeMethod(SerializeMethod&& newMethod) & noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_serializeMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::textMark(Text::BulletMark  mark,
                 bool              leftAlignMark) & noexcept
{
    text().d_data.d_labelMark     = mark;
    text().d_data.d_leftAlignMark = leftAlignMark;
    d_settings.d_hideSelect       = mark != Text::BulletMark::eNONE;
    return *this;
}

// PUBLIC MEMBERS
Widget::Widget(Widget&& move) noexcept
{
    d_widgetLabel   = std::move(move.d_widgetLabel);
    d_root          = move.d_root;
    d_downMethod    = std::move(move.d_downMethod);
    d_methods       = std::move(move.d_methods);
    d_layout        = std::move(move.d_layout);
    d_rectangle     = std::move(move.d_rectangle);
    d_settings      = std::move(move.d_settings);
    d_text          = std::move(move.d_text);
    d_children      = std::move(move.d_children);

    if (d_widgetLabel) {
        d_widgetLabel.update(this);
    }

    if (d_root == &move) { // move after 'assignWidgets' called: redo
        d_root = this;
        assert(d_text);

        if (hasChildren()) {
            auto next       = uint16_t{1};
            auto relativeId = uint16_t{0};

            for (auto& widget : children()) {
                next = widget.assignWidgetIds(next,
                                              relativeId++,
                                              d_text->d_layout.d_charSizeMap,
                                              this);
            }
        }
    }
    move.d_root = nullptr;
    return;                                                           // RETURN
}

Widget&
Widget::operator=(Widget&& rhs) noexcept
{
    if (this != &rhs) {
        this->~Widget(); // Destructor is NOT virtual!
        new (this) Widget(std::move(rhs)); // uses move constructor
    }
    return *this;                                                     // RETURN
}

Widget&
Widget::addChild(Widget&& child) &
{
    if (!d_root) { // this method is a no-op once widget IDs are assigned
        children().push_back(std::move(child));
        if (d_methods && d_methods->d_newChildMethod) {
            d_methods->d_newChildMethod(this, &children().back());
        }
    }
    return *this;                                                     // RETURN
}

uint16_t
Widget::assignWidgetIds(uint16_t        next,
                        uint16_t        relativeId,
                        CharSizeMapPtr  map,
                        Widget         *root) noexcept
{
    if (root == nullptr) {
        next = 1;
        root = this;
        map = std::make_shared<Text::CharSizeMap>();
        d_layout.d_upperLeft = {-1.0, -1.0};
        d_layout.d_lowerRight= { 1.0,  1.0};
        text().d_layout.d_charSizeMap  = map;
        
        if (!text().d_layout.d_viewFn) {
            text().d_layout.d_viewFn = [] { return StringView_t(); };
        }
    }
    else if (hasText()) { // Don't create text layout if not needed.
        d_text->d_layout.d_charSizeMap = map;
    }
    d_root                      = root;

    if (d_layout.d_thickness == -1.0) {
        auto thickness = WawtEnv::defaultBorderThickness(optionName());
        d_layout.d_thickness = thickness;
    }

    if (!options().has_value()) {
        options(WawtEnv::defaultOptions(d_settings.d_optionName));
    }

    d_settings.d_relativeId = relativeId;
    relativeId  = 0;

    if (hasChildren()) {
        for (auto& widget : children()) {
            next = widget.assignWidgetIds(next, relativeId++, map, root);
        }
    }
    d_settings.d_widgetIdValue  = next++;
    return next;                                                      // RETURN
}

Widget::Children&
Widget::children() noexcept
{
    if (!d_children) {
        try {
            d_children = std::make_unique<Children>();
        }
        catch(...) {
            std::terminate();
        }
    }
    return *d_children;                                               // RETURN
}

const Widget::Children&
Widget::children() const noexcept
{
    return d_children ? *d_children : s_emptyList;                    // RETURN
}

Widget
Widget::clone() const
{
    // Copy everything EXCEPT for 'd_widgetLabel' (Tracker reference)
    auto copy = Widget{d_settings.d_optionName, {}};

    copy.d_root         = d_root;
    copy.d_downMethod   = d_downMethod;

    if (d_methods) {
        copy.d_methods  = std::make_unique<Methods>(*d_methods);
    }
    copy.d_layout       = d_layout;
    copy.d_rectangle    = d_rectangle;
    copy.d_settings     = d_settings;

    if (d_text) {
        copy.d_text     = std::make_unique<Text>(*d_text);
    }

    if (hasChildren()) {
        for (auto& child : children()) {
            copy.children().push_back(child.clone());
        }
    }
    return copy;                                                      // RETURN
}

EventUpCb
Widget::downEvent(double x, double y, Widget *parent)
{
    if (d_root == this && d_methods) {
        auto input = inputMethod();

        if (input) {
            inputMethod(InputMethod()); // lose focus
            input(this, WawtEnv::kFocusChg); // hide cursor
        }
    }
    auto upCb = EventUpCb();

    if (hasChildren()) {
        auto& widgets = children();

        for (auto rit = widgets.rbegin(); rit != widgets.rend(); ++rit) {
            if (upCb = rit->downEvent(x, y, this)) {
                break;                                                 // BREAK
            }
        }
    }

    if (!upCb && !isDisabled() && inside(x, y) && d_downMethod) {
        upCb = d_downMethod(x, y, this, parent);
    }
    return upCb;                                                      // RETURN
}

Widget::DownEventMethod
Widget::downEventMethod() const noexcept
{
    return d_downMethod;
}

void
Widget::draw(DrawProtocol *adapter) noexcept
{
    if (!isHidden()) {
        if (d_text && d_text->d_layout.d_refreshBounds) { // realign new string
            auto& data            = d_text->d_data;
            auto& layout          = d_text->d_layout;
            auto  charSizeLimit   = layout.upperLimit(d_rectangle);

            if (!data.resolveSizes(d_rectangle,
                                   charSizeLimit,
                                   adapter,
                                   d_settings.d_options)) {
                d_settings.d_successfulLayout = false;
                return;                                               // RETURN
            }
            d_settings.d_successfulLayout = true;
            data.d_upperLeft = layout.position(data.d_bounds, d_rectangle);
            layout.d_refreshBounds = false;
        }

        call(&defaultDraw, &Methods::d_drawMethod, this, adapter);

        if (hasChildren()) {
            for (auto& child : children()) {
                child.draw(adapter);
            }
        }
    }
    return;                                                           // RETURN
}

Widget::DrawMethod
Widget::drawMethod() const noexcept
{
    return d_methods ? d_methods->d_drawMethod : DrawMethod();
}

void
Widget::focus(Widget *target)  noexcept
{
    if (d_root) {
        if (d_root != this) {
            d_root->focus(target);
            return;                                                   // RETURN
        }

        if (target == nullptr) {
            if (d_methods) {
                inputMethod(InputMethod());
            }
            return;                                                   // RETURN
        }
        auto handler =  [target](Widget*, Char_t input) -> bool {
                            return target->inputEvent(input);
                        };
        inputMethod(handler);
        handler(target, WawtEnv::kFocusChg); // show cursor
    }
}

bool
Widget::inputEvent(Char_t input) noexcept
{
    auto handler = inputMethod();

    if (handler) {
        if (this == d_root) {
            inputMethod(InputMethod());
        }

        if (handler(this, input)) {
            inputMethod(std::move(handler));
            return true;                                              // RETURN
        }
    }
    return false;                                                     // RETURN
}

Widget::InputMethod
Widget::inputMethod() const noexcept
{
    return d_methods ? d_methods->d_inputMethod : InputMethod();
}

Widget::LayoutMethod
Widget::layoutMethod() const noexcept
{
    return d_methods ? d_methods->d_layoutMethod : LayoutMethod();
}

Widget*
Widget::lookup(WidgetId id) const noexcept
{
    const Widget *widget = nullptr;

    if (id.isSet()) {
        if (id.isRelative()) {
            if (id.value() < children().size()) {
                widget = &children().at(id.value());
            }
        }
        else if (id.value() == widgetIdValue()) {
            widget = this;
        }
        else {
            findWidget(&widget, *this, id.value());
        }
    }
    return const_cast<Widget*>(widget);                               // RETURN
}

Widget::NewChildMethod
Widget::newChildMethod() const noexcept
{
    return d_methods ? d_methods->d_newChildMethod : NewChildMethod();
}

void
Widget::popDialog()
{
    if (this == d_root) {
        if (hasChildren()) {
            auto& panel = children().back();

            if (0 == std::strcmp(panel.settings().d_optionName,
                                 WawtEnv::sPanel)
             && panel.hasChildren()
             && 0 == std::strcmp(panel.children().front()
                                      .settings().d_optionName,
                                 WawtEnv::sDialog)) {
                children().pop_back();
                d_settings.d_widgetIdValue
                        = hasChildren() ? children().back().widgetIdValue() + 1
                                        : 1;
            }
        }
    }
    else {
        d_root->popDialog();
    }
    return;                                                           // RETURN
}

WidgetId
Widget::pushDialog(Widget&& child, DrawProtocol *adapter)
{
    auto childId = WidgetId{};

    if (0      == std::strcmp(child.settings().d_optionName, WawtEnv::sDialog)
     && d_root != nullptr) {
        if (this == d_root) {
            if (!hasChildren()
             || (0 != std::strcmp(children().back().settings().d_optionName,
                                  WawtEnv::sDialog))) {
                auto id = widgetIdValue();
                auto relativeId
                        = hasChildren() ? children().back().relativeId() + 1
                                        : 0;
                // First push a transparent panel to soak up stray down
                // events:
                auto  panel  = Widget(WawtEnv::sPanel, Layout()) // full screen
                                .downEventMethod(eatDownEvents());
                auto& screen = children().emplace_back(std::move(panel));
                
                if (d_methods && d_methods->d_newChildMethod) {
                    d_methods->d_newChildMethod(this, &screen);
                }
                // Below that panel, put the dialog.
                screen.children().emplace_back(std::move(child));
                // Dialog's get their own char size map providing a
                // seperate "name space" for char size group IDs.
                auto map = std::make_shared<Text::CharSizeMap>();
                d_settings.d_widgetIdValue
                    = screen.assignWidgetIds(id,
                                             relativeId,
                                             map,
                                             d_root);
                screen.synchronizeTextView(true);
                screen.layout(adapter, true,  *d_root);
                screen.layout(adapter, false, *d_root);
                childId = WidgetId(id, false);
            }
        }
        else {
            childId = d_root->pushDialog(std::move(child), adapter);
        }
    }
    return childId;                                                   // RETURN
}

bool
Widget::resetLabel(Text::View_t&& newLabel)
{
    if (d_text) {
        d_text->d_layout.d_viewFn = std::move(newLabel.d_viewFn);
        d_text->d_layout.d_refreshBounds = true;
        d_text->d_data.d_view = d_text->d_layout.d_viewFn();
    }
    return bool(d_text);                                              // RETURN
}

void
Widget::resizeScreen(double width, double height, DrawProtocol *adapter)
{
    if (d_root && adapter) {
        if (d_root == this) {
            assert(text().d_layout.d_charSizeMap);
            text().d_layout.d_charSizeMap->clear();
            d_root->d_rectangle.d_bounds.d_width  = width;
            d_root->d_rectangle.d_bounds.d_height = height;
            d_root->d_rectangle.d_border
                = float(std::min(width, height) * layout().d_thickness/1000.0);

            if (hasChildren()) {
                auto& panel = children().back();

                if (0 == std::strcmp(panel.settings().d_optionName,
                                     WawtEnv::sPanel)
                 && panel.hasChildren()
                 && 0 == std::strcmp(panel.children().front()
                                          .settings().d_optionName,
                                     WawtEnv::sDialog)) {
                    clearCharSizeMap(&panel);
                }

                for (auto& child : children()) {
                    child.layout(adapter, true, *this);
                }

                for (auto& child : children()) {
                    child.layout(adapter, false, *this);
                }
            }
        }
        else {
            d_root->resizeScreen(width, height, adapter);
        }
    }
    return;                                                           // RETURN
}

void
Widget::serialize(std::ostream&     os,
                  unsigned int      indent)    const noexcept
{
    auto closeTag = std::string{};

    call(&defaultSerialize,
         &Methods::d_serializeMethod,
         os,
         &closeTag,
         *this,
         indent);

    indent += 2;

    if (hasChildren()) {
        for (auto& child : children()) {
            child.serialize(os, indent);
        }
    }
    os << closeTag;
    return;                                                           // RETURN
}

Widget::SerializeMethod
Widget::serializeMethod() const noexcept
{
    return d_methods ? d_methods->d_serializeMethod : SerializeMethod();
}

void
Widget::synchronizeTextView(bool recurse) noexcept
{
    if (d_text) {
        d_text->d_data.d_view = d_text->d_layout.d_viewFn();
        d_text->d_layout.d_refreshBounds; // realign new string
    }

    if (recurse && hasChildren()) {
        for (auto& child : children()) {
            child.synchronizeTextView(true);
        }
    }
}

const Text&
Widget::text() const noexcept
{
    return d_text ? *d_text : s_emptyText;
}

                                //-----------
                                // class Draw
                                //-----------


DrawStream::DrawStream()
: d_os(std::cout)
{
}

bool
DrawStream::draw(const Wawt::Layout::Result& box,
                 const Widget::Settings&     settings) noexcept
{
    auto  spaces     = Indent(0);
    auto  widgetName = settings.d_optionName;
    auto  widgetId   = settings.d_widgetIdValue;

    auto fmtflags = d_os.flags();
    d_os.setf(std::ios::boolalpha);

    d_os << spaces << "<" << widgetName << " id='" << widgetId
         << "' rid=" << settings.d_relativeId << "'>\n";

    spaces += 2;
    d_os << spaces
         << "<settings options='"  << settings.d_options.has_value()
         << "' selected='"         << bool(settings.d_selected)
         << "' disable='"          << bool(settings.d_disabled)
         << "' hidden='"           << bool(settings.d_hidden)
         << "'/>\n"
         << spaces
         << "<rect x='"       << std::floor(box.d_upperLeft.d_x)
         <<     "' y='"       << std::floor(box.d_upperLeft.d_y)
         <<     "' width='"   << std::ceil(box.d_bounds.d_width)
         <<     "' height='"  << std::ceil(box.d_bounds.d_height)
         <<     "' border='"  << std::floor(box.d_border)
         << "'/>\n";
    spaces -= 2;
    d_os << spaces << "</" << widgetName << ">\n";
    d_os.flags(fmtflags);
    return d_os.good();                                               // RETURN
}

bool
DrawStream::draw(const Text::Data&        text,
                 const Widget::Settings&  settings) noexcept
{
    auto  spaces     = Indent(0);
    auto  widgetName = settings.d_optionName;
    auto  widgetId   = settings.d_widgetIdValue;

    auto fmtflags = d_os.flags();
    d_os.setf(std::ios::boolalpha);

    d_os << spaces << "<" << widgetName << " id='" << widgetId
         << "' rid=" << settings.d_relativeId << "'>\n";

    spaces += 2;
    d_os << spaces
         << "<text x='"        << std::floor(text.d_upperLeft.d_x)
         <<     "' y='"        << std::floor(text.d_upperLeft.d_y)
         <<     "' width='"    << std::ceil(text.d_bounds.d_width)
         <<     "' height='"   << std::ceil(text.d_bounds.d_height)
         <<     "' charSize='" << text.d_charSize;

    if (text.d_labelMark != Text::BulletMark::eNONE) {
        d_os << "' mark='"    << int(text.d_labelMark)
             << "' left='"    << bool(text.d_leftAlignMark);
    }
    d_os << "'/>\n";
    spaces += 2;
    d_os << spaces << "<string>";
    outputXMLescapedString(d_os, text.view());
    d_os << "</string>\n";
    spaces -= 2;
    d_os << spaces << "</text>\n";
    spaces -= 2;
    d_os << spaces << "</" << widgetName << ">\n";
    d_os.flags(fmtflags);
    return d_os.good();                                               // RETURN
}

bool
DrawStream::getTextValues(Text::Data&      values,
                          const Bounds&    container,
                          uint16_t         upperLimit,
                          const std::any&)    noexcept
{
    auto  count     = values.view().length();

    if (values.d_labelMark != Text::BulletMark::eNONE) {
        count += 1;
    }
    auto  width     = container.d_width;
    auto  height    = container.d_height;
    auto  size      = count > 0 ? width/count : height;

    values.d_charSize       = size >= upperLimit ? upperLimit - 1 : size;
    values.d_bounds         = { float(count*(values.d_charSize)),
                                float(values.d_charSize) };
    return true;                                                      // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
