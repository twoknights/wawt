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
#include <iostream>
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

inline
std::ostream& operator<<(std::ostream& os, Indent indent) {
    if (indent.d_indent > 0) {
        os << std::setw(indent.d_indent) << ' ';
    }
    return os;                                                        // RETURN
}

inline
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

inline bool isBackspace(wchar_t ch) {
    return ch == L'\b';
}

inline bool isBackspace(char32_t ch) {
    return ch == U'\b';
}

inline bool isEnter(wchar_t ch) {
    return ch == L'\r';
}

inline bool isEnter(char32_t ch) {
    return ch == U'\r';
}

inline bool isEos(wchar_t ch) {
    return ch == L'\0';
}

inline bool isEos(char32_t ch) {
    return ch == U'\0';
}

inline std::wstring makeString(wchar_t ch) {
    return isEos(ch) ? std::wstring()
                     : std::wstring(1, static_cast<wchar_t>(ch));
}

inline
std::string makeString(char32_t ch) {
    std::string result;
    if (!isEos(ch)) {
        auto c = static_cast<uint32_t>(ch);
        switch (sizeOfChar(ch)) {
          case 4: result.push_back(char(0360|((c >> 18)&007)));
                  result.push_back(char(0200|((c >> 12)&077)));
                  result.push_back(char(0200|((c >>  6)&077)));
                  result.push_back(char(0200|( c       &077)));
                  break;
          case 3: result.push_back(char(0340|((c >> 12)&017)));
                  result.push_back(char(0200|((c >>  6)&077)));
                  result.push_back(char(0200|( c       &077)));
                  break;
          case 2: result.push_back(char(0300|((c >>  6)&037)));
                  result.push_back(char(0200|( c       &077)));
                  break;
          default:result.push_back(char(       c    &  0177));
                  break;
        };
    }
    return result;
}

inline int textLength(const std::wstring& text) {
    return text.length();
}

inline int textLength(const std::string& text) {
    int   length = 0;
    auto  p      = text.c_str();
    auto  end    = p + text.length();
    while (p < end && *p) {
        auto c = *p;
        ++length;
        p += (c&0340) == 0340 ? ((c&020) ? 4 : 3) : ((c&0200) ? 2 : 1);
    }
    return length;
}

