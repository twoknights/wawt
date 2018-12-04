/** @file dropdownlist.cpp
 *  @brief Factory for text entry widgets and associated support.
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
#include "wawt/dropdownlist.h"
#include "wawt/wawtenv.h"

#include <algorithm>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <iostream>

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

constexpr float kSPACING = 2.0f;

} // unnamed namespace

                            //-------------------
                            // class DropDownList
                            //-------------------


// PRIVATE METHODS
void
DropDownList::popUpDropDown(Widget *dropDown)
{
    auto root = dropDown->screen();
    auto id   = root->widgetIdValue();
    auto relativeId
              = root->hasChildren() ? root->children().back().relativeId() + 1
                                    : 0;
    // First push a transparent panel which, when clicked on, discards
    // the drop-down widget.
    auto soak = Widget(WawtEnv::sPanel, Layout(root->layout()))
                 .drawMethod(
                    [](Widget *me, DrawProtocol *adapter) {
                        me->synchronizeTextView(true);
                        me->resolveLayout(adapter, true,  *me->screen());
                        me->resolveLayout(adapter, false, *me->screen());
                    })
                 .downEventMethod(
                    [root, id](auto, auto, auto, auto) {
                        return
                            [root, id](double, double, bool up) {
                                if (up) {
                                    root->children().pop_back();
                                    root->widgetIdValue() = id;
                                }
                                return;
                            };
                    });
    auto screenBox       = root->layoutData();
    soak.layoutData()    = screenBox;
    auto& screen         = root->children().emplace_back(std::move(soak));
    auto  newChildMethod = root->newChildMethod();
    
    if (newChildMethod) {
        newChildMethod(root, &screen);
    }
    // 'dropDown' is a text widget holding the current selection.
    // It's layout data is not usable in the copy to be made, but its draw
    // data is corret, and can be used to generate the scrolled list layout.

    auto  width  = screenBox.d_bounds.d_width  - 2*screenBox.d_border;
    auto  height = screenBox.d_bounds.d_height - 2*screenBox.d_border;
    auto& bounds = dropDown->layoutData().d_bounds;
    auto  border = 2*dropDown->layoutData().d_border + kSPACING;
    auto  texty  = dropDown->text().d_data.d_bounds.d_height + kSPACING;
                    
    auto  lines  = std::size_t((d_maxHeight*height-border) / texty);
    auto  ux     = dropDown->layoutData().d_upperLeft.d_x;
    auto  uy     = dropDown->layoutData().d_upperLeft.d_y + bounds.d_height;
    auto  lx     = ux + bounds.d_width;
    auto  ly     = uy + std::min(lines, rows().size()) * texty + border;

    if (ly > height) {
        ly = height;
    }
    d_list.clearSelection();
    d_list.top(d_list.rows().begin());
    d_list.onItemClick([this, root, id](ScrolledList*, ItemIter row) -> void {
                            d_selectedRow = row;
                            root->children().pop_back();
                            root->widgetIdValue() = id;
                       });

    auto  widget = d_list.widget();
    widget.layout().d_upperLeft.d_sX         = -1.0 + 2.0 * ux / width;
    widget.layout().d_upperLeft.d_sY         = -1.0 + 2.0 * uy / height;
    widget.layout().d_upperLeft.d_widgetRef  = WidgetId::kPARENT;
    widget.layout().d_lowerRight.d_sX        = -1.0 + 2.0 * lx / width;
    widget.layout().d_lowerRight.d_sY        = -1.0 + 2.0 * ly / height;
    widget.layout().d_lowerRight.d_widgetRef = WidgetId::kPARENT;
    widget.layout().d_thickness              = dropDown->layout().d_thickness;

    screen.addChild(std::move(widget));

    root->widgetIdValue()
        = screen.assignWidgetIds(id,
                                 relativeId,
                                 dropDown->text().d_layout.d_charSizeMap,
                                 root);
    return;                                                           // RETURN
}

void
DropDownList::draw(Widget *widget, DrawProtocol *adapter) noexcept
{
    auto  box    = widget->layoutData();
    auto  text   = widget->text().d_data;
    auto& layout = widget->text().d_layout;

    adapter->draw(box, widget->settings());

    if (d_selectedRow.has_value()) {
        text.d_view   = (**d_selectedRow)->d_view.d_viewFn();
    }
    else {
        text.d_view   = S("");
    }

    if (text.resolveSizes(box,
                           0,
                           adapter,
                           widget->settings().d_options)) {
        text.d_upperLeft = layout.position(text.d_bounds, box);
        adapter->draw(text, widget->settings());
    }
    return;                                                           // RETURN
}

void
DropDownList::serialize(std::ostream&  os,
                        std::string   *closeTag,
                        const Widget&  entry,
                        unsigned int   indent) noexcept
{
    Widget::defaultSerialize(os, closeTag, entry, indent);

    std::string spaces(indent+2, ' ');
    d_list.serialize(os, closeTag, Widget(nullptr, Layout()), indent);

    os << spaces << "<maxHeight value='" << d_maxHeight << "'/>\n" ;
    return;                                                           // RETURN
}

// PUBLIC METHODS

DropDownList::DropDownList(double              maxHeight,
                           uint16_t            minCharactersToShow,
                           bool                scrollbarsOnLeft) noexcept
: d_list(minCharactersToShow, TextAlign::eCENTER, scrollbarsOnLeft, false)
, d_maxHeight(maxHeight)
, d_selectedRow()
{
}

DropDownList::DropDownList(double              maxHeight,
                           Initializer         items,
                           bool                scrollbarsOnLeft) noexcept
: d_list(items, TextAlign::eCENTER, scrollbarsOnLeft, false)
, d_maxHeight(maxHeight)
, d_selectedRow(d_list.lastRowClicked())
{
}

Widget
DropDownList::widget() noexcept
{
    auto layoutView = Text::View_t();

    if (d_list.d_layoutString.empty()) {
        auto count = 0u;

        for (auto& item : rows()) {
            auto& view = item->d_view;

            if (view.d_viewFn().length() > count) {
                count = view.d_viewFn().length();
            }
        }
        layoutView = Text::View_t(String_t(count, C('X')) + S("|"));
    }
    else {
        layoutView = Text::View_t(d_list.d_layoutString);
    }

    return
        Widget(WawtEnv::sList, *this, Layout())
            .text(std::move(layoutView))
            .textMark(Text::BulletMark::eDOWNARROW)
            .verticalAlign(TextAlign::eBASELINE)
            .downEventMethod(
                [](double, double, Widget *me, Widget*) {
                    me->selected(true);
                    return  [me](double x, double y, bool up) {
                        if (up) {
                            me->selected(false);
                            if (me->inside(x, y)) {
                                static_cast<DropDownList*>(me->tracker())
                                                           ->popUpDropDown(me);
                            }
                        }
                        return;
                    };
                })
            .drawMethod(
                [](Widget *me, DrawProtocol *adapter) {
                    auto list = static_cast<DropDownList*>(me->tracker());
                    if (list) {
                        list->draw(me, adapter);
                    }
                })
            .serializeMethod(
                [](std::ostream& os, std::string *closeTag,
                   const Widget& me, unsigned int indent) {
                    auto *list = static_cast<DropDownList*>(me.tracker());
                    if (list) {
                        list->serialize(os, closeTag, me, indent);
                    }
                });                                                   // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
