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

constexpr static const Wawt::Vertex kUPPER_LEFT   {-1.0_M,-1.0_M};
constexpr static const Wawt::Vertex kUPPER_CENTER { 0.0_M,-1.0_M};
constexpr static const Wawt::Vertex kUPPER_RIGHT  { 1.0_M,-1.0_M};
constexpr static const Wawt::Vertex kLOWER_LEFT   {-1.0_M, 1.0_M};
constexpr static const Wawt::Vertex kLOWER_CENTER { 0.0_M, 1.0_M};
constexpr static const Wawt::Vertex kLOWER_RIGHT  { 1.0_M, 1.0_M};
constexpr static const Wawt::Vertex kCENTER_LEFT  {-1.0_M, 0.0_M};
constexpr static const Wawt::Vertex kCENTER_CENTER{ 0.0_M, 0.0_M};
constexpr static const Wawt::Vertex kCENTER_RIGHT { 1.0_M, 0.0_M};

const char * const s_alignment[] = { "LEFT", "CENTER", "RIGHT" };

using FontIdMap  = std::map<Wawt::FontSizeGrp, uint16_t>;

WawtDump s_defaultAdapter(std::wcout);

inline
std::wostream& operator<<(std::wostream& os, const WawtDump::Indent indent) {
    if (indent.d_indent > 0) {
        os << std::setw(indent.d_indent) << L' ';
    }
    return os;                                                        // RETURN
}

Wawt::FocusCb eatMouseUp(int, int, bool) {
    return Wawt::FocusCb();                                            // RETURN
}

inline
int Int(double value) {
    return int(std::round(value));
}

bool findBox(const Wawt::DrawDirective **box,
             const Wawt::Panel&          panel,
             Wawt::WidgetId              id) {
    for (auto& nextWidget : panel.widgets()) {
        if (std::holds_alternative<Wawt::Panel>(nextWidget)) {
            auto& next = std::get<Wawt::Panel>(nextWidget);

            if (findBox(box, next, id)) {
                return true;                                          // RETURN
            }

            if (!next.d_widgetId.isSet() || id < next.d_widgetId) {
                return false;                                         // RETURN
            }

            if (id == next.d_widgetId) {
                *box = &next.adapterView();
                return true;                                          // RETURN
            }
        }
        else {
            auto& base
                = std::visit([](const Wawt::Base& r)-> const Wawt::Base& {
                                        return r;
                                    }, nextWidget);

            if (base.d_widgetId == id) {
                *box = &base.adapterView();
                return true;                                          // RETURN
            }

            if (base.d_widgetId > id) {
                return false;                                         // RETURN
            }
        }
    }
    return false;                                                     // RETURN
}

const Wawt::DrawDirective *getAdapterView(const Wawt::Panel& parent,
                                          const Wawt::Panel& root,
                                          Wawt::WidgetId     id) {
    const Wawt::DrawDirective *view = nullptr;

    if (id.isSet()) {
        if (id.isRelative()) {
            if (id.value() == UINT16_MAX) { // see: kPARENT
                view = &parent.adapterView();
            }
            else if (id.value() == UINT16_MAX-1) { // see: kROOT
                view = &root.adapterView();
            }
            else if (id.value() > 0) {
                uint16_t offset = id.value();

                for (auto const& nextWidget : parent.widgets()) {
                    if (--offset == 0) {
                        view = std::visit([](const Wawt::Base& r) {
                                             return &r.adapterView();
                                          }, nextWidget);
                        break;                                         // BREAK
                    }
                }
            }
        }
        else {
            findBox(&view, root, id);
        }
    }

    if (!view) {
        throw Wawt::Exception("Context of widget not found.", id);      // THROW
    }
    return view;                                                      // RETURN
}

bool handleChar(Wawt::Base            *base,
                Wawt::EnterFn         *enterFn,
                uint16_t              maxChars,
                wchar_t               pressed)
{
    auto text = base->textView().getText();
    bool hasCursor = !text.empty() && text.back() == Wawt::s_cursor;

    if (hasCursor) {
        text.pop_back();
        base->drawView().selected() = false;
    }

    if (pressed) {
        if (pressed == L'\b') {
            if (!text.empty()) {
                text.pop_back();
            }
        }
        else if (pressed == L'\r') {
            bool ret = (*enterFn) ? (*enterFn)(&text) : true;
            base->textView().setText(text);
            return ret;                                               // RETURN
        }
        else if (text.length() < maxChars) {
            text.push_back(pressed);
        }
        text.push_back(Wawt::s_cursor);
        base->drawView().selected() = true;
    }
    else if (!hasCursor) {
        text.push_back(Wawt::s_cursor);
        base->drawView().selected() = true;
    }
    base->textView().setText(text);
    return false;                                                     // RETURN
}

