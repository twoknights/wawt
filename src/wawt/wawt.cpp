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
#define S(str) String_t(L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace Wawt {

namespace {

const Widget::Children s_emptyList;

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
wchar_t frontChar(const std::wstring_view& s) {
    return s.empty() ? '\0' : s[0];                                   // RETURN
}

inline
char32_t frontChar(const std::string_view& text) {
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

// The following defaults should be found in all fonts.
Char_t   WawtEnv::kFocusChg   = C('\0');

char     WawtEnv::sButton[]   = "button";
char     WawtEnv::sDialog[]   = "dialog";
char     WawtEnv::sEntry[]    = "entry";
char     WawtEnv::sItem[]     = "item";
char     WawtEnv::sLabel[]    = "label";
char     WawtEnv::sList[]     = "list";
char     WawtEnv::sPanel[]    = "panel";
char     WawtEnv::sScreen[]   = "screen";

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

void outputXMLString(std::ostream& os, StringView_t text)
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
    }
    return;                                                           // RETURN
}

Char_t popFrontChar(StringView_t& view)
{
    auto front = frontChar(view);
    if (front != '\0') {
        view.remove_prefix(sizeOfChar(front)/sizeof(Char_t));
    }
    return front;                                                     // RETURN
}

String_t toString(Char_t *charArray, std::size_t length)
{
    auto result = String_t{};

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

Tracker::Tracker(Tracker&& copy)
{
    d_widget = copy.d_widget;
    d_label  = copy.d_label;
    copy.d_widget   = nullptr;
    copy.d_label    = nullptr;

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
        d_widget = rhs.d_widget;
        d_label  = rhs.d_label;
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


Layout::Coordinates
Layout::Position::resolvePosition(const Widget& parent) const
{
    auto       reference  = d_widgetRef.getWidgetPointer(parent);
    auto&      rectangle  = reference->drawData().d_rectangle;
    auto&      ul_x       = rectangle.d_ux;
    auto&      ul_y       = rectangle.d_uy;
    auto       lr_x       = ul_x + rectangle.d_width;
    auto       lr_y       = ul_y + rectangle.d_height;

    auto xorigin = (ul_x  + lr_x)/2.0;
    auto yorigin = (ul_y  + lr_y)/2.0;
    auto xradius = lr_x - xorigin;
    auto yradius = lr_y - yorigin;

    if (reference == &parent || reference == parent.screen()) {
        auto       thickness  = std::ceil(reference->drawData().d_border);
        xradius -= thickness;
        yradius -= thickness;
    }

    auto x = xorigin + d_sX*xradius;
    auto y = yorigin + d_sY*yradius;

    return Coordinates(x,y);                                          // RETURN
}

                            //--------------
                            // class  Layout
                            //--------------

float
Layout::resolveBorder(const Widget& reference) const
{
    assert(reference.screen());
    auto min = std::min(reference.drawData().d_rectangle.d_width,
                        reference.drawData().d_rectangle.d_height);
    return float(min * d_thickness / 1000.0);                         // RETURN
}

Rectangle
Layout::resolveOutline(const Widget& parent) const
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
    return { ux, uy, lx - ux, ly - uy };                              // RETURN
}

Layout&
Layout::scale(double sx, double sy)
{
    if (d_upperLeft.d_widgetRef != d_lowerRight.d_widgetRef) {
        // No common coordinate system, so:
        throw WawtException("Incompatible widget references in scaling.");
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
                                //-------------
                                // class Widget
                                //-------------

// SPECIALIZATIONS
template<class Method>
Method
Widget::getInstalled() const noexcept
{
    return Method();
}

template<>
Widget::DownEventMethod
Widget::getInstalled() const noexcept
{
    return d_downMethod;
}

template<>
Widget::DrawMethod
Widget::getInstalled() const noexcept
{
    return d_methods ? d_methods->d_drawMethod : DrawMethod();
}

template<>
Widget::LayoutMethod
Widget::getInstalled() const noexcept
{
    return d_methods ? d_methods->d_layoutMethod : LayoutMethod();
}

template<>
Widget::NewChildMethod
Widget::getInstalled() const noexcept
{
    return d_methods ? d_methods->d_newChildMethod : NewChildMethod();
}

template<>
Widget::SerializeMethod
Widget::getInstalled() const noexcept
{
    return d_methods ? d_methods->d_serializeMethod : SerializeMethod();
}

// PRIVATE MEMBERS
void
Widget::layout(DrawProtocol       *adapter,
               bool                firstPass,
               const Widget&       parent)
{
    call(&defaultLayout,
         &Methods::d_layoutMethod,
         this,
         parent,
         firstPass,
         adapter);

    d_layoutData.d_refreshBounds = false;

    auto space = std::min(d_drawData.d_rectangle.d_width,
                          d_drawData.d_rectangle.d_height)
               - 2*d_drawData.d_border;

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
    adapter->draw(widget->drawData());
}

void
Widget::defaultLayout(Widget                  *widget,
                      const Widget&            parent,
                      bool                     firstPass,
                      DrawProtocol            *adapter) noexcept
{
    auto&       data       = widget->drawData();
    auto const& layoutData = widget->layoutData();

    if (firstPass) {
        data.d_rectangle = layoutData.d_layout.resolveOutline(parent);
        data.d_border    = layoutData.d_layout.resolveBorder(*parent.screen());
    }

    if (!data.d_label.empty()
     || data.d_labelMark != DrawData::BulletMark::eNONE) {
        labelLayout(&data, firstPass, layoutData, adapter);
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
    auto& drawData   = widget.drawData();
    auto& layoutData = widget.layoutData();
    auto& widgetName = drawData.d_optionName;
    auto& widgetId   = drawData.d_widgetId;
    auto& layout     = layoutData.d_layout;

    auto fmtflags = os.flags();
    os.setf(std::ios::boolalpha);

    std::ostringstream ss;
    ss << spaces << "</" << widgetName << ">\n";
    os << spaces << "<" << widgetName << " id='" << widgetId
       << "' rid='" << drawData.d_relativeId << "'>\n";

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

    if (!drawData.d_label.empty()) {
        os << spaces
           << "<text align='"   << int(layoutData.d_textAlign)
           << "' group='";

        if (layoutData.d_charSizeGroup) {
            os << *layoutData.d_charSizeGroup;
        }

        if (drawData.d_labelMark != DrawData::BulletMark::eNONE) {
            os << "' mark='"    << int(drawData.d_labelMark)
               << "' left='"    << bool(drawData.d_leftMark);
        }
        os << "'>";
        outputXMLString(os, drawData.d_label);
        os << "</text>\n";
    }
    else {
        os << spaces << "<text/>\n";
    }
    std::stringstream im;
    spaces += 2;

    if (widget.getInstalled<Widget::DownEventMethod>()) {
        im << spaces << "<downMethod type='functor'/>\n";
    }
    
    if (widget.getInstalled<Widget::DrawMethod>()) {
        im << spaces << "<drawMethod type='functor'/>\n";
    }

    if (widget.getInstalled<Widget::LayoutMethod>()) {
        im << spaces << "<layoutMethod type='functor'/>\n";
    }
    
    if (widget.getInstalled<Widget::NewChildMethod>()) {
        im << spaces << "<newChildMethod type='functor'/>\n";
    }
    
    if (widget.getInstalled<Widget::SerializeMethod>()) {
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

void
Widget::labelLayout(DrawData                  *data,
                    bool                       firstPass,
                    const Widget::LayoutData&  layoutData,
                    DrawProtocol              *adapter) noexcept
{
    auto charSizeMap      = layoutData.d_charSizeMap.get();
    auto charSizeLimit    = data->d_rectangle.d_height;
    auto borderAdjustment = 2*(data->d_border + 1);

    if (charSizeLimit <= borderAdjustment) {
        return;                                                       // RETURN
    }
    charSizeLimit -= borderAdjustment;

    if (layoutData.d_charSizeGroup) {
        if (auto it  = charSizeMap->find(*layoutData.d_charSizeGroup);
                 it != charSizeMap->end()) {
            if (charSizeLimit > it->second + 1) {
                charSizeLimit = it->second + 1;
            }
        }
    }
    data->d_labelBounds.d_ux= data->d_rectangle.d_ux + data->d_border + 1;
    data->d_labelBounds.d_uy= data->d_rectangle.d_uy + data->d_border + 1;

    if (firstPass || data->d_charSize + 1 != charSizeLimit) {
        auto textBounds = Dimensions{
            data->d_rectangle.d_width  - borderAdjustment,
            data->d_rectangle.d_height - borderAdjustment };

        if (data->d_label.empty()) {
            data->d_charSize = charSizeLimit-1;
        }
        else if (!adapter->getTextMetrics(&textBounds,
                                          &data->d_charSize,
                                          *data,
                                          charSizeLimit)) {
            assert(!"Failed to get text metrics.");
            data->d_charSize    = charSizeLimit - 1;
        }
        else {
            assert(data->d_charSize < charSizeLimit);
        }

        if (layoutData.d_charSizeGroup) {
            (*charSizeMap)[*layoutData.d_charSizeGroup] = data->d_charSize;
        }
        data->d_labelBounds.d_width   = textBounds.d_x;
        data->d_labelBounds.d_height  = textBounds.d_y;
    }

    if (layoutData.d_textAlign != TextAlign::eLEFT) {
        auto space = data->d_rectangle.d_width
                   - data->d_labelBounds.d_width
                   - borderAdjustment;

        if (layoutData.d_textAlign == TextAlign::eCENTER) {
            space /= 2.0f;
        }
        data->d_labelBounds.d_ux += space;
    }
    auto space = data->d_rectangle.d_height
               - data->d_labelBounds.d_height
               - borderAdjustment;

    space /= 2.0f;
    data->d_labelBounds.d_uy += space;
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
Widget::method(DownEventMethod&& newMethod) && noexcept
{
    d_downMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::method(DrawMethod&& newMethod) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_drawMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::method(InputMethod&& newMethod) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_inputMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::method(LayoutMethod&& newMethod) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_layoutMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::method(NewChildMethod&& newMethod) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_newChildMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::method(SerializeMethod&& newMethod) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_serializeMethod = std::move(newMethod);
    return std::move(*this);                                          // RETURN
}

Widget&
Widget::method(DownEventMethod&& newMethod) & noexcept
{
    d_downMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::method(DrawMethod&& newMethod) & noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_drawMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::method(InputMethod&& newMethod) & noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_inputMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::method(LayoutMethod&& newMethod) & noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_layoutMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::method(NewChildMethod&& newMethod) & noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_newChildMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget&
Widget::method(SerializeMethod&& newMethod) & noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_serializeMethod = std::move(newMethod);
    return *this;                                                     // RETURN
}

Widget
Widget::text(StringView_t string, CharSizeGroup group, TextAlign alignment) &&
                                                                      noexcept
{
    d_layoutData.d_charSizeGroup    = group;
    d_layoutData.d_textAlign        = alignment;
    d_drawData.d_label              = WawtEnv::translate(string);
    return std::move(*this);                                          // RETURN
}

// PUBLIC MEMBERS
Widget::Widget(Widget&& copy) noexcept
{
    d_widgetLabel   = std::move(copy.d_widgetLabel);
    d_root          = copy.d_root;
    d_textHit       = copy.d_textHit;
    d_methods       = std::move(copy.d_methods);
    d_downMethod    = std::move(copy.d_downMethod);
    d_drawData      = std::move(copy.d_drawData);
    d_layoutData    = std::move(copy.d_layoutData);
    d_children      = std::move(copy.d_children);

    if (d_widgetLabel) {
        d_widgetLabel.update(this);
    }
    return;                                                           // RETURN
}

Widget&
Widget::operator=(Widget&& rhs) noexcept
{
    if (this != &rhs) {
        d_widgetLabel   = std::move(rhs.d_widgetLabel);
        d_root          = rhs.d_root;
        d_textHit       = rhs.d_textHit;
        d_methods       = std::move(rhs.d_methods);
        d_downMethod    = std::move(rhs.d_downMethod);
        d_drawData      = std::move(rhs.d_drawData);
        d_layoutData    = std::move(rhs.d_layoutData);
        d_children      = std::move(rhs.d_children);

        if (d_widgetLabel) {
            d_widgetLabel.update(this);
        }
    }
    return *this;                                                     // RETURN
}

Widget&
Widget::addChild(Widget&& child) &
{
    if (!d_root) { // a no-op once widget IDs are assigned
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
        map = std::make_shared<CharSizeMap>();
        d_layoutData.d_layout.d_upperLeft = {-1.0, -1.0};
        d_layoutData.d_layout.d_lowerRight= { 1.0,  1.0};
    }

    if (d_layoutData.d_layout.d_thickness == -1.0) {
        auto thickness = WawtEnv::defaultBorderThickness(optionName());
        d_layoutData.d_layout.d_thickness = thickness;
    }

    if (!options().has_value()) {
        options(WawtEnv::defaultOptions(d_drawData.d_optionName));
    }
    d_drawData.d_relativeId = relativeId;
    relativeId  = 0;

    if (hasChildren()) {
        for (auto& widget : children()) {
            next = widget.assignWidgetIds(next, relativeId++, map, root);
        }
    }
    d_layoutData.d_charSizeMap  = map;
    d_drawData.d_widgetId       = next++;
    d_root                      = root;
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
    auto copy = Widget{d_drawData.d_optionName, {}};

    copy.d_root          = d_root;
    copy.d_textHit       = d_textHit;
    copy.d_downMethod    = d_downMethod;

    if (d_methods) {
        copy.d_methods = std::make_unique<Methods>(*d_methods);
    }
    copy.d_drawData      = d_drawData;
    copy.d_layoutData    = d_layoutData;

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
    if (d_root == this) {
        auto input = getInstalled<InputMethod>();

        if (input) {
            input(this, WawtEnv::kFocusChg); // hide cursor
            method(InputMethod()); // lose focus
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

void
Widget::draw(DrawProtocol *adapter) noexcept
{
    if (!isHidden()) {
        if (d_layoutData.d_refreshBounds) { // realign new string
            labelLayout(&d_drawData, false, d_layoutData, adapter);
            d_layoutData.d_refreshBounds = false;
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

bool
Widget::inputEvent(Char_t input) noexcept
{
    auto handler = getInstalled<InputMethod>();

    if (handler) {
        if (handler(this, input)) {
            return true;                                              // RETURN
        }

        if (this == d_root) {
            method(InputMethod());
        }
    }
    return false;                                                     // RETURN
}

const Widget*
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
    return widget;                                                    // RETURN
}

void
Widget::popDialog()
{
    if (this == d_root) {
        if (hasChildren()) {
            if (0 == std::strcmp(children().back().drawData().d_optionName,
                                 WawtEnv::sDialog)) {
                children().pop_back();
                d_drawData.d_widgetId
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

    if (0      == std::strcmp(child.drawData().d_optionName, WawtEnv::sDialog)
     && d_root != nullptr) {
        if (this == d_root) {
            if (!hasChildren()
             || (0 != std::strcmp(children().back().drawData().d_optionName,
                                  WawtEnv::sDialog))) {
                auto id = widgetIdValue();
                auto relativeId
                        = hasChildren() ? children().back().relativeId() + 1
                                        : 0;
                // First push a transparent panel to soak up stray down
                // events:
                auto  panel  = Widget(WawtEnv::sPanel, Layout()) // full screen
                                .method(eatDownEvents());
                auto& screen = children().emplace_back(std::move(panel));
                
                if (d_methods && d_methods->d_newChildMethod) {
                    d_methods->d_newChildMethod(this, &screen);
                }
                // Below that panel, put the dialog.
                screen.children().emplace_back(std::move(child));
                // Dialog's get their own char size map providing a
                // seperate "name space" for char size group IDs.
                d_drawData.d_widgetId
                    = screen.assignWidgetIds(id,
                                             relativeId,
                                             std::make_shared<CharSizeMap>(),
                                             d_root);
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

void
Widget::resetLabel(StringView_t newLabel, bool copy)
{
    d_drawData.d_label = copy ? WawtEnv::translate(std::move(newLabel))
                              : newLabel;
    d_layoutData.d_refreshBounds = true;
    return;                                                           // RETURN
}

void
Widget::resizeScreen(double width, double height, DrawProtocol *adapter)
{
    if (d_root && adapter) {
        if (d_root == this) {
            assert(d_layoutData.d_charSizeMap);
            d_layoutData.d_charSizeMap->clear();
            d_root->d_drawData.d_rectangle.d_width  = width;
            d_root->d_drawData.d_rectangle.d_height = height;
            d_root->d_drawData.d_border
                = float(width * layout().d_thickness/1000.0);

            if (hasChildren()) {
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

    for (auto& child : children()) {
        child.serialize(os, indent);
    }
    os << closeTag;
    return;                                                           // RETURN
}

void
Widget::setFocus(Widget *target)  noexcept
{
    if (d_root) {
        if (d_root != this) {
            d_root->setFocus(target);
            return;                                                   // RETURN
        }

        if (target == nullptr) {
            if (d_methods) {
                method(InputMethod());
            }
            return;                                                   // RETURN
        }
        auto handler =  [target](Widget*, Char_t input) -> bool {
                            return target->inputEvent(input);
                        };
        method(handler);
        handler(target, WawtEnv::kFocusChg); // show cursor
    }
}

                                //-----------
                                // class Draw
                                //-----------


Draw::Draw()
: d_os(std::cout)
{
}

bool
Draw::draw(const DrawData& drawData)  noexcept
{
    auto  spaces     = Indent(0);
    auto& widgetName = drawData.d_optionName;
    auto& widgetId   = drawData.d_widgetId;

    auto fmtflags = d_os.flags();
    d_os.setf(std::ios::boolalpha);

    d_os << spaces << "<" << widgetName << " id='" << widgetId
         << "' rid=" << drawData.d_relativeId << "'>\n";

    spaces += 2;
    d_os << spaces
         << "<draw options='" << drawData.d_options.has_value()
         << "' selected='"    << bool(drawData.d_selected)
         << "' disable='"     << bool(drawData.d_disableEffect)
         << "' hidden='"      << bool(drawData.d_hidden)
         << "'>\n";
    spaces += 2;
    d_os << spaces
         << "<rect x='"       << std::floor(drawData.d_rectangle.d_ux)
         <<     "' y='"       << std::floor(drawData.d_rectangle.d_uy)
         <<     "' width='"   << std::ceil(drawData.d_rectangle.d_width)
         <<     "' height='"  << std::ceil(drawData.d_rectangle.d_height)
         <<     "' border='"  << std::ceil(drawData.d_border)
         << "'/>\n";

    if (drawData.d_labelBounds.d_width > 0) {
        d_os << spaces
             << "<text x='"      << std::round(drawData.d_labelBounds.d_ux)
             <<     "' y='"      << std::round(drawData.d_labelBounds.d_uy)
             <<     "' width='"  << std::round(drawData.d_labelBounds.d_width)
             <<     "' height='" << std::round(drawData.d_labelBounds.d_height)
             <<     "' charSize='" << drawData.d_charSize;

        if (drawData.d_labelMark != DrawData::BulletMark::eNONE) {
            d_os << "' mark='"    << int(drawData.d_labelMark)
                 << "' left='"    << bool(drawData.d_leftMark);
        }
        d_os << "'/>\n" << spaces << "<string>";
        outputXMLString(d_os, drawData.d_label);
        d_os << "</string>\n";
    }
    spaces -= 2;
    d_os << spaces << "</draw>\n";
    spaces -= 2;
    d_os << spaces << "</" << widgetName << ">\n";
    d_os.flags(fmtflags);
    return d_os.good();                                               // RETURN
}

bool
Draw::getTextMetrics(Dimensions          *textBounds,
                     DrawData::CharSize  *charSize,
                     const DrawData&      drawData,
                     DrawData::CharSize   upperLimit)    noexcept
{
    auto  count     = drawData.d_label.length();

    if (drawData.d_labelMark != DrawData::BulletMark::eNONE) {
        count += 1;
    }
    auto  thickness = std::ceil(drawData.d_border);
    auto  width     = drawData.d_rectangle.d_width  - 2*thickness - 2;
    auto  height    = drawData.d_rectangle.d_height - 2*thickness - 2;
    auto  size      = count > 0 ? width/count : height;

    *charSize   = size >= upperLimit ? upperLimit - 1 : size;
    *textBounds = { float(count*(*charSize)), float(*charSize) };
    return true;                                                      // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
