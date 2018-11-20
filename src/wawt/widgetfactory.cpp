/** @file widgetfactory.cpp
 *  @brief Factory methods to create different widget "classes".
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

#include "wawt/widgetfactory.h"
#include "wawt/wawtenv.h"

#include <memory>
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

void addDropDown(Widget             *dropDown,
                 Widget             *select,
                 const GroupClickCb& selectCb)
{
    auto root = dropDown->screen();
    auto id   = root->widgetIdValue();
    auto relativeId
              = root->hasChildren() ? root->children().back().relativeId() + 1
                                    : 0;
    // First push a transparent panel which, when clicked on, discards
    // the drop-down widget.
    auto soak = panel(Layout(root->layout()))
                 .downEventMethod([root, id](auto, auto, auto, auto) {
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
    // 'dropDown' contains the fixed sized list that will be the drop down.
    // It's layout data is not usable in the copy to be made, but its draw
    // data is corret, and can be used to generate the layout.
    auto& list = screen.children()
                       .emplace_back(dropDown->children()[1].clone());
    list.hidden(false).disabled(false);
    
    auto ux    = list.layoutData().d_upperLeft.d_x;
    auto uy    = list.layoutData().d_upperLeft.d_y;
    auto lx    = ux + list.layoutData().d_bounds.d_width;
    auto ly    = uy + list.layoutData().d_bounds.d_height;

    auto width = screenBox.d_bounds.d_width  - 2*screenBox.d_border;
    auto height= screenBox.d_bounds.d_height - 2*screenBox.d_border;

    list.layout().d_upperLeft.d_sX         = -1.0 + 2.0 * ux / width;
    list.layout().d_upperLeft.d_sY         = -1.0 + 2.0 * uy / height;
    list.layout().d_upperLeft.d_widgetRef  = WidgetId::kPARENT;
    list.layout().d_lowerRight.d_sX        = -1.0 + 2.0 * lx / width;
    list.layout().d_lowerRight.d_sY        = -1.0 + 2.0 * ly / height;
    list.layout().d_lowerRight.d_widgetRef = WidgetId::kPARENT;

    for (auto& item : list.children()) {
        // item layout is relative to list so they require no adjustments.
        item.downEventMethod( // on click copy label to drop-down clean-up etc.
            [cb=selectCb,select,id,root](auto, auto, Widget *widget, auto)
            {
                widget->selected(true);
                return
                    [cb,select,widget,id,root](double x, double y, bool up)
                    {
                        if (up) {
                            widget->selected(false);

                            if (widget->inside(x, y)) {
                                select->resetLabel(widget->text()
                                                         .d_layout
                                                         .d_viewFn);
                                auto lclselect = select;
                                auto lclcb     = std::move(cb);
                                auto rid       = widget->settings()
                                                        .d_relativeId;
                                root->widgetIdValue() = id;
                                // The "pop" destroys the lambda captures, so
                                // only the local variables can be used
                                // afterwords.
                                root->children().pop_back();

                                if (lclcb) {
                                    lclcb(lclselect, rid);
                                }
                            }
                        }
                        return;
                    };
            });
    }

    root->widgetIdValue()
        = screen.assignWidgetIds(id,
                                 relativeId,
                                 dropDown->text().d_layout.d_charSizeMap,
                                 root);
    return;                                                           // RETURN
}

Widget::DownEventMethod createDropDown(GroupClickCb&& selectCb)
{
    return
        [cb=std::move(selectCb)] (auto, auto, auto *widget, auto *parent) {
            widget->selected(true);
            return  [cb, widget, parent](double x, double y, bool up) {
                if (up) {
                    widget->selected(false);
                    if (widget->inside(x, y)) {
                        addDropDown(parent, widget, cb);
                    }
                }
                return;
            };
        };                                                            // RETURN
}

using BoundsPtr = std::shared_ptr<Bounds>;

Widget::LayoutMethod genSpacedLayout(const BoundsPtr&   bounds,
                                     int                rows,
                                     int                columns,
                                     TextAlign          horizontalAlign,
                                     TextAlign          verticalAlign
                                                        = TextAlign::eBASELINE)
{
    return
        [=] (Widget                 *widget,
             const Widget&           parent,
             bool                    firstPass,
             DrawProtocol           *adapter) -> void {
            Widget::defaultLayout(widget, parent, firstPass, adapter);

            auto& data     = widget->text().d_data;
            auto& settings = widget->settings();
            auto& layout   = widget->layoutData();

            if (!settings.d_successfulLayout) {
                return;
            }

            if (firstPass) {
                if (settings.d_relativeId == 0 // ignore old values
                 || data.d_bounds.d_width > bounds->d_width) {
                    bounds->d_width = data.d_bounds.d_width;
                }

                if (settings.d_relativeId == 0
                 || data.d_bounds.d_height > bounds->d_height) {
                    bounds->d_height = data.d_bounds.d_height;
                }
                return;
            }
            // 'bounds' gives the text box dimensions. The border thickness
            // is unchanged, so:
            auto border = layout.d_border;
            auto width  = bounds->d_width  + 2*(border + 1);
            auto height = bounds->d_height + 2*(border + 1);

            // Now placing the widget requires the panels dimensions to
            // calculate the border margin and spacing between buttons.
            auto panelWidth   = parent.layoutData().d_bounds.d_width;
            auto panelHeight  = parent.layoutData().d_bounds.d_height;

            // Calculate separator spacing between buttons:
            auto spacex = columns == 1
                ? 0
                : (panelWidth - columns*width)/(columns - 1);
            auto spacey = rows == 1
                ? 0
                : (panelHeight - rows*height)/(rows - 1);

            // But don't seperate them by too much:
            if (spacex > width/2) {
                spacex = width/2;
            }

            if (spacey > height/2) {
                spacey = height/2;
            }
            // ... which might mean the panel needs margins:
            auto marginx = (panelWidth - columns*width - (columns-1)*spacex)/2;
            auto marginy = (panelHeight- rows * height -    (rows-1)*spacey)/2;
            auto c       = settings.d_relativeId % columns;
            auto r       = settings.d_relativeId / columns;
            
            // Translate the button at row 'r' and column 'c'
            auto& position      = widget->layoutData().d_upperLeft;
            auto& extent        = widget->layoutData().d_bounds;
            extent.d_width      = width;
            extent.d_height     = height;
            position.d_x        = parent.layoutData().d_upperLeft.d_x
                                        + marginx + c * (spacex+width);
            position.d_y        = parent.layoutData().d_upperLeft.d_y
                                        + marginy + r * (spacey+height);
            auto& labelPosition = data.d_upperLeft;
            auto& labelBounds   = data.d_bounds;
            labelPosition.d_y   = position.d_y + border + 1;
            labelPosition.d_x   = position.d_x + border + 1;

            if (horizontalAlign != TextAlign::eLEFT) {
                auto space = width - labelBounds.d_width - 2*(border+1);

                if (horizontalAlign == TextAlign::eCENTER) {
                    space /= 2.0;
                }
                labelPosition.d_x += space;
            }

            if (verticalAlign != TextAlign::eTOP
             && verticalAlign != TextAlign::eBASELINE) {
                auto space = height - labelBounds.d_height - 2*(border+1);

                if (verticalAlign == TextAlign::eCENTER) {
                    space /= 2.0;
                }
                labelPosition.d_y += space;
            }
        };                                                            // RETURN
}

Widget::DownEventMethod makePushButtonDownMethod(OnClickCb && onClick)
{
    return
        [cb=std::move(onClick)] (auto, auto, auto *widget, auto) -> EventUpCb {
            widget->selected(true);
            return  [cb, widget](double x, double y, bool up) -> void {
                if (up) {
                    widget->selected(false);
                    if (cb) {
                        if (widget->inside(x, y)) {
                            cb(widget);
                        }
                    }
                }
                return;
            };
        };                                                            // RETURN
}

Widget::DownEventMethod makeToggleButtonDownMethod(const GroupClickCb& cb)
{
    return
        [cb] (auto, auto, auto *widget, auto *parent) -> EventUpCb {
            return  [cb, widget, parent](double x, double y, bool up) -> void {
                if (up && widget->inside(x, y)) {
                    widget->selected(!widget->isSelected());
                    if (cb) {
                        cb(parent, widget->relativeId());
                    }
                }
                return;
            };
        };                                                            // RETURN
}

Widget::DownEventMethod makeRadioButtonDownMethod(const GroupClickCb& cb)
{
    return
        [cb] (auto, auto, auto *widget, auto *parent) -> EventUpCb {
            return  [cb, widget, parent](double x, double y, bool up) -> void {
                if (up && widget->inside(x, y)) {
                    if (!widget->isSelected()) {
                        for (auto& child : parent->children()) {
                            child.selected(false);
                        }
                        widget->selected(true);

                        if (cb) {
                            return cb(parent, widget->relativeId());
                        }
                    }
                }
                return;
            };
        };                                                            // RETURN
}

} // unnamed namespace

void adjacentTextLayout(Widget        *widget,
                        const Widget&  parent,
                        bool           firstPass,
                        DrawProtocol  *adapter,
                        TextAlign      alignment) noexcept
{
    assert(widget->hasChildren());
    assert(widget->children().front().hasText());

    auto  layout      = firstPass ? widget->layout().resolveLayout(parent)
                                  : widget->layoutData();
    auto& firstChild  = widget->children().front();
    auto& text        = firstChild.text();
    auto  adjustment  = layout.d_border+1;
    auto  width       = layout.d_bounds.d_width - 2*adjustment;
    auto  values      = Text::Data();
    values.d_charSize = text.d_layout.upperLimit(layout) - 1u;

    if (firstPass) {
        widget->layoutData()      = layout;
        layout.d_upperLeft.d_x   += adjustment;
        layout.d_upperLeft.d_y   += adjustment;
        layout.d_bounds.d_width  -= 2*adjustment;
        layout.d_bounds.d_height -= 2*adjustment;
        layout.d_border           = 0;

        auto  concat  = String_t{};
        concat.reserve(64);

        for (auto& child : widget->children()) {
            assert(child.hasText());
            child.layoutData() = layout;
            concat += child.text().d_data.view();
            child.settings().d_successfulLayout = true;
        }
        auto upperLimit = values.d_charSize + 1;
        values.d_view   = StringView_t(concat);

        if (!values.resolveSizes(layout, upperLimit, adapter, std::any())) {
            for (auto& child : widget->children()) {
                child.settings().d_successfulLayout = false;
            }
            return;                                                   // RETURN
        }
        values.d_view = StringView_t();
    }

    if (firstChild.settings().d_successfulLayout) {
        // The bounds calculated above is only a lower limit, and on the
        // second pass 'charSize' may have changed. Compute actual bounds
        // using the options assigned to each child:
        auto concatWidth = float{};

        do {
            (*text.d_layout.d_charSizeMap)[*text.d_layout.d_charSizeGroup]
                = values.d_charSize;
            concatWidth = 0;

            for (auto& child : widget->children()) {
                if (child.text().resolveLayout(child.layoutData(),
                                               adapter,
                                               child.options())) {
                    auto& childData = child.text().d_data;
                    concatWidth += childData.d_bounds.d_width;
                }
            }
        } while (concatWidth > width && --values.d_charSize > 2);

        if (values.d_charSize <= 2) {
            for (auto& child : widget->children()) {
                child.settings().d_successfulLayout = false;
            }
        }
        else if (!firstPass) {
            // Each child's layout was the full width of the concatenated
            // text.  Now their width must be set to that required by the
            // text fragment they contain, and the corner x position set
            // to the position it must be in.
            auto  xpos    = firstChild.layoutData().d_upperLeft.d_x;

            if (alignment != TextAlign::eLEFT) {
                auto space = firstChild.layoutData().d_bounds.d_width
                                                             - concatWidth;

                if (alignment == TextAlign::eCENTER) {
                    space /= 2.0f;
                }
                xpos += space;
            }
         
            for (auto& child : widget->children()) {
                auto& childLayout            = child.layoutData();
                auto& childData              = child.text().d_data;
                auto& childWidth             = childData.d_bounds.d_width;

                childData.d_upperLeft.d_x    = xpos;
                childData.d_bounds.d_width   = childWidth;
                childLayout.d_upperLeft.d_x  = xpos;
                childLayout.d_bounds.d_width = childWidth;
                xpos                        += childWidth + 1;
            }
        }
    }
    return;                                                           // RETURN
}

Widget canvas(Trackee&&                        tracker,
              const Layout&                    layout,
              Widget::DrawMethod&&             customDraw,
              Widget::DownEventMethod&&        onClick)
{
    return  Widget(WawtEnv::sCanvas, std::move(tracker), layout)
            .downEventMethod(std::move(onClick))
            .drawMethod(std::move(customDraw));
}

Widget canvas(const Layout&                    layout,
              Widget::DrawMethod&&             customDraw,
              Widget::DownEventMethod&&        onClick)
{
    return  Widget(WawtEnv::sCanvas, layout)
            .downEventMethod(std::move(onClick))
            .drawMethod(std::move(customDraw));
}

Widget checkBox(Trackee&&               tracker,
                const Layout&           layout,
                Text::View_t&&          string,
                CharSizeGroup           group,
                TextAlign               alignment)
{
    return  Widget(WawtEnv::sItem, std::move(tracker), layout)
            .downEventMethod(makeToggleButtonDownMethod(GroupClickCb()))
            .useTextBounds(layout.d_thickness <= 0.0)
            .text(std::move(string))
            .charSizeGroup(group)
            .horizontalAlign(alignment)
            .textMark(Text::BulletMark::eSQUARE,
                      alignment != TextAlign::eRIGHT);                // RETURN
}

Widget checkBox(const Layout&           layout,
                Text::View_t&&          string,
                CharSizeGroup           group,
                TextAlign               alignment)
{
    return  Widget(WawtEnv::sItem, layout)
            .downEventMethod(makeToggleButtonDownMethod(GroupClickCb()))
            .useTextBounds(layout.d_thickness <= 0.0)
            .text(std::move(string))
            .charSizeGroup(group)
            .horizontalAlign(alignment)
            .textMark(Text::BulletMark::eSQUARE,
                      alignment != TextAlign::eRIGHT);                // RETURN
}

Widget dialogBox(Trackee&&                     tracker,
                 const Layout&                 dialogLayout,
                 Widget&&                      buttons,
                 LabelGroupList                dialog)
{
    auto lineLayout = gridLayoutGenerator(-1.0, dialog.size()+3, 1);
    auto modal = Widget(WawtEnv::sDialog, std::move(tracker), dialogLayout);

    for (auto line : dialog) {
        auto nextChild = label(lineLayout(),
                               std::move(line.d_view),
                               line.d_group);

        if (modal.hasChildren()) {
            modal.addChild(std::move(nextChild));
        }
        else {
            // The first line in the dialog will have twice the height:
            auto layout = lineLayout();
            nextChild.layout().d_lowerRight = layout.d_lowerRight;
            modal.addChild(std::move(nextChild));
        }
    }
    lineLayout(); // skip a line
    modal.addChild(std::move(buttons).layout(lineLayout()));
    return modal;                                                     // RETURN
}

Widget dialogBox(const Layout&                 dialogLayout,
                 Widget&&                      buttons,
                 LabelGroupList                dialog)
{
    return dialogBox(Trackee(), dialogLayout, std::move(buttons), dialog);
}

Widget dropDownList(Trackee&&                  tracker,
                    Layout                     layout,
                    GroupClickCb&&             selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels)
{
    // A drop-down has to (like a dialog box) be appended to the children
    // list of the root window when it's "selection box" is clicked, so it
    // will overlay other widgets when drawn.
    // It must be a child of a transparent panel that covers the entire screen,
    // and prevents down click events from being propagated to any other widget
    // if it isn't on the drop-down list proper (instead the drop-down
    // [and its parent] are removed).
    using namespace Wawt::literals;
    auto selectLayout = gridLayoutGenerator(-1.0, labels.size()+1, 1);
    auto parent
        = panel(std::move(tracker), layout.border(0.0))
            .addChild(Widget(WawtEnv::sList, selectLayout())
                        .downEventMethod(createDropDown(std::move(selectCb)))
                        .text(S(""))
                        .charSizeGroup(group)
                        .horizontalAlign(TextAlign::eLEFT)
                        .textMark(Text::BulletMark::eDOWNARROW))
            .addChild(fixedSizeList({{-1.0,1.0,0_wr},{1.0,1.0}},
                                    true,
                                    GroupClickCb(),
                                    group,
                                    labels).disabled(true).hidden(true));
    return parent;                                                    // RETURN
}

Widget dropDownList(const Layout&              listLayout,
                    GroupClickCb&&             selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels)
{
    return dropDownList(Trackee(), listLayout, std::move(selectCb),
                        group, labels);
}

Widget fixedSizeList(Trackee&&               tracker,
                     const Layout&           listLayout,
                     bool                    singleSelect,
                     const GroupClickCb&     selectCb,
                     CharSizeGroup           group,
                     LabelList               labels)
{
    auto childLayout = gridLayoutGenerator(-1.0, labels.size(), 1);
    auto list        = Widget(WawtEnv::sList, std::move(tracker), listLayout);

    for (auto label : labels) {
        list.addChild(
            Widget(WawtEnv::sItem, std::move(tracker), childLayout())
                .downEventMethod(singleSelect
                    ? makeRadioButtonDownMethod(selectCb)
                    : makeToggleButtonDownMethod(selectCb))
                .text(std::move(label))
                .charSizeGroup(group)
                .horizontalAlign(TextAlign::eCENTER));

    }
    return list;                                                      // RETURN
}

Widget fixedSizeList(const Layout&           listLayout,
                     bool                    singleSelect,
                     const GroupClickCb&     selectCb,
                     CharSizeGroup           group,
                     LabelList               labels)
{
    return fixedSizeList(Trackee(), listLayout, singleSelect,
                         selectCb, group, labels);
}

Widget label(Trackee&&                  tracker,
             const Layout&              layout,
             Text::View_t&&             string,
             CharSizeGroup              group,
             TextAlign                  alignment)
{
    return  Widget(WawtEnv::sLabel, std::move(tracker), layout)
            .text(std::move(string))
            .charSizeGroup(group)
            .verticalAlign(TextAlign::eBASELINE)
            .horizontalAlign(alignment);                              // RETURN
}

Widget label(const Layout&              layout,
             Text::View_t&&             string,
             CharSizeGroup              group,
             TextAlign                  alignment)
{
    return  Widget(WawtEnv::sLabel, layout)
            .text(std::move(string))
            .charSizeGroup(group)
            .verticalAlign(TextAlign::eBASELINE)
            .horizontalAlign(alignment);                              // RETURN
}

Widget label(Trackee&&                  tracker,
             const Layout&              layout,
             Text::View_t&&             string,
             TextAlign                  alignment)
{
    return  Widget(WawtEnv::sLabel, std::move(tracker), layout)
            .text(std::move(string))
            .verticalAlign(TextAlign::eBASELINE)
            .horizontalAlign(alignment);                              // RETURN
}

Widget label(const Layout& layout, Text::View_t&& string, TextAlign alignment)
{
    return  Widget(WawtEnv::sLabel, layout)
            .text(std::move(string))
            .verticalAlign(TextAlign::eBASELINE)
            .horizontalAlign(alignment);                              // RETURN
}

Widget panel(Trackee&& tracker, const Layout& layout, std::any options)
{
    return Widget(WawtEnv::sPanel, std::move(tracker), layout)
            .options(std::move(options));                             // RETURN
}

Widget panel(const Layout& layout, std::any options)
{
    return Widget(WawtEnv::sPanel, layout)
            .options(std::move(options));                             // RETURN
}

Widget pushButton(Trackee&&             tracker,
                  const Layout&         layout,
                  OnClickCb             clicked,
                  Text::View_t&&        string,
                  CharSizeGroup         group,
                  TextAlign             alignment)
{
    return  Widget(WawtEnv::sButton, std::move(tracker), layout)
            .downEventMethod(makePushButtonDownMethod(std::move(clicked)))
            .text(std::move(string))
            .charSizeGroup(group)
            .horizontalAlign(alignment);                              // RETURN
}

Widget pushButton(const Layout&         layout,
                  OnClickCb             clicked,
                  Text::View_t&&        string,
                  CharSizeGroup         group,
                  TextAlign             alignment)
{
    return pushButton(Trackee(), layout,
                      clicked, std::move(string), group, alignment);  // RETURN
}

Widget pushButton(const Layout&         layout,
                  OnClickCb             clicked,
                  Text::View_t&&        string,
                  TextAlign             alignment)
{
    return pushButton(Trackee(), layout, clicked,
                      std::move(string), CharSizeGroup(), alignment); // RETURN
}

Widget pushButton(Trackee&&             tracker,
                  const Layout&         layout,
                  OnClickCb             clicked,
                  Text::View_t&&        string,
                  TextAlign             alignment)
{
    return pushButton(std::move(tracker), layout, clicked,
                      std::move(string), CharSizeGroup(), alignment); // RETURN
}

Widget pushButtonGrid(Trackee&&               tracker,
                      Layout                  gridLayout,
                      int                     columns,
                      double                  borderThickness,
                      CharSizeGroup           group,
                      TextAlign               alignment,
                      LabelClickList          buttonDefs,
                      bool                    spaced)
{
    auto gridPanel   = panel(std::move(tracker), gridLayout);
    auto rows        = std::size_t{};
    auto childLayout = gridLayoutGenerator(borderThickness,
                                           buttonDefs.size(),
                                           columns,
                                           &rows);
    auto bounds      = std::make_shared<Bounds>();

    if (spaced) {
        gridPanel.layoutMethod(
            [] (Widget                 *widget,
                const Widget&           parent,
                bool                    firstPass,
                DrawProtocol           *adapter) -> void
            {
                if (firstPass) {
                    Widget::defaultLayout(widget, parent, true, adapter);
                }
                else if (widget->hasChildren()) {
                    for (auto& child : widget->children()) {
                        auto fn = child.layoutMethod();
                        fn(&child, *widget, true, adapter);
                    }
                }
            });
    }
                    
    for (auto& [label, click] : buttonDefs) {
        gridPanel.addChild(pushButton(childLayout(),
                                      click,
                                      Text::View_t(label),
                                      group,
                                      alignment));
        if (spaced) {
            gridPanel.children()
                     .back()
                     .layoutMethod(genSpacedLayout(bounds,
                                                   rows,
                                                   columns,
                                                   alignment));
        }
    }
    return gridPanel;                                                 // RETURN
}

Widget pushButtonGrid(const Layout&           gridLayout,
                      int                     columns,
                      double                  borderThickness,
                      CharSizeGroup           group,
                      TextAlign               alignment,
                      LabelClickList          buttonDefs,
                      bool                    spaced)
{
    return pushButtonGrid(Trackee(),  gridLayout, columns, borderThickness,
                          group, alignment, buttonDefs, spaced);      // RETURN
}

Widget pushButtonGrid(Trackee&&               tracker,
                      const Layout&           gridLayout,
                      int                     columns,
                      double                  borderThickness,
                      CharSizeGroup           group,
                      LabelClickList          buttonDefs,
                      bool                    spaced)
{
    return pushButtonGrid(std::move(tracker), gridLayout, columns,
                          borderThickness, group, TextAlign::eCENTER,
                          buttonDefs, spaced);                        // RETURN
}

Widget pushButtonGrid(const Layout&           gridLayout,
                      int                     columns,
                      double                  borderThickness,
                      CharSizeGroup           group,
                      LabelClickList          buttonDefs,
                      bool                    spaced)
{
    return pushButtonGrid(Trackee(), gridLayout, columns, borderThickness,
                          group, TextAlign::eCENTER, buttonDefs,
                          spaced);                                    // RETURN
}

Widget pushButtonGrid(Trackee&&               tracker,
                      const Layout&           gridLayout,
                      double                  borderThickness,
                      CharSizeGroup           group,
                      LabelClickList          buttonDefs,
                      bool                    spaced)
{
    return pushButtonGrid(std::move(tracker), gridLayout, buttonDefs.size(),
                          borderThickness, group, TextAlign::eCENTER,
                          buttonDefs, spaced);                        // RETURN
}

Widget pushButtonGrid(const Layout&           gridLayout,
                      double                  borderThickness,
                      CharSizeGroup           group,
                      LabelClickList          buttonDefs,
                      bool                    spaced)
{
    return pushButtonGrid(Trackee(), gridLayout, buttonDefs.size(),
                          borderThickness, group, TextAlign::eCENTER,
                          buttonDefs, spaced);                        // RETURN
}

Widget radioButtonPanel(Trackee&&               tracker,
                        const Layout&           panelLayout,
                        const GroupClickCb&     gridCb,
                        CharSizeGroup           group,
                        TextAlign               alignment,
                        LabelList               labels,
                        int                     columns)
{
    auto childLayout = gridLayoutGenerator(0.0, labels.size(), columns);
    auto gridPanel   = Widget(WawtEnv::sPanel,
                              std::move(tracker),
                              panelLayout);

    for (auto label : labels) {
        gridPanel.addChild(
            Widget(WawtEnv::sItem, std::move(tracker), childLayout())
                .downEventMethod(makeRadioButtonDownMethod(gridCb))
                .useTextBounds(true)
                .text(std::move(label))
                .charSizeGroup(group)
                .horizontalAlign(alignment)
                .textMark(Text::BulletMark::eROUND,
                          alignment != TextAlign::eRIGHT));

    }
    return gridPanel;                                                 // RETURN
}

Widget radioButtonPanel(const Layout&           panelLayout,
                        const GroupClickCb&     gridCb,
                        CharSizeGroup           group,
                        TextAlign               alignment,
                        LabelList               labels,
                        int                     columns)
{
    return radioButtonPanel(Trackee(), panelLayout,
                            gridCb, group, alignment, labels, columns);
}

Widget radioButtonPanel(Trackee&&               tracker,
                        const Layout&           panelLayout,
                        const GroupClickCb&     gridCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        int                     columns)
{
    return radioButtonPanel(std::move(tracker), panelLayout,
                            gridCb, group, TextAlign::eLEFT, labels, columns);
}

Widget radioButtonPanel(const Layout&           panelLayout,
                        const GroupClickCb&     gridCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        int                     columns)
{
    return radioButtonPanel(Trackee(), panelLayout,
                            gridCb, group, TextAlign::eLEFT, labels, columns);
}

Widget widgetGrid(Trackee&&                    tracker,
                  const Layout&                layoutPanel,
                  int                          rows,
                  int                          columns,
                  const WidgetGenerator&       generator,
                  bool                         spaced)
{
    auto grid   = panel(std::move(tracker), layoutPanel);
    auto bounds = std::make_shared<Bounds>();

    for (auto r = 0; r < rows; ++r) {
        for (auto c = 0; c < columns; ++c) {
            auto next = generator(r, c);

            if (spaced && next.hasText()) {
                next.layoutMethod(genSpacedLayout(bounds,
                                                  rows,
                                                  columns,
                                                  next.text()
                                                      .d_layout
                                                      .horizontalAlign(),
                                                  next.text()
                                                      .d_layout
                                                      .verticalAlign()));
            }
            grid.addChild(std::move(next));
        }
    }

    if (spaced && bounds.use_count() > 1) {
        grid.layoutMethod(
            [] (Widget                 *widget,
                const Widget&           parent,
                bool                    firstPass,
                DrawProtocol           *adapter) -> void
            {
                if (firstPass) {
                    Widget::defaultLayout(widget, parent, true, adapter);
                }
                else if (widget->hasChildren()) {
                    for (auto& child : widget->children()) {
                        auto fn = child.layoutMethod();
                        fn(&child, *widget, true, adapter);
                    }
                }
            });
    }
    return grid;                                                      // RETURN
}


Widget widgetGrid(const Layout&                layoutPanel,
                  int                          rows,
                  int                          columns,
                  const WidgetGenerator&       generator,
                  bool                         spaced)
{
    return widgetGrid(Trackee(),
                      layoutPanel,
                      rows,
                      columns,
                      generator,
                      spaced);                                        // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