Wawt::DrawPosition makeAbsolute(const Wawt::Position& position,
                               const Wawt::Panel&    parent,
                               const Wawt::Panel&    root) {
    auto        view       = getAdapterView(parent, root, position.d_widgetId);
    auto&       upperLeft  = view->d_upperLeft;
    auto&       lowerRight = view->d_lowerRight;
    auto        thickness  = view->d_borderThickness;

    auto xorigin = (upperLeft.d_x  + lowerRight.d_x)/2.0;
    auto yorigin = (upperLeft.d_y  + lowerRight.d_y)/2.0;
    auto xradius = lowerRight.d_x - xorigin;
    auto yradius = lowerRight.d_y - yorigin;

    switch (position.d_endPoint.first) {
      case Wawt::eOUTER:  { // Already have this.
        } break;
      case Wawt::eOUTER1: {
            xradius += 1;
        } break;
      case Wawt::eINNER:  { // if border thickness == 1, then same as eX_OUTER
            xradius -= thickness - 1;
        } break;
      case Wawt::eINNER1: {
            xradius -= thickness;
        } break;
    };

    switch (position.d_endPoint.second) {
      case Wawt::eOUTER:  { // Already have this.
        } break;
      case Wawt::eOUTER1: {
            yradius += 1;
        } break;
      case Wawt::eINNER:  { // if border thickness == 1, then same as eX_OUTER
            yradius -= thickness - 1;
        } break;
      case Wawt::eINNER1: {
            yradius -= thickness;
        } break;
    };

    auto x = xorigin + position.d_vertex.d_sx*xradius;
    auto y = yorigin + position.d_vertex.d_sy*yradius;

    x += position.d_x;
    y += position.d_y;

    return Wawt::DrawPosition{x,y};                                 // RETURN
}

void refreshTextMetric(Wawt::DrawDirective            *args,
                       Wawt::TextBlock                *block,
                       Wawt::DrawAdapter              *adapter_p,
                       FontIdMap&                     fontIdToSize) {
    auto  id         = block->fontSizeGrp();
    auto  textHeight = args->interiorHeight();
    auto  fontSize   = id.has_value() ? fontIdToSize.find(*id)->second : 0;
    auto  charSize   = fontSize > 0   ? fontSize : textHeight;
    auto  iconSize   = args->d_bulletType != Wawt::BulletType::eNONE
                                      ? args->interiorHeight()
                                      : 0;

    if (charSize != args->d_charSize) {
        block->initTextMetricValues(args, adapter_p, charSize);

        if (fontSize == 0 && id.has_value()) {
            fontIdToSize[*id] = args->d_charSize;
        }
    }
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

void scaleAdapterParameters(Wawt::DrawDirective     *args,
                            const Wawt::Scale&       scale) {
    auto width             = args->width();
    auto height            = args->height();
    args->d_upperLeft.d_x *= scale.first;
    args->d_upperLeft.d_y *= scale.second;
    args->d_lowerRight.d_x = args->d_upperLeft.d_x + width  * scale.first  - 1;
    args->d_lowerRight.d_y = args->d_upperLeft.d_y + height * scale.second - 1;
}

double scaleBorder(double borderScale, double thickness) {
    auto scaled = borderScale * thickness;

    if (int(scaled) == 0 && thickness > 0.0) {
        scaled = 1.0; // should always show a border
    }
    return scaled;
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
                      const Wawt::Panel&        root,
                      const Wawt::Scale&        scale) {
    auto borderScale = std::min(scale.first, scale.second);

    args->d_borderThickness
        = layout->d_borderThickness
        = scaleBorder(borderScale, layout->d_borderThickness);

    layout->d_upperLeft.d_x  *= scale.first;
    layout->d_upperLeft.d_y  *= scale.second;
    layout->d_lowerRight.d_x *= scale.first;
    layout->d_lowerRight.d_y *= scale.second;

    args->d_upperLeft  = makeAbsolute(layout->d_upperLeft,  parent, root);
    args->d_lowerRight = makeAbsolute(layout->d_lowerRight, parent, root);

    // "tie" is used to create a square widgets.
    if (layout->d_tie == Wawt::TieScale::eUL_X) {
        auto ux     = args->d_upperLeft.d_x;
        auto uy     = args->d_upperLeft.d_y;
        auto offset = args->d_lowerRight.d_y - uy;
        args->d_lowerRight.d_x = ux + offset;
    }
    else if (layout->d_tie == Wawt::TieScale::eUL_Y) {
        auto ux     = args->d_upperLeft.d_x;
        auto uy     = args->d_upperLeft.d_y;
        auto offset = args->d_lowerRight.d_x - ux;
        args->d_lowerRight.d_y = uy + offset;
    }
    else if (layout->d_tie == Wawt::TieScale::eLR_X) {
        auto lx     = args->d_lowerRight.d_x;
        auto ly     = args->d_lowerRight.d_y;
        auto offset = args->d_upperLeft.d_y - ly;
        args->d_upperLeft.d_x = lx + offset;
    }
    else if (layout->d_tie == Wawt::TieScale::eLR_Y) {
        auto lx     = args->d_lowerRight.d_x;
        auto ly     = args->d_lowerRight.d_y;
        auto offset = args->d_upperLeft.d_x - lx;
        args->d_upperLeft.d_y = ly + offset;
    }
    else if (layout->d_tie == Wawt::TieScale::eCC_X) {
        auto h      = args->height();
        auto w      = args->width();
        auto offset = (h - w)/2.0;
        args->d_upperLeft.d_x  -= offset;
        args->d_lowerRight.d_x += offset;
    }
    else if (layout->d_tie == Wawt::TieScale::eCC_Y) {
        auto h      = args->height();
        auto w      = args->width();
        auto offset = (w - h)/2.0;
        args->d_upperLeft.d_y  -= offset;
        args->d_lowerRight.d_y += offset;
    }
}

} // end unnamed namespace

                            //-----------------
                            // class  Wawt::Base
                            //-----------------

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