inline
void outputXMLString(std::ostream& os, const std::string_view& text) {
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

inline
void outputXMLString(std::ostream& os, const std::wstring_view& text) {
    // NOTE: std::codecvt is deprecated. So, to avoid compiler warnings,
    // and until this is sorted out, lets roll our own.
    // For anticipated use (i.e. test drivers), this is expected to be
    // good enough.
    for (auto c : text) {
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

float borderSize(const Rectangle& root, float thickness) {
    auto scalex = thickness * root.d_width  / 1280.0;
    auto scaley = thickness * root.d_height /  720.0;
    auto value  = std::round(std::min(scalex, scaley));

    if (thickness > 0 && value == 0) {
        value = 1.f;
    }
    return value;
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

using Corner = std::pair<float,float>;

Corner findCorner(const Layout::Position& position,
                  const Widget&           parent,
                  const Widget&           root) {
    auto       widget     = position.d_widgetRef.getWidgetPointer(parent, root);
    auto&      rectangle  = widget->drawData().d_rectangle;
    auto&      ul_x       = rectangle.d_ux;
    auto&      ul_y       = rectangle.d_uy;
    auto       lr_x       = ul_x + rectangle.d_width;
    auto       lr_y       = ul_y + rectangle.d_height;
    auto       thickness  = rectangle.d_borderThickness;

    auto xorigin = (ul_x  + lr_x)/2.0;
    auto yorigin = (ul_y  + lr_y)/2.0;
    auto xradius = lr_x - xorigin;
    auto yradius = lr_y - yorigin;

    switch (position.d_normalizeX) {
      case Layout::Normalize::eOUTER:  { // Already have this.
        } break;
      case Layout::Normalize::eMIDDLE: {
            xradius -= thickness/2;
        } break;
      case Layout::Normalize::eINNER: {
            xradius -= thickness;
        } break;
      case Layout::Normalize::eDEFAULT: {
            if (widget == &parent) {
                xradius -= thickness; // eINNER
            }
        } break;
    };

    switch (position.d_normalizeY) {
        case Layout::Normalize::eOUTER:  { // Already have this.
        } break;
      case Layout::Normalize::eMIDDLE: {
            yradius -= thickness/2;
        } break;
      case Layout::Normalize::eINNER: {
            yradius -= thickness;
        } break;
      case Layout::Normalize::eDEFAULT: {
            if (widget == &parent) {
                yradius -= thickness; // eINNER
            }
        } break;
    };

    auto x = xorigin + position.d_sX*xradius;
    auto y = yorigin + position.d_sY*yradius;

    return Corner(x,y);                                               // RETURN
}

Rectangle makeRectangle(const Layout&         layout,
                        const Widget&         parent,
                        const Widget&         root) {
    auto thickness  = borderSize(root.drawData().d_rectangle,
                                 layout.d_thickness);
    auto [ux, uy]   = findCorner(layout.d_upperLeft,  parent, root);
    auto [lx, ly]   = findCorner(layout.d_lowerRight, parent, root);
    auto width      = lx - ux;
    auto height     = ly - uy;

    if (layout.d_pin != Layout::Vertex::eNONE) {
        auto  square = (width + height)/2.0;

        if (square > width) {
            square = width;
        }
        else if (square > height) {
            square = height;
        }
        auto  deltaW = square - width;
        auto  deltaH = square - height;

        switch (layout.d_pin) {
          case Layout::Vertex::eUPPER_LEFT: {
                ly += deltaH; lx += deltaW;
            } break;
          case Layout::Vertex::eUPPER_CENTER: {
                ly += deltaH; lx += deltaW/2.0;
                              ux -= deltaW/2.0;
            } break;
          case Layout::Vertex::eUPPER_RIGHT: {
                ly += deltaH;
                              ux -= deltaW;
            } break;
          case Layout::Vertex::eCENTER_LEFT: {
                uy -= deltaH/2.0;
                ly += deltaH/2.0; lx += deltaW;
            } break;
          case Layout::Vertex::eCENTER_CENTER: {
                uy -= deltaH/2.0; ux -= deltaW/2.0;
                ly += deltaH/2.0; lx += deltaW/2.0;
            } break;
          case Layout::Vertex::eCENTER_RIGHT: {
                uy -= deltaH/2.0; ux -= deltaW;
                ly += deltaH/2.0;
            } break;
          case Layout::Vertex::eLOWER_LEFT: {
                uy -= deltaH;
                              lx += deltaW;
            } break;
          case Layout::Vertex::eLOWER_CENTER: {
                uy -= deltaH; ux -= deltaW/2.0;
                              lx += deltaW/2.0;
            } break;
          case Layout::Vertex::eLOWER_RIGHT: {
                uy -= deltaH; ux -= deltaW;
            } break;
          default: break;
        }
    }
    return { ux, uy, lx - ux, ly - uy, thickness };                   // RETURN
}

} // end unnamed namespace

std::atomic_flag    WawtEnv::d_atomicFlag   = ATOMIC_FLAG_INIT;
WawtEnv            *WawtEnv::d_instance     = nullptr;
DrawProtocol       *WawtEnv::d_drawAdapter  = nullptr;

// The following defaults should be found in all fonts.
Char_t   WawtEnv::kDownArrow = C('v');
Char_t   WawtEnv::kUpArrow   = C('^');
Char_t   WawtEnv::kCursor    = C('|');
Char_t   WawtEnv::kFocusChg  = C('\0');

                            //-------------------------
                            // class  Layout::WidgetRef
                            //-------------------------

const Widget *
Layout::WidgetRef::getWidgetPointer(const Widget&     parent,
                                    const Widget&     root) const
{
    if (d_widget != nullptr && *d_widget != nullptr) {
        return *d_widget;                                             // RETURN
    }
    const Widget *widget = nullptr;

    if (d_widgetId.isSet()) {
        if (d_widgetId.isRelative()) {
            if (d_widgetId == WidgetId::kPARENT) {
                widget = &parent;
            }
            else if (d_widgetId == WidgetId::kROOT) {
                widget = &root;
            }
            else {
                auto offset = d_widgetId.value();

                if (offset < parent.children().size()) {
                    widget = &parent.children()[offset];
                }
            }
        }
        else if (root.widgetIdValue() == d_widgetId.value()) {
            widget = &root;
        }
        else {
            findWidget(&widget, root, d_widgetId.value());
        }
    }

    if (!widget) {
        throw WawtException("Widget ID is invalid.", d_widgetId);
    }
    return widget;                                                    // RETURN
}

WidgetId
Layout::WidgetRef::getWidgetId() const noexcept
{
    WidgetId ret = d_widgetId;

    if (!ret.isSet()) { // if widget ID is not available
        if (d_widget && *d_widget) { // but a forwarding pointer is
            ret = WidgetId((*d_widget)->widgetIdValue(), false); 
        }
    }
    return ret;                                                       // RETURN
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
         &d_drawData,
         firstPass,
         *d_root,
         parent,
         d_layoutData,
         adapter);

    d_layoutData.d_refreshBounds = false;

    if (hasChildren()) {
        for (auto& child : children()) {
            child.layout(adapter, firstPass, *this);
        }
    }
}

// PUBLIC CLASS MEMBERS
void
Widget::defaultDraw(DrawProtocol *adapter, Widget *widget)
{
    adapter->draw(widget->drawData());
}

void
Widget::defaultLayout(DrawData                *data,
                      bool                     firstPass,
                      const Widget&            root,
                      const Widget&            parent,
                      const LayoutData&        layoutData,
                      DrawProtocol            *adapter) noexcept
{
    if (firstPass) {
        data->d_rectangle = makeRectangle(layoutData.d_layout, parent, root);
    }

    if (!data->d_label.empty()) {
        auto borderAdjustment = 2*data->d_rectangle.d_borderThickness + 2;
        auto charSizeMap      = layoutData.d_charSizeMap.get();
        auto charSizeLimit    = data->d_rectangle.d_height;

        if (charSizeLimit <= borderAdjustment) {
            return;                                                   // RETURN
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

        if (firstPass || data->d_charSize + 1 != charSizeLimit) {
            auto textBounds = Dimensions{
                data->d_rectangle.d_width  - borderAdjustment,
                data->d_rectangle.d_height - borderAdjustment };

            if (adapter->getTextMetrics(&textBounds,
                                        &data->d_charSize,
                                        *data,
                                        charSizeLimit)) {
            }
            else {
                assert(!"Failed to get text metrics.");
                data->d_charSize    = charSizeLimit - 1;
            }

            if (layoutData.d_charSizeGroup) {
                charSizeMap->emplace(*layoutData.d_charSizeGroup,
                                     data->d_charSize);
            }
            data->d_labelBounds.d_width   = textBounds.d_width;
            data->d_labelBounds.d_height  = textBounds.d_height;
        }
        data->d_labelBounds.d_ux = data->d_rectangle.d_ux
                                 + data->d_rectangle.d_borderThickness + 1;
        data->d_labelBounds.d_uy = data->d_rectangle.d_uy
                                 + data->d_rectangle.d_borderThickness + 1;

        if (layoutData.d_textAlign != TextAlign::eLEFT) {
            auto space = data->d_rectangle.d_width
                       - data->d_labelBounds.d_width
                       - 2*data->d_rectangle.d_borderThickness - 2;

            if (layoutData.d_textAlign == TextAlign::eCENTER) {
                space /= 2.0f;
            }
            data->d_labelBounds.d_ux += space;
        }
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
    auto& widgetName = drawData.d_className;
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
       <<   "' norm_x='" << int(layout.d_upperLeft.d_normalizeX)
       <<   "' norm_y='" << int(layout.d_upperLeft.d_normalizeY)
       << "'/>\n";
    os << spaces
       <<     "<lr sx='" << layout.d_lowerRight.d_sX
       <<       "' sy='" << layout.d_lowerRight.d_sY
       <<   "' widget='" << layout.d_lowerRight.d_widgetRef.getWidgetId()
       <<   "' norm_x='" << int(layout.d_lowerRight.d_normalizeX)
       <<   "' norm_y='" << int(layout.d_lowerRight.d_normalizeY)
       << "'/>\n";
    spaces -= 2;
    os << spaces << "</layout>\n";

    if (!drawData.d_label.empty()) {
        os << spaces
           << "<text align='"   << int(layoutData.d_textAlign)
           << "' charSize='"    << drawData.d_charSize
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

// PUBLIC R-Value MANIPULATORS

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
Widget::addMethod(DownEventMethod&& method) && noexcept
{
    d_downMethod = std::move(method);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::addMethod(DrawMethod&& method) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_drawMethod = std::move(method);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::addMethod(LayoutMethod&& method) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_layoutMethod = std::move(method);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::addMethod(NewChildMethod&& method) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_newChildMethod = std::move(method);
    return std::move(*this);                                          // RETURN
}

Widget
Widget::addMethod(SerializeMethod&& method) && noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_serializeMethod = std::move(method);
    return std::move(*this);                                          // RETURN
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
    d_widgetLabel   = copy.d_widgetLabel;
    d_root          = copy.d_root;
    d_textHit       = copy.d_textHit;
    d_methods       = std::move(copy.d_methods);
    d_downMethod    = std::move(copy.d_downMethod);
    d_drawData      = std::move(copy.d_drawData);
    d_layoutData    = std::move(copy.d_layoutData);
    d_children      = std::move(copy.d_children);

    if (d_widgetLabel) {
        *d_widgetLabel     = this;
        copy.d_widgetLabel = nullptr;
    }
    return;                                                           // RETURN
}

Widget::~Widget() noexcept
{
    if (d_widgetLabel) {
        *d_widgetLabel = nullptr;
    }
    return;                                                           // RETURN
}

Widget&
Widget::operator=(Widget&& rhs) noexcept
{
    if (this != &rhs) {
        d_widgetLabel   = rhs.d_widgetLabel;
        d_root          = rhs.d_root;
        d_textHit       = rhs.d_textHit;
        d_methods       = std::move(rhs.d_methods);
        d_downMethod    = std::move(rhs.d_downMethod);
        d_drawData      = std::move(rhs.d_drawData);
        d_layoutData    = std::move(rhs.d_layoutData);
        d_children      = std::move(rhs.d_children);

        if (d_widgetLabel) {
            *d_widgetLabel    = this;
            rhs.d_widgetLabel = nullptr;
        }
    }
    return *this;                                                     // RETURN
}

Widget&
Widget::addChild(Widget&& child) &
{
    if (!d_root) {
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
    }

    if (d_layoutData.d_layout.d_thickness == -1.0) {
        d_layoutData.d_layout.d_thickness =
            WawtEnv::defaultBorderThickness(d_drawData.d_className);
    }

    if (!d_drawData.d_options.has_value()) {
        d_drawData.d_options =
            WawtEnv::defaultOptions(d_drawData.d_className);
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

    if (d_widgetLabel) {
        *d_widgetLabel = this;
    }
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
    auto copy = Widget{d_drawData.d_className, {}};

    copy.d_widgetLabel   = nullptr;
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
Widget::downEvent(float x, float y)
{
    auto upCb = EventUpCb();

    if (!isDisabled() && inside(x, y)) {
        if (d_downMethod) {
            upCb = d_downMethod(x, y, this);
        }

        if (!upCb && hasChildren()) {
            for (auto& child : children()) {
                if (upCb = child.downEvent(x, y)) {
                    break;                                             // BREAK
                }
            }
        }
    }
    return upCb;                                                      // RETURN
}

void
Widget::draw(DrawProtocol *adapter) noexcept
{
    if (!isHidden()) {
        if (d_layoutData.d_refreshBounds) {
            defaultLayout(&d_drawData,
                          false,
                          *d_root,
                          *this,
                          d_layoutData,
                          adapter);
            d_layoutData.d_refreshBounds = false;
        }

        call(&defaultDraw, &Methods::d_drawMethod, adapter, this);

        if (hasChildren()) {
            for (auto& child : children()) {
                child.draw(adapter);
            }
        }
    }
    return;                                                           // RETURN
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
            if (0 == std::strcmp(children().back().drawData().d_className,
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
Widget::pushDialog(DrawProtocol *adapter, Widget&& child)
{
    auto childId = WidgetId{};

    if (0      == std::strcmp(child.drawData().d_className, WawtEnv::sDialog)
     && d_root != nullptr) {
        if (this == d_root) {
            if (!hasChildren()
             || (0 != std::strcmp(children().back().drawData().d_className,
                                  WawtEnv::sDialog))) {
                auto id = widgetIdValue();
                auto relativeId
                        = hasChildren() ? children().back().relativeId() + 1
                                        : 0;
                // Dialog's get their own char size map providing a
                // seperate "name space" for char size group IDs.
                auto& dialog = children().emplace_back(std::move(child));
                
                if (d_methods && d_methods->d_newChildMethod) {
                    d_methods->d_newChildMethod(this, &dialog);
                }
                d_drawData.d_widgetId
                    = dialog.assignWidgetIds(id,
                                             relativeId,
                                             std::make_shared<CharSizeMap>(),
                                             d_root);
                dialog.layout(adapter, true,  *d_root);
                dialog.layout(adapter, false, *d_root);
                childId = WidgetId(id, false);
            }
        }
        else {
            childId = d_root->pushDialog(adapter, std::move(child));
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
Widget::resizeScreen(DrawProtocol *adapter, float width, float height)
{
    if (d_root) {
        assert(d_layoutData.d_charSizeMap);
        d_layoutData.d_charSizeMap->clear();
        d_root->d_drawData.d_rectangle.d_width  = width;
        d_root->d_drawData.d_rectangle.d_height = height;

        if (hasChildren()) {
            for (auto& child : children()) {
                child.layout(adapter, true, *this);
                child.layout(adapter, false, *this);
            }
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
Widget::setMethod(DownEventMethod&& method) noexcept
{
    d_downMethod = std::move(method);
    return;                                                           // RETURN
}

void
Widget::setMethod(DrawMethod&& method) noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_drawMethod = std::move(method);
    return;                                                           // RETURN
}

void
Widget::setMethod(LayoutMethod&& method) noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_layoutMethod = std::move(method);
    return;                                                           // RETURN
}

void
Widget::setMethod(NewChildMethod&& method) noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_newChildMethod = std::move(method);
    return;                                                           // RETURN
}

void
Widget::setMethod(SerializeMethod&& method) noexcept
{
    if (!d_methods) {
        d_methods = std::make_unique<Methods>();
    }
    d_methods->d_serializeMethod = std::move(method);
    return;                                                           // RETURN
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
    auto& widgetName = drawData.d_className;
    auto& widgetId   = drawData.d_widgetId;

    d_os << spaces << "<" << widgetName << " id='" << widgetId
         << "' rid=" << drawData.d_relativeId << "'>\n";

    spaces += 2;
    d_os << spaces
         << "<draw options='" << int(drawData.d_options.has_value())
         << "' selected='"    << drawData.d_selected
         << "' disable='"     << drawData.d_disableEffect
         << "' hidden='"      << int(drawData.d_hidden)
         << "'>\n";
    spaces += 2;
    d_os << spaces
         << "<rect x='"       << drawData.d_rectangle.d_ux
         <<     "' y='"       << drawData.d_rectangle.d_uy
         <<     "' width='"   << drawData.d_rectangle.d_width
         <<     "' height='"  << drawData.d_rectangle.d_height
         <<     "' border='"  << drawData.d_rectangle.d_borderThickness
         << "'/>\n";

    if (drawData.d_labelBounds.d_width > 0) {
        d_os << spaces
             << "<text x='"       << drawData.d_labelBounds.d_ux
             <<       "' y='"     << drawData.d_labelBounds.d_uy
             <<       "' width='" << drawData.d_labelBounds.d_width
             <<       "' height='"<< drawData.d_labelBounds.d_height
             <<       "' charSize='" << drawData.d_charSize;

        if (drawData.d_labelMark != DrawData::BulletMark::eNONE) {
            d_os << "' mark='"    << int(drawData.d_labelMark)
                 << "' left='"    << drawData.d_leftMark;
        }
        d_os << "'/>\n" << spaces << "<string>";
        outputXMLString(d_os, drawData.d_label);
        d_os << "</string>\n";
    }
    spaces -= 2;
    d_os << spaces << "</draw>\n";
    spaces -= 2;
    d_os << spaces << "</" << widgetName << ">\n";
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
    auto  thickness = drawData.d_rectangle.d_borderThickness;
    auto  width     = drawData.d_rectangle.d_width  - 2*thickness - 2;
    auto  height    = drawData.d_rectangle.d_height - 2*thickness - 2;
    auto  size      = count > 0 ? width/count : height;

    *textBounds = { width, height };
    *charSize   = size >= upperLimit ? upperLimit - 1 : size;
    return true;                                                      // RETURN
}

}  // namespace BDS
// vim: ts=4:sw=4:et:ai
