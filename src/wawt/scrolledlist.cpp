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

constexpr float kSPACING = 3.0f;

bool adjustView(Text::Data&         view,
                DrawProtocol       *adapter,
                const Bounds&       bounds,
                const std::any&     options) {

    if (!adapter->getTextValues(view, bounds, 0, options)) {
        // Failed to find bounding box size for the character size. 
        view.d_view           = StringView_t{};
        view.d_baselineOffset = 0;
        return false;
    }
    assert(bounds.d_height > view.d_bounds.d_height);

    if (bounds.d_width < view.d_bounds.d_width) {
        auto length   = view.view().length();
        auto fitCount = 0u;
        auto tryCount = (length + 1)/2;
        auto attempt  = view;

        while (fitCount < tryCount) {
            auto tmp = view.view();
            tmp.remove_suffix(length - tryCount);
            attempt.d_view = tmp;
            adapter->getTextValues(attempt, bounds, 0, options);

            if (bounds.d_width < view.d_bounds.d_width) {
                tryCount = (tryCount + fitCount) / 2;
            }
            else {
                view.d_baselineOffset = attempt.d_baselineOffset;
                fitCount = tryCount;
                tryCount = (length + fitCount) / 2;
            }
        }
        auto tmp = view.view();
        tmp.remove_suffix(length - fitCount);
        view.d_view   = tmp;
        view.d_bounds = attempt.d_bounds;
    }
    return true;
}

Widget::DownEventMethod makeScroll(int delta) {
    return
        [delta] (double, double, Widget *, Widget *list) -> EventUpCb {
            auto cb = EventUpCb();

            if (list->tracker()) {
                cb= [list, delta](double x, double y, bool up) -> void {
                        if (up && list->inside(x, y)) {
                            auto me = static_cast<ScrolledList*>(
                                                            list->tracker());

                            if (me) {
                                auto iTop = me->top() + delta;

                                if (iTop < 0) {
                                    me->top(0);
                                }
                                else if (uint16_t(iTop) >= me->rows().size()) {
                                    me->top(me->rows().size()-1);
                                }
                                else {
                                    me->top(uint16_t(iTop));
                                }
                                me->synchronizeView();
                            }
                        }
                    };
            }
            return cb;
        };
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

    if (d_charSize > 0 && !d_rows.empty()) {
        if (d_windowView.empty()) {
            synchronizeView(adapter);
        }
        auto& layout   = widget->layoutData();
        auto  row      = Text::Data{};
        auto& button   = widget->children()[0];
        auto  settings = widget->settings();
        auto  bounds   = layout.d_bounds.d_width - 2*(layout.d_border+1);

        settings.d_optionName = WawtEnv::sItem;
        settings.d_options    = d_itemOptions;

        row.d_charSize      = d_charSize;
        row.d_upperLeft.d_y = layout.d_upperLeft.d_y
                            + layout.d_border
                            + kSPACING;

        if (d_scrollbarsOnLeft && !button.isHidden()) {
            row.d_upperLeft.d_x = button.layoutData().d_upperLeft.d_x
                                + button.layoutData().d_bounds.d_width
                                + 1;
        }
        else {
            row.d_upperLeft.d_x = layout.d_upperLeft.d_x + layout.d_border + 1;
        }

        for (auto [view, baselineOffset, selected, width] : d_windowView) {
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
                row.d_baselineOffset = baselineOffset;
                row.d_upperLeft.d_x += align;
                adapter->draw(row, settings);
                row.d_upperLeft.d_x -= align;
            }
            row.d_upperLeft.d_y += d_charSize + kSPACING;
        }
    }
}

void
ScrolledList::upEvent(double x, double y, Widget *widget) noexcept
{
    // The scrolled list is the parent of the scroll buttons, and overlays
    // them.  So before determining which row in the view window is being
    // selected; a check for scrolling must be done (if the buttons aren't
    // hidden).
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

Widget
ScrolledList::list(const Layout&          layout,
                   const SelectChange&    selectCb,
                   bool                   singleSelect) noexcept
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
                             {{-1.0, 1.0, 2_wr}, { 1.0,-1.0, 3_wr}, 0});
                               
    return Widget(WawtEnv::sList, *this, layout)
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
                                me->d_charSize = 0;
                            }
                            else {
                                me->d_charSize = uint16_t(size);
                            }
                        }
                        else {
                            me->synchronizeView(adapter);
                        }
                    }
                });
}

void
ScrolledList::synchronizeView(DrawProtocol *adapter) noexcept
{
    assert(d_widget->screen() != nullptr);

    if (d_top >= d_rows.size()) {
        d_top = 0;
    }
    // TBD: adjust scrollbox
    d_windowView.clear();

    if (d_charSize > 0 && adapter != nullptr) {
        assert(d_widget->children().size() == 5);
        auto hide = (d_rows.size() <= d_windowSize) && !d_alwaysShowScrollbars;

        for (auto& child : d_widget->children()) {
            child.hidden(hide);
        }
        auto& layout     = d_widget->layoutData();
        auto& button     = d_widget->children()[0];
        auto  text       = Text::Data();
        auto  margin     = 2*(layout.d_border + 1);

        if (!hide) {
            margin += button.layoutData().d_bounds.d_width;
        }

        if (layout.d_bounds.d_width > margin+2*d_charSize) {
            auto  height = float(d_charSize + 2);
            auto  bounds = Bounds{layout.d_bounds.d_width - margin, height};

            for (auto row = d_top;
                      row < d_rows.size() && row - d_top < d_windowSize;
                    ++row) {
                auto& item          = d_rows[row];
                text.d_view         = item.first;
                text.d_charSize     = d_charSize;

                if (!text.view().empty()) {
                    adjustView(text, adapter, bounds, d_itemOptions);
                }
                d_windowView.emplace_back(text.view(),
                                          uint16_t(text.d_baselineOffset),
                                          item.second,
                                          text.d_bounds.d_width);
            }
        }
    }
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