Wawt::ButtonBar::ButtonBar(Layout&&                          layout,
                          OptInt                            borderThickness,
                          std::initializer_list<Button>     buttons)
: Base(std::move(layout),
       InputHandler().defaultAction(ActionType::eCLICK),
       TextString(),
       DrawOptions())
, d_buttons(buttons)
{
    for (auto& btn : d_buttons) {
        btn.d_input.defaultAction(ActionType::eCLICK);
        btn.d_layout.d_borderThickness = borderThickness.value_or(-1.0);
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


                            //-------------------
                            // class  Wawt::Canvas
                            //-------------------

                            //---------------------------
                            // class  Wawt::DrawOptions
                            //---------------------------

bool
Wawt::DrawOptions::draw(DrawAdapter *adapter, const std::wstring& text) const
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
    bool finalvalue = d_type == ActionType::eCLICK
                        ? false
                        : (d_type == ActionType::eTOGGLE ? !previous : true);

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
    bool finalvalue = d_type == ActionType::eCLICK
                        ? false
                        : (d_type == ActionType::eTOGGLE ? !previous : true);

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
                 maxChars = p3->second] (wchar_t c) {
                    return handleChar(text, enterFn, maxChars, c);
                };                                                    // RETURN
    }
    return Wawt::FocusCb();                                            // RETURN
}

bool
Wawt::InputHandler::contains(int x, int y, const Base *base) const
{
    return x >= base->d_draw.d_upperLeft.d_x
        && x <= base->d_draw.d_lowerRight.d_x
        && y >= base->d_draw.d_upperLeft.d_y
        && y <= base->d_draw.d_lowerRight.d_y;                         // RETURN
}

