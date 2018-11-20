/** @file scrolledlist.cpp
 *  @brief Implements a list that handles variable number of elements.
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

#include "wawt/drawprotocol.h"
#include "wawt/scrolledlist.h"
#include "wawt/wawtenv.h"
#include "wawt/widget.h"

#include <iostream>

#include <iterator>

#ifdef WAWT_WIDECHAR
#define S(str) String_t(L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

using namespace Wawt::literals;

namespace Wawt {

namespace {

constexpr float kSPACING = 4.0f;

inline
void removeSuffix(StringView_t& view, std::size_t count)
{
    while (count-- > 0) {
        popBackChar(view);
    }
    return;                                                           // RETURN
}

bool adjustView(Text::Data&         view,
                DrawProtocol       *adapter,
                const Bounds&       bounds,
                const std::any&     options) {
    auto rowView = view.view();
    assert(!rowView.empty());

    if (!adapter->getTextValues(view, bounds, 0, options)) {
        // Failed to find bounding box size for the character size. 
        view.d_view           = StringView_t(S(""));
        return false;
    }
    assert(bounds.d_height > view.d_bounds.d_height);

    if (bounds.d_width < view.d_bounds.d_width) {
        auto length   = rowView.length();
        auto fitCount = 0u;             // always less then 'length'
        auto tryCount = (length + 1)/2; // always less then 'length'
        auto attempt  = view;

        while (fitCount < tryCount) {
            auto tmp = rowView;
            removeSuffix(tmp, length - tryCount);
            attempt.d_view = tmp;
            adapter->getTextValues(attempt, bounds, 0, options);

            if (bounds.d_width < attempt.d_bounds.d_width) {
                tryCount = (tryCount + fitCount) / 2; // ignore overflow
            }
            else {
                view.d_bounds         = attempt.d_bounds;
                fitCount = tryCount;
                tryCount = (length + fitCount) / 2;
            }
        }
        auto tmp = rowView;
        removeSuffix(tmp, length - fitCount);
        view.d_view = tmp;
    }
    return true;                                                      // RETURN
}

inline float yorigin(const Layout::Result& layout) {
    return layout.d_upperLeft.d_y + layout.d_border + kSPACING/2.0 + 1;
}

} // unnamed namespace

                                //-----------
                                // class List
                                //-----------


// PRIVATE METHODS
void
ScrolledList::draw(Widget *widget, DrawProtocol *adapter) noexcept
{
    Widget::defaultDraw(widget, adapter);

    if (d_rowSize > 0 && !d_rows.empty()) {
        if (d_windowView.empty()) {
            synchronizeView(adapter);
        }
        auto& layout   = widget->layoutData();
        auto& button   = widget->children()[0];
        auto  settings = widget->settings();
        auto  bounds   = layout.d_bounds.d_width - 2*(layout.d_border+1);
        auto  row      = Text::Data{};

        settings.d_optionName = WawtEnv::sItem;
        settings.d_options    = d_itemOptions;

        row.d_charSize      = uint16_t(5*d_rowSize/6);

        if (d_scrollbarsOnLeft && !button.isHidden()) {
            row.d_upperLeft.d_x = button.layoutData().d_upperLeft.d_x
                                + button.layoutData().d_bounds.d_width
                                + 1;
        }
        else {
            row.d_upperLeft.d_x = layout.d_upperLeft.d_x + layout.d_border + 1;
        }

        if (!button.isHidden()) {
            bounds -= button.layoutData().d_bounds.d_width;
        }
        auto y = yorigin(layout);

        settings.d_selected = true;

        for (auto& index : d_selectedSet) {
            auto box = Layout::Result(row.d_upperLeft.d_x,
                                      y + index*(d_rowSize + kSPACING) - 1,
                                      bounds,
                                      d_rowSize + kSPACING,
                                      0);
            adapter->draw(box, settings);
        }
        row.d_upperLeft.d_y = y-1;
        row.d_baselineAlign = true;

        for (auto [view, selected, width] : d_windowView) {
            if (!view.empty()) {
                auto align           = float{};

                if (d_alignment != TextAlign::eLEFT) {
                    align = bounds - width;

                    if (d_alignment == TextAlign::eCENTER) {
                        align /= 2.0;
                    }
                }
                settings.d_selected  = selected;
                
                row.d_view           = view;
                row.d_upperLeft.d_x += align;
                adapter->draw(row, settings);
                row.d_upperLeft.d_x -= align;
            }
            row.d_upperLeft.d_y += d_rowSize + kSPACING;
        }
    }
    return;                                                           // RETURN
}

Widget::DownEventMethod
ScrolledList::makeScroll(int delta) noexcept
{
    return
        [delta] (double, double, Widget *, Widget *list) -> EventUpCb {
            auto cb = EventUpCb();

            if (list->tracker()) {
                cb= [list, delta](double x, double y, bool up) -> void {
                        if (up && list->inside(x, y)) {
                            auto me = static_cast<ScrolledList*>(
                                                            list->tracker());

                            if (me) {
                                me->scroll(delta);
                            }
                        }
                    };
            }
            return cb;
        };                                                            // RETURN
}

void
ScrolledList::upEvent(double, double y, Widget *widget) noexcept
{
    auto& layout   = widget->layoutData();
    auto  ul_y     = yorigin(layout);
    auto  row      = uint16_t((y - ul_y) / (d_rowSize + kSPACING));

    if (row < d_windowView.size()) {
        if (d_singleSelect) {
            clearSelection();
        }
        d_lastRowClicked = d_top;
        std::advance(d_lastRowClicked.value(), row);

        std::get<1>(d_windowView[row])   ^= true;
        d_lastRowClicked.value()->second ^= true; // non-single select: toggle

        if (d_lastRowClicked.value()->second) {
            d_selectedSet.insert(row);
            d_selectCount += 1;
        }
        else {
            d_selectedSet.erase(row);
            d_selectCount -= 1;
        }

        if (d_clickCb) {
            d_clickCb(d_lastRowClicked.value());
        }
    }
    return;                                                           // RETURN
}

// PUBLIC METHODS
ScrolledList::ScrolledList(uint16_t       visibleRowCount,
                           TextAlign      alignment,
                           bool           scrollbarsOnLeft,
                           bool           alwaysShowScrollbars) noexcept
: d_windowSize(visibleRowCount == 0 ? 1 : visibleRowCount)
, d_alignment(alignment)
, d_scrollbarsOnLeft(scrollbarsOnLeft)
, d_alwaysShowScrollbars(alwaysShowScrollbars)
, d_itemOptions(WawtEnv::defaultOptions(WawtEnv::sItem))
{
}

void
ScrolledList::clear() noexcept
{
    d_rows.clear();
    d_windowView.clear();
    d_selectedSet.clear();
    d_top    = d_rows.begin();
    d_topPos = 0;
    d_lastRowClicked.reset();

    for (auto& child : d_widget->children()) {
        child.hidden(true);
    }
}

void
ScrolledList::clearSelection() noexcept
{
    if (d_singleSelect) {
        if (d_lastRowClicked.has_value()) {
            d_lastRowClicked.value()->second = false;
        }
    }
    else {
        for (auto& item : d_rows) {
            if (item.second) {
                item.second = false;
            }
        }
    }

    for (auto& item : d_windowView) {
        std::get<2>(item) = false;
    }
    d_lastRowClicked.reset();
    d_selectedSet.clear();
    d_selectCount = 0;
    return;                                                           // RETURN
}

Widget
ScrolledList::widget() noexcept
{
    auto rowHeight  = 2.0/d_windowSize;
    auto upOneRow   = Widget(WawtEnv::sButton, {})
                        .downEventMethod(makeScroll(-1))
                        .textMark(Text::BulletMark::eUPARROW);
                        
    auto downOneRow = Widget(WawtEnv::sButton, {})
                        .downEventMethod(makeScroll(1))
                        .textMark(Text::BulletMark::eDOWNARROW);
    
    if (d_scrollbarsOnLeft) {
        auto offset = -1.0 + rowHeight;

          upOneRow.layout({{  -1.0,   -1.0},
                           {offset, offset},
                           Layout::Vertex::eUPPER_LEFT,
                           0});
        downOneRow.layout({{  -1.0,-offset},
                           {offset,    1.0},
                           Layout::Vertex::eLOWER_LEFT,
                           0});
    }
    else {
        auto offset = 1.0 - rowHeight;

          upOneRow.layout({{offset,   -1.0},
                           {   1.0,-offset},
                           Layout::Vertex::eUPPER_RIGHT,
                           0});
        downOneRow.layout({{offset, offset},
                           {   1.0,    1.0},
                           Layout::Vertex::eLOWER_RIGHT,
                           0});
    }
    // Scrollbar consists of two buttons with a scroll box sandwiched
    // in between.  The layout of the component pieces (below) is adjusted
    // on each call to 'synchronizeView()'.  Note that the initial layout
    // below is appropriate for an empty list (zero height page buttons).
    auto upOnePg    = Widget(WawtEnv::sButton,
                             {{-1.0, 1.0, 0_wr},
                              { 1.0, 1.0, 0_wr}, // Y modified in sync. view.
                              0})
                        .downEventMethod(makeScroll(-d_windowSize));
                        
    auto downOnePg  = Widget(WawtEnv::sButton,
                             {{-1.0,-1.0, 1_wr}, // Y modified in sync. view.
                              { 1.0,-1.0, 1_wr},
                              0})
                        .downEventMethod(makeScroll(d_windowSize));
                        
    auto scrollBox  = Widget(WawtEnv::sScrollbox,
                             {{-1.0, 1.0, 2_wr}, { 1.0,-1.0, 3_wr}, 0})
                        .downEventMethod( // eat down events
                                [] (double, double, Widget*, Widget*) {
                                    return [](double, double, bool) { };
                                });
                               
    return Widget(WawtEnv::sList, *this, Layout())
            .addChild(std::move(upOneRow))      // RID: 0
            .addChild(std::move(downOneRow))    // RID: 1
            .addChild(std::move(upOnePg))       // RID: 2
            .addChild(std::move(downOnePg))     // RID: 3
            .addChild(std::move(scrollBox))     // RID: 4
            .downEventMethod(
                [](double, double, Widget* list, Widget*) -> EventUpCb {
                    auto cb = EventUpCb();

                    if (list->tracker()) {
                        cb= [list](double x, double y, bool up) -> void {
                                if (up && list->inside(x, y)) {
                                    auto me = list->tracker();

                                    if (me) {
                                        static_cast<ScrolledList*>(me)
                                                        ->upEvent(x, y, list);
                                    }
                                }
                            };
                    }
                    return cb;
                })
            .drawMethod(
                [](Widget *list, DrawProtocol *adapter) {
                    auto me = static_cast<ScrolledList*>(list->tracker());

                    if (me) {
                        me->draw(list, adapter);
                    }
                })
            .layoutMethod(
                [](Widget        *list,
                   const Widget&  parent,
                   bool           firstPass,
                   DrawProtocol  *adapter) -> void {
                    auto me = static_cast<ScrolledList*>(list->tracker());

                    if (me) {
                        if (firstPass) {
                            Widget::defaultLayout(list, parent, true, adapter);
                            auto& data   = list->layoutData();
                            auto  window = me->viewSize();
                            auto  size   = (data.d_bounds.d_height
                                                     - 2.*data.d_border
                                                     - kSPACING)/window
                                            - kSPACING;
                            if (size < 4) {
                                me->d_rowSize  = 0;
                            }
                            else {
                                me->d_rowSize  = size;
                            }
                        }
                        else {
                            me->synchronizeView(adapter);
                        }
                    }
                });                                                   // RETURN
}

ScrolledList&
ScrolledList::singleSelectList(bool value) noexcept
{
    if (value != d_singleSelect) {
        if (!d_singleSelect) { // from multi-select to single select
            if (d_lastRowClicked.has_value()
             && !d_lastRowClicked.value()->second) { // was toggled off
                d_lastRowClicked.reset();
            }

            for (auto& item : d_rows) {
                if (item.second) {
                    item.second = false;
                }
            }

            if (d_lastRowClicked.has_value()) {
                d_lastRowClicked.value()->second = true;
                d_selectCount = 1;
            }
            else {
                d_selectCount = 0;
            }
        }
        d_singleSelect = value;
    }
    return *this;                                                     // RETURN
}

void
ScrolledList::scroll(int delta) noexcept
{
    while (delta < 0) {
        if (d_top == d_rows.begin()) {
            break;
        }
        d_topPos -= 1;
        --d_top;
        ++delta;
    }

    while (delta > 0) {
        if (++d_top == d_rows.end()) {
            --d_top;
            break;
        }
        d_topPos += 1;
        --delta;
    }

    d_windowView.clear();
    d_selectedSet.clear();
}

void
ScrolledList::synchronizeView(DrawProtocol *adapter) noexcept
{
    assert(d_widget);
    assert(d_widget->screen() != nullptr);
    assert(d_widget->children().size() == 5);

    d_windowView.clear();
    d_selectedSet.clear();

    auto hide = (d_rows.size() <= d_windowSize) && !d_alwaysShowScrollbars;

    for (auto& child : d_widget->children()) {
        child.hidden(hide);
    }
    auto& layout     = d_widget->layoutData();
    auto& button     = d_widget->children()[0];
    auto  text       = Text::Data();
    auto  margin     = 2*(layout.d_border + 1);

    if (hide) {
        d_top    = d_rows.begin();
        d_topPos = 0;
    }
    else {
        margin += button.layoutData().d_bounds.d_width;
    }

    if (d_rowSize  > 0 && layout.d_bounds.d_width > margin+2*d_rowSize ) {
        auto  height = float(d_rowSize  + 2);
        auto  bounds = Bounds{layout.d_bounds.d_width - margin, height};

        text.d_baselineAlign = true;

        for (auto row  = d_top;
                  row != d_rows.end() && d_windowView.size()<d_windowSize;
                ++row) {
            text.d_view         = StringView_t(row->first);
            text.d_charSize     = uint16_t(5*d_rowSize/6);

            if (!text.view().empty()) {
                adjustView(text, adapter, bounds, d_itemOptions);
            }

            if (row->second) {
                d_selectedSet.insert(d_windowView.size());
            }
            d_windowView.emplace_back(text.view(),
                                      row->second,
                                      text.d_bounds.d_width);
        }
    }
    auto& upOneRow   = d_widget->children()[0];
    auto& downOneRow = d_widget->children()[1];
    auto& upOnePg    = d_widget->children()[2];
    auto& downOnePg  = d_widget->children()[3];
    auto& scrollBox  = d_widget->children()[4];

    if (d_rows.size() <= d_windowSize) {
        upOnePg.layout().d_lowerRight.d_sY   = 1.0;
        downOnePg.layout().d_lowerRight.d_sY = -1.0;
    }
    else {
        auto  height     = downOneRow.layoutData().d_upperLeft.d_y
                         - upOnePg.layoutData().d_upperLeft.d_y;
        auto  fullBar    = 2*height / upOneRow.layoutData().d_bounds.d_height;

        upOnePg.layout().d_lowerRight.d_sY
            =  1.0 + double(d_topPos)/d_rows.size() * fullBar;
        downOnePg.layout().d_upperLeft.d_sY
            = -1.0 - (1.0 - double(d_topPos+d_windowView.size())/d_rows.size())
                                                    * fullBar;
    }
    // Re-layout the scrollbar with new layout values.
    Widget::defaultLayout(&upOnePg, *d_widget, true, adapter);
    Widget::defaultLayout(&downOnePg, *d_widget, true, adapter);
    Widget::defaultLayout(&scrollBox, *d_widget, true, adapter);
    return;                                                           // RETURN
}

ScrolledList::ItemIter
ScrolledList::top() const noexcept
{
    return d_top;
}

void
ScrolledList::top(ItemIter topItem) noexcept
{
    if (d_rows.empty()) {
        d_topPos = 0;
        d_top    = d_rows.begin();
    }
    else if (topItem == d_rows.end()) {
        d_top    = --topItem;
        d_topPos = std::distance(d_rows.begin(), d_top);
    }
    else {
        d_top    = topItem;
        d_topPos = std::distance(d_rows.begin(), d_top);
    }
    d_windowView.clear();
    d_selectedSet.clear();
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