bool
Wawt::InputHandler::textcontains(int x, int y, const Text *text) const
{
    if (d_type == ActionType::eENTRY
     || text->d_layout.d_borderThickness > 0) {
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
        assert(d_type != ActionType::eINVALID);

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
                        maxChars = p5->second] (wchar_t c) {
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
        assert(d_type != ActionType::eINVALID);

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

                            //----------------------
                            // class  Wawt::TextBlock
                            //----------------------

void
Wawt::TextBlock::initTextMetricValues(Wawt::DrawDirective      *args,
                                     Wawt::DrawAdapter        *adapter,
                                     uint16_t                 upperLimit)
{
    int  width  = args->interiorWidth();
    int  height = args->interiorHeight();

    if (args->d_bulletType != Wawt::BulletType::eNONE) {
        width -= height;
    }
    d_metrics.d_textWidth  = width  - 2;  // these are upper limits
    d_metrics.d_textHeight = height - 2;  // 1 pixel spacing around border.

    auto charSizeLimit
        = (upperLimit > 0 && upperLimit < height) ? upperLimit+1 : height;

    adapter->getTextMetrics(args,
                            &d_metrics,
                            d_block.d_string,
                            charSizeLimit);
    return;                                                           // RETURN
}
void
Wawt::TextBlock::setText(TextId id)
{
    d_block.d_id = id;

    if (id != kNOID) {
        d_block.d_string.clear();
    }
}

void
Wawt::TextBlock::setText(std::wstring string)
{
    d_block.d_string = std::move(string);
    d_block.d_id     = kNOID;
}

void
Wawt::TextBlock::setText(const TextMapper& mappingFn)
{
    if (d_block.d_id != kNOID && mappingFn) {
        d_block.d_string = mappingFn(d_block.d_id);
    }
}

                            //----------------------
                            // class  Wawt::TextEntry
                            //----------------------


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

Wawt::List::List(Layout&&                          layout,
                FontSizeGrp                       fontSizeGrp,
                DrawOptions&&                  options,
                ListType                          listType,
                Labels                            labels,
                const GroupCb&                    click,
                Panel                            *root)
: Base(std::move(layout),
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

Wawt::List::List(Layout&&                          layout,
                FontSizeGrp                       fontSizeGrp,
                DrawOptions&&                  options,
                ListType                          listType,
                unsigned int                      rows,
                const GroupCb&                    click,
                Panel                            *root)
: Base(std::move(layout),
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

Wawt::List&
Wawt::List::operator=(const List& rhs)
{
    if (this != &rhs) {
        d_buttons       = rhs.d_buttons;
        d_root          = rhs.d_root;
        d_rows          = rhs.d_rows;
        d_startRow      = rhs.d_startRow;
        d_buttonClick   = rhs.d_buttonClick;
        d_type          = rhs.d_type;
        d_fontSizeGrp   = rhs.d_fontSizeGrp;
        d_rowHeight     = rhs.d_rowHeight;
        Base::operator=(rhs);

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
    Canvas canvas({d_root->d_layout.d_upperLeft,d_root->d_layout.d_lowerRight},
                  PaintFn(),
                  {clickCb});
    canvas.d_draw.d_upperLeft  = d_root->d_draw.d_upperLeft;
    canvas.d_draw.d_lowerRight = d_root->d_draw.d_lowerRight;
    canvas.d_draw.d_tracking   = std::make_tuple(kCANVAS, nextId.value(), -1);
    canvas.d_widgetId          = Wawt_Id::inc(nextId);
    widgets.emplace_back(canvas);

    auto& dropDown   = std::get<List>(widgets.emplace_back(*this));
    auto  dropDownId = nextId.value();
    dropDown.d_draw.d_tracking    = std::make_tuple(kLIST, dropDownId, -1);
    dropDown.d_widgetId           = Wawt_Id::inc(nextId);
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

    auto ul = dropDown.adapterView().d_upperLeft;
    auto lr = dropDown.adapterView().d_lowerRight;

    // The parent has changed so redefine the relative positions
    // so it lies below the visible list (for resizing).
    int y = lr.d_y - ul.d_y;
    dropDown.d_layout.d_upperLeft  = { kUPPER_LEFT,  d_widgetId };
    dropDown.d_layout.d_lowerRight = { kLOWER_RIGHT, d_widgetId, 0, y };

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
                         .textView().setText(std::wstring(1, s_downArrow)
                                             + L' '
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
    button.d_layout.d_borderThickness = 1; // for click box - do not scale
    button.d_draw.d_bulletType        = BulletType::eNONE;

    switch (d_type) {
        case ListType::eCHECKLIST:   {
            button.d_layout.d_borderThickness = 0;
            button.d_input.d_type             = ActionType::eTOGGLE;
            button.d_input.d_disabled         = false;
            button.d_text.alignment()         = Align::eLEFT;
            button.d_draw.d_bulletType        = BulletType::eCHECK;
        } break;
        case ListType::eRADIOLIST:   {
            button.d_layout.d_borderThickness = 0;
            button.d_input.d_type             = ActionType::eBULLET;
            button.d_input.d_disabled         = false;
            button.d_text.alignment()         = Align::eLEFT;
            button.d_draw.d_bulletType        = BulletType::eRADIO;
        } break;
        case ListType::ePICKLIST:    {
            button.d_input.d_type             = ActionType::eTOGGLE;
            button.d_input.d_disabled         = false;
            button.d_text.alignment()         = Align::eCENTER;
        } break;
        case ListType::eSELECTLIST:  {
            button.d_input.d_type             = ActionType::eCLICK;
            button.d_input.d_disabled         = false;
            button.d_text.alignment()         = Align::eCENTER;
        } break;
        case ListType::eVIEWLIST:    {
            button.d_input.d_type             = ActionType::eCLICK;
            button.d_input.d_disabled         = true;
            button.d_text.alignment()         = Align::eLEFT;
        } break;
        case ListType::eDROPDOWNLIST:{
            button.d_input.d_type             = ActionType::eCLICK;
            button.d_input.d_disabled         = !lastButton;
            button.d_text.alignment()         = Align::eCENTER;
            button.d_draw.d_hidden            = !lastButton;
        } break;
        default: abort();
    };
    return;                                                           // RETURN
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

    auto y       = d_draw.d_upperLeft.d_y + d_draw.d_borderThickness;
    auto rows    = 0u;

    for (auto& button : d_buttons) {
        button.d_draw.d_upperLeft.d_x   = left_x;
        button.d_draw.d_lowerRight.d_x  = right_x;

        button.d_draw.d_upperLeft.d_y   = y;
        button.d_draw.d_lowerRight.d_y  = y + d_rowHeight;

        // The List's button borders are not scaled.
        button.d_draw.d_borderThickness = button.d_layout.d_borderThickness;

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

                                //-----------
                                // class  Wawt
                                //-----------

// PRIVATE CLASS METHODS
void
Wawt::scalePosition(Panel::Widget *widget, const Scale& scale, double border)
{
    auto index = widget->index();
    switch (index) {
        case kCANVAS: { // Canvas
            auto& canvas = std::get<Canvas>(*widget);
            canvas.d_draw.d_borderThickness
                = scaleBorder(border, canvas.d_layout.d_borderThickness);
            scaleAdapterParameters(&canvas.d_draw, scale);
        } break;                                                       // BREAK
        case kTEXTENTRY: { // TextEntry
            auto& entry = std::get<TextEntry>(*widget);
            entry.d_draw.d_borderThickness
                = scaleBorder(border, entry.d_layout.d_borderThickness);
            scaleAdapterParameters(&entry.d_draw, scale);
        } break;                                                       // BREAK
        case kLABEL: { // Label
            auto& label = std::get<Label>(*widget);
            label.d_draw.d_borderThickness
                = scaleBorder(border, label.d_layout.d_borderThickness);
            scaleAdapterParameters(&label.d_draw, scale);
        } break;                                                       // BREAK
        case kBUTTON: { // Button
            auto& btn = std::get<Button>(*widget);
            btn.d_draw.d_borderThickness
                = scaleBorder(border, btn.d_layout.d_borderThickness);
            scaleAdapterParameters(&btn.d_draw, scale);
        } break;                                                       // BREAK
        case kBUTTONBAR: { // ButtonBar
            auto& bar   = std::get<ButtonBar>(*widget);
            bar.d_draw.d_borderThickness
                = scaleBorder(border, bar.d_layout.d_borderThickness);
            scaleAdapterParameters(&bar.d_draw, scale);

            for (auto& btn : bar.d_buttons) {
                btn.d_draw.d_borderThickness
                    = scaleBorder(border, btn.d_layout.d_borderThickness);
                scaleAdapterParameters(&btn.d_draw, scale);
            }
        } break;                                                       // BREAK
        case kLIST: { // List
            auto& list = std::get<List>(*widget);
            list.d_rowHeight *= scale.second;
            list.d_draw.d_borderThickness
                = scaleBorder(border, list.d_layout.d_borderThickness);
            scaleAdapterParameters(&list.d_draw, scale);
            list.d_rowHeight = double(list.d_draw.interiorHeight())
                                                         / list.windowSize();
            list.setButtonPositions(); // List buttons have no borders.
        } break;                                                       // BREAK
        case kPANEL: { // Panel
            auto& panel = std::get<Wawt::Panel>(*widget);
            panel.d_draw.d_borderThickness
                = scaleBorder(border, panel.d_layout.d_borderThickness);
            scaleAdapterParameters(&panel.d_draw, scale);

            for (auto& nextWidget : panel.d_widgets) {
                scalePosition(&nextWidget, scale, border);
            }
        } break;                                                       // BREAK
        default: abort();
    }
    return;                                                           // RETURN
}

void
Wawt::setIds(Panel::Widget *widget, Wawt::WidgetId& id)
{
    auto index = widget->index();
    switch (index) {
        case kCANVAS: { // Canvas
            auto& canvas = std::get<Canvas>(*widget);
            canvas.d_widgetId        = Wawt_Id::inc(id);
            canvas.d_draw.d_tracking = {index, canvas.d_widgetId.value(), -1};
        } break;                                                   // BREAK
        case kTEXTENTRY: { // TextEntry
            auto& entry = std::get<TextEntry>(*widget);
            entry.d_widgetId        = Wawt_Id::inc(id);
            entry.d_draw.d_tracking = {index, entry.d_widgetId.value(), -1};
        } break;                                                   // BREAK
        case kLABEL: { // Label
            auto& label = std::get<Label>(*widget);
            label.d_widgetId        = Wawt_Id::inc(id);
            label.d_draw.d_tracking = {index, label.d_widgetId.value(), -1};
        } break;                                                   // BREAK
        case kBUTTON: { // Button
            auto& btn = std::get<Button>(*widget);
            btn.d_widgetId        = Wawt_Id::inc(id);
            btn.d_draw.d_tracking = {index, btn.d_widgetId.value(), -1};
        } break;                                                   // BREAK
        case kBUTTONBAR: { // ButtonBar
            auto& bar   = std::get<ButtonBar>(*widget);
            auto  row = 0;
            bar.d_widgetId        = Wawt_Id::inc(id);
            bar.d_draw.d_tracking = {index, bar.d_widgetId.value(), -1};

            for (auto& btn : bar.d_buttons) {
                btn.d_draw.d_tracking = {index, bar.d_widgetId.value(), row++};
            }
        } break;                                                   // BREAK
        case kLIST: { // List
            auto& list = std::get<List>(*widget);
            auto  row = 0;
            list.d_widgetId        = Wawt_Id::inc(id);
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
            panel.d_widgetId        = Wawt_Id::inc(id);
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
                               const Scale&                    scale,
                               const BorderThicknessDefaults&  border,
                               const WidgetOptionDefaults&     option)
{
    switch (widget->index()) {
        case kCANVAS: { // Canvas
            auto& canvas = std::get<Wawt::Canvas>(*widget);
            auto& base   = static_cast<Wawt::Base&>(canvas);
            auto& layout = base.d_layout;
            auto& draw   = base.d_draw;

            if (layout.d_borderThickness < 0) {
                layout.d_borderThickness = double(border.d_canvasThickness);
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_canvasOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root, scale);

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

            if (layout.d_borderThickness < 0) {
                layout.d_borderThickness = double(border.d_textEntryThickness);
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_textEntryOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root, scale);

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

            if (layout.d_borderThickness < 0) {
                layout.d_borderThickness = double(border.d_labelThickness);
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_labelOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root, scale);

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

            if (layout.d_borderThickness < 0) {
                layout.d_borderThickness = double(border.d_buttonThickness);
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_buttonOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root, scale);

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

            if (layout.d_borderThickness < 0) {
                layout.d_borderThickness =  double(border.d_buttonBarThickness);
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_buttonBarOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root, scale);

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
            auto  borderScale = std::min(scale.first, scale.second);

            auto thickness = bar.d_buttons.front().d_layout.d_borderThickness;

            if (thickness < 0) {
                thickness = double(border.d_buttonThickness);
            }

            thickness   = scaleBorder(borderScale, thickness);

            auto  overhead    = 2.*view.d_borderThickness;

            if (overhead + 2*thickness+4         > view.height()
             || overhead + count*(2*thickness+4) > view.width()) {
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
                buttonView.d_borderThickness = thickness;
                buttonView.d_upperLeft       = upperLeft;
                buttonView.d_lowerRight      = lowerRight;
                count                       -= 1;

                button.d_layout.d_borderThickness = thickness;

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

            if (layout.d_borderThickness < 0) {
                layout.d_borderThickness
                    = double(usePanel ? border.d_panelThickness
                                      : border.d_listThickness);
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = usePanel ? option.d_panelOptions
                                          : option.d_listOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root, scale);

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

            if (layout.d_borderThickness < 0) {
                layout.d_borderThickness = double(border.d_panelThickness);
            }

            if (!draw.d_options.has_value()) {
                draw.d_options = option.d_panelOptions;
            }

            setAdapterValues(&draw, &layout, panel, *root, scale);

            if (!next.adapterView().verify()) {
                throw Wawt::Exception("'Panel' corners are inverted.",
                                     next.d_widgetId);                 // THROW
            }

            for (auto& nextWidget : next.d_widgets) {
                setWidgetAdapterPositions(&nextWidget,
                                          root,
                                          next,
                                          scale,
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
    Panel container({list.d_layout.d_upperLeft, list.d_layout.d_lowerRight});
    auto  border = unsigned(std::round(list.d_layout.d_borderThickness));

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
    Button    scrollUp({{},{}, border, TieScale::eUL_X},
                       {scrollUpCb},
                       {std::wstring(1, s_upArrow)},
                       {options});
    Button    scrollDown({{},{}, border, TieScale::eLR_X},
                         {scrollDownCb},
                         {std::wstring(1, s_downArrow)},
                         {options});

    auto rowHeight  = 2.0/double(list.windowSize()); // as a scale factor
    auto scale      = Metric(1.0 - rowHeight);
    auto any_M      = Metric(0); // value is just a placeholder

    if (buttonsOnLeft) {
        scrollDown.d_layout.d_tie        = TieScale::eUL_X;

        scrollUp.d_layout.d_upperLeft    = Position(kUPPER_LEFT);
        scrollUp.d_layout.d_lowerRight   = Position({ any_M,-scale});

        scrollDown.d_layout.d_upperLeft  = Position({-1.0_M, scale});
        scrollDown.d_layout.d_lowerRight = Position({ any_M, 1.0_M});

        list.d_layout.d_upperLeft        = Position(kUPPER_RIGHT,
                                           {eINNER,eOUTER},
                                           1_wr);
        list.d_layout.d_lowerRight       = Position(kLOWER_RIGHT); // panel
    }
    else {
        scrollUp.d_layout.d_tie          = TieScale::eLR_X;

        scrollUp.d_layout.d_upperLeft    = Position({ any_M, -1.0_M});
        scrollUp.d_layout.d_lowerRight   = Position({ 1.0_M, -scale});

        scrollDown.d_layout.d_upperLeft  = Position({ any_M,  scale});
        scrollDown.d_layout.d_lowerRight = Position(kLOWER_RIGHT);

        list.d_layout.d_upperLeft        = Position(kUPPER_LEFT); // panel
        list.d_layout.d_lowerRight       = Position(kLOWER_LEFT,
                                           {eINNER,eOUTER},
                                           2_wr);
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
    Wawt_Id::inc(id);
    auto& down  = list->d_root->lookup<Wawt::Button>(id, "Scroll down button");
    list->setStartingRow(row, &up, &down);
    return;                                                           // RETURN
}

// PUBLIC DATA MEMBERS
const Wawt::WidgetId   Wawt::kPARENT(UINT16_MAX, true, true);
const Wawt::WidgetId   Wawt::kROOT(UINT16_MAX-1, true, true);

const std::any        Wawt::s_noOptions;

wchar_t Wawt::s_downArrow = L'v';
wchar_t Wawt::s_upArrow   = L'^';
wchar_t Wawt::s_cursor    = L'|';

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

void
Wawt::popUpModalDialogBox(Panel *root, Panel&& dialogBox)
{
    auto& layout = dialogBox.d_layout;

    if (layout.d_upperLeft.d_x != 0 || layout.d_upperLeft.d_y != 0) {
        throw
            Exception("Dialog 'Panel' upper left offsets are NOT zero.");
    }
    auto   nextId  = root->d_widgetId;
    auto&  widgets = root->d_widgets;
    Canvas canvas({root->d_layout.d_upperLeft, root->d_layout.d_lowerRight},
                  PaintFn(),
                  {OnClickCb()}); // transparent
    canvas.d_widgetId          = Wawt_Id::inc(nextId);
    canvas.d_draw.d_upperLeft  = root->d_draw.d_upperLeft;
    canvas.d_draw.d_lowerRight = root->d_draw.d_lowerRight;
    widgets.emplace_back(canvas);

    Scale scale(1.0, 1.0);

    if (layout.d_lowerRight.d_x > 0) {
        scale.first = root->d_draw.width()/layout.d_lowerRight.d_x;
    }

    if (layout.d_lowerRight.d_y > 0) {
        scale.second = root->d_draw.height()/layout.d_lowerRight.d_y;
    }

    auto& dialog = widgets.emplace_back(std::move(dialogBox));
    setIds(&dialog, nextId);
    root->d_widgetId = nextId;

    setWidgetAdapterPositions(&dialog,
                              root,
                              *root,
                              scale,
                              d_borderDefaults,
                              d_optionDefaults);

    auto& panel = std::get<Panel>(dialog);
    setTextAndFontValues(&panel);
    refreshTextMetrics(&panel); // since font size entries may have changed.
    return;                                                           // RETURN
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
                                  d_fontIdToSize);
            } break;                                                   // BREAK
            case kLABEL: { // Label
                auto& label = std::get<Label>(widget);
                refreshTextMetric(&label.d_draw,
                                  &label.d_text,
                                  d_adapter_p,
                                  d_fontIdToSize);
            } break;                                                   // BREAK
            case kBUTTON: { // Button
                auto& button = std::get<Button>(widget);
                refreshTextMetric(&button.d_draw,
                                  &button.d_text,
                                  d_adapter_p,
                                  d_fontIdToSize);
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
                                      d_fontIdToSize);
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
                                      d_fontIdToSize);
                }
            } break;                                                   // BREAK
            case kLIST: { // List
                auto& list = std::get<List>(widget);
                for (auto& button : list.d_buttons) {
                    refreshTextMetric(&button.d_draw,
                                      &button.d_text,
                                      d_adapter_p,
                                      d_fontIdToSize);
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

    if (root->d_layout.d_upperLeft.d_x != 0
     || root->d_layout.d_upperLeft.d_y != 0) {
        throw
            Exception("Root 'Panel' upper left offsets are NOT zero.");// THROW
    }
    auto baseWidth  = root->d_layout.d_lowerRight.d_x + 1;
    auto baseHeight = root->d_layout.d_lowerRight.d_y + 1;

    if (baseWidth < 1 || baseHeight < 1) {
        throw                                                          // THROW
            Exception("Root 'Panel' lower right offsets are bad (not set?).");
    }

    if (!root->drawView().options().has_value()) {
         root->drawView().options() = d_optionDefaults.d_screenOptions;
    }
    // Optimization: instead of interpreting the layout on each call, do it
    // when the new size is about 25% larger than the old size (improves
    // drag resizing performance).
    bool interpretLayout = root->d_draw.width() == 1
                        || std::abs(width-baseWidth)/baseWidth    > 0.25
                        || std::abs(height-baseHeight)/baseHeight > 0.25;

    if (interpretLayout) {
        root->d_draw.d_upperLeft                = {0, 0};
        root->d_draw.d_lowerRight.d_x           = width  - 1;
        root->d_draw.d_lowerRight.d_y           = height - 1;
        root->d_draw.d_borderThickness          = 0.0;
        root->d_layout.d_lowerRight.d_x         = width  - 1;
        root->d_layout.d_lowerRight.d_y         = height - 1;
        root->d_layout.d_borderThickness        = 0.0;

        Scale scale{ width/baseWidth, height/baseHeight };

        for (auto& widget : root->d_widgets) {
            setWidgetAdapterPositions(&widget,
                                      root,
                                      *root,
                                      scale,
                                      d_borderDefaults,
                                      d_optionDefaults);
        }
    }
    auto  sizex  = root->d_draw.width();
    auto  sizey  = root->d_draw.height();
    Scale scale{ width/sizex, height/sizey };

    bool needRescale = scale.first != 1.0 || scale.second != 1.0;

    if (needRescale) {
        root->d_draw.d_lowerRight.d_x = width-1;
        root->d_draw.d_lowerRight.d_y = height-1;

        auto borderScale = std::min(width/(root->d_layout.d_lowerRight.d_x+1),
                                    height/(root->d_layout.d_lowerRight.d_y+1));

        for (auto& nextWidget : root->d_widgets) {
            scalePosition(&nextWidget, scale, borderScale);
        }
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
              const std::wstring&        text)
{
    auto [type, id, row] = widget.d_tracking;
    d_dumpOs << std::boolalpha << d_indent << L"<Widget id=" << type
             << L"," << id;

    if (row >= 0) {
        d_dumpOs  << L"," << row;
    }
    d_dumpOs  << L"' borderThickness='"    << widget.d_borderThickness
              << L"' greyEffect='"         << widget.d_greyEffect
              << L"' options='"            << widget.d_options.has_value()
              << L"'>\n";
    d_indent += 2;
    d_dumpOs  << d_indent
              << L"<UpperLeft x='"         << widget.d_upperLeft.d_x
              << L"' y='"                  << widget.d_upperLeft.d_y
              << L"'/>\n"
              << d_indent
              << L"<LowerRight x='"        << widget.d_lowerRight.d_x
              << L"' y='"                  << widget.d_lowerRight.d_y
              << L"'/>\n";

    if (widget.d_charSize > 0) {
        //std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        d_dumpOs  << d_indent
                  << L"<Text startx='"     << Int(widget.d_startx)
                  << L"' selected='"       << widget.d_selected;

        if (widget.d_bulletType      == Wawt::BulletType::eCHECK) {
            d_dumpOs  << L"' bulletType='Check";
        }
        else if (widget.d_bulletType == Wawt::BulletType::eRADIO) {
            d_dumpOs  << L"' bulletType='Radio";
        }
        d_dumpOs  << L"'>\n";
        d_indent += 2;
        d_dumpOs  << d_indent
                  << L"<String charSize='" << widget.d_charSize
                  << L"'>"
                  << text
                  << L"</String>\n";
        d_indent -= 2;
        d_dumpOs  << d_indent
                  << L"</Text>\n";
    }
    d_indent -= 2;
    d_dumpOs  << d_indent << L"</Widget>\n" << std::noboolalpha;
}

void
WawtDump::getTextMetrics(Wawt::DrawDirective   *parameters,
                        Wawt::TextMetrics     *metrics,
                        const std::wstring&   text,
                        double                upperLimit)
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
