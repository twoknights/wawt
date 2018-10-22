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

void addDropDown(Widget *dropDown, Widget *select, const GridFocusCb& selectCb)
{
    auto root = dropDown->screen();
    auto id   = root->widgetIdValue();
    auto relativeId
              = root->hasChildren() ? root->children().back().relativeId() + 1
                                    : 0;
    // First push a transparent panel which, when clicked on, discards
    // the drop-down widget.
    auto soak = panel(Layout(root->layout()))
                 .method([root, id](auto, auto, auto, auto) {
                             return
                                 [root, id](float, float, bool up) {
                                     if (up) {
                                         root->children().pop_back();
                                         root->drawData().d_widgetId = id;
                                     }
                                     return FocusCb();
                                 };
                         });
    auto screenBox                  = root->drawData().d_rectangle;
    auto screenBorders              = root->drawData().d_borders;
    soak.drawData().d_rectangle     = screenBox;
    soak.drawData().d_borders       = screenBorders;
    auto& screen = root->children().emplace_back(std::move(soak));
    auto  newChildMethod = root->getInstalled<Widget::NewChildMethod>();
    
    if (newChildMethod) {
        newChildMethod(root, &screen);
    }
    // 'dropDown' contains the fixed sized list that will be the drop down.
    // It's layout data is not usable in the copy to be made, but its draw
    // data is corret, and can be used to generate the layout.
    auto& list = screen.children()
                       .emplace_back(dropDown->children()[1].clone());
    list.hidden(false).disabled(false);
    
    auto ux    = list.drawData().d_rectangle.d_ux;
    auto uy    = list.drawData().d_rectangle.d_uy;
    auto lx    = ux + list.drawData().d_rectangle.d_width;
    auto ly    = uy + list.drawData().d_rectangle.d_height;

    auto width = screenBox.d_width  - 2*screenBorders.d_x;
    auto height= screenBox.d_height - 2*screenBorders.d_y;

    list.layout().d_upperLeft.d_sX         = -1.0 + 2.0 * ux / width;
    list.layout().d_upperLeft.d_sY         = -1.0 + 2.0 * uy / height;
    list.layout().d_upperLeft.d_widgetRef  = WidgetId::kPARENT;
    list.layout().d_lowerRight.d_sX        = -1.0 + 2.0 * lx / width;
    list.layout().d_lowerRight.d_sY        = -1.0 + 2.0 * ly / height;
    list.layout().d_lowerRight.d_widgetRef = WidgetId::kPARENT;

    for (auto& item : list.children()) {
        // item layout is relative to list so they require no adjustments.
        item.method( // on click copy label to drop-down etc. and clean up.
            [cb=selectCb,select,id,root](auto, auto, Widget *widget, auto)
            {
                widget->drawData().d_selected = true;
                return
                    [cb,select,widget,id,root](float x, float y, bool up)
                    {
                        if (up) {
                            widget->drawData().d_selected = false;

                            if (widget->inside(x, y)) {
                                select->resetLabel(widget->drawData().d_label);
                                auto lclselect = select;
                                auto lclcb   = std::move(cb);
                                auto rid     = widget->drawData().d_relativeId;
                                root->drawData().d_widgetId = id;
                                // The "pop" destroys the lambda captures, so
                                // only the local variables can be used
                                // following it.
                                root->children().pop_back();

                                if (lclcb) {
                                    return lclcb(lclselect, rid);
                                }
                            }
                        }
                        return FocusCb();
                    };
            });
    }

    root->drawData().d_widgetId
        = screen.assignWidgetIds(id,
                                 relativeId,
                                 dropDown->layoutData().d_charSizeMap,
                                 root);
    return;                                                           // RETURN
}

Widget::DownEventMethod createDropDown(GridFocusCb&& selectCb)
{
    return
        [cb=std::move(selectCb)] (auto, auto, auto *widget, auto *parent) {
            widget->drawData().d_selected = true;
            return  [cb, widget, parent](float x, float y, bool up) {
                if (up) {
                    widget->drawData().d_selected = false;
                    if (widget->inside(x, y)) {
                        addDropDown(parent, widget, cb);
                    }
                }
                return FocusCb();
            };
        };                                                            // RETURN
}

Widget::DownEventMethod makePushButtonDownMethod(FocusChgCb&& onClick)
{
    return
        [cb=std::move(onClick)] (auto, auto, auto *widget, auto) -> EventUpCb {
            widget->drawData().d_selected = true;
            return  [cb, widget](float x, float y, bool up) -> FocusCb {
                if (up) {
                    widget->drawData().d_selected = false;
                    if (cb) {
                        if (widget->inside(x, y)) {
                            return cb(widget);
                        }
                    }
                }
                return FocusCb();
            };
        };                                                            // RETURN
}

Widget::DownEventMethod makeToggleButtonDownMethod(const GridFocusCb&  cb)
{
    return
        [cb] (auto, auto, auto *widget, auto *parent) -> EventUpCb {
            return  [cb, widget, parent](float x, float y, bool up) -> FocusCb{
                if (up && widget->inside(x, y)) {
                    widget->drawData().d_selected =
                        !widget->drawData().d_selected;
                    if (cb) {
                        return cb(parent, widget->relativeId());
                    }
                }
                return FocusCb();
            };
        };                                                            // RETURN
}

Widget::DownEventMethod makeRadioButtonDownMethod(const GridFocusCb& cb)
{
    return
        [cb] (auto, auto, auto *widget, auto *parent) -> EventUpCb {
            return  [cb, widget, parent](float x, float y, bool up) -> FocusCb{
                if (up && widget->inside(x, y)) {
                    if (!widget->drawData().d_selected) {
                        for (auto& child : parent->children()) {
                            child.drawData().d_selected = 0;
                        }
                        widget->drawData().d_selected = true;

                        if (cb) {
                            return cb(parent, widget->relativeId());
                        }
                    }
                }
                return FocusCb();
            };
        };                                                            // RETURN
}

using DimensionPtr = std::shared_ptr<Dimensions>;

Widget::LayoutMethod genSpacedLayout(const DimensionPtr&   bounds,
                                     int                   rows,
                                     int                   columns,
                                     TextAlign             alignment)
{
    return
        [=] (Widget                 *widget,
             const Widget&           parent,
             const Widget&           root,
             bool                    firstPass,
             DrawProtocol           *adapter) -> void {
            Widget::defaultLayout(widget, parent, root, firstPass, adapter);

            auto& data = widget->drawData();

            if (firstPass) {
                if (data.d_relativeId == 0 // ignore old values
                 || data.d_labelBounds.d_width > bounds->d_x) {
                    bounds->d_x = data.d_labelBounds.d_width;
                }

                if (data.d_relativeId == 0
                 || data.d_labelBounds.d_height > bounds->d_y) {
                    bounds->d_y = data.d_labelBounds.d_height;
                }
                return;
            }
            // bounds gives the text box dimensions. Figure out the border:
            auto ratio = widget->layoutData().d_layout.d_percentBorder/200.0;
            auto width = (bounds->d_x+2) / (1.0 - ratio);
            auto height= (bounds->d_y+2) / (1.0 - ratio) - 2;

            auto panelWidth   = parent.drawData().d_rectangle.d_width;
            auto panelHeight  = parent.drawData().d_rectangle.d_height;
            auto panelBorders = parent.drawData().d_borders;

            if (panelWidth-2*panelBorders.d_x <= width) {
                assert(!"width bad");
                width = panelWidth-2*panelBorders.d_x;
            }

            if (panelHeight-2*panelBorders.d_y <= height) {
                assert(!"height bad");
                height = panelHeight-2*panelBorders.d_y;
            }

            data.d_borders= Dimensions{float(bounds->d_x * ratio),
                                       float(bounds->d_y * ratio)};

            if (ratio > 0) { // always have a border
                if (data.d_borders.d_x == 0) {
                    data.d_borders.d_x = 1;
                }
                if (data.d_borders.d_y == 0) {
                    data.d_borders.d_y = 1;
                }
            }
            auto offset      = Dimensions{2*(data.d_borders.d_x+2),
                                          2*(data.d_borders.d_y+2)};
            // Calculate seperator spacing between buttons:
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
            auto c       = data.d_relativeId % columns;
            auto r       = data.d_relativeId / columns;
            
            // Translate the button at row 'r' and column 'c'
            auto& rectangle     = data.d_rectangle;
            rectangle.d_width   = width;
            rectangle.d_height  = height;
            rectangle.d_ux      = parent.drawData().d_rectangle.d_ux
                                        + marginx + c * (spacex+width);
            rectangle.d_uy      = parent.drawData().d_rectangle.d_uy
                                        + marginy + r * (spacey+height);
            auto& label         = data.d_labelBounds;
            label.d_uy          = rectangle.d_uy + (height-label.d_height)/2.0;
            label.d_ux          = rectangle.d_ux + offset.d_x/2.0;

            if (alignment != TextAlign::eLEFT) {
                auto space = width - label.d_width - offset.d_x;

                if (alignment == TextAlign::eCENTER) {
                    space /= 2.0;
                }
                label.d_ux += space;
            }
        };                                                            // RETURN
}

} // unnamed namespace

Widget checkBox(Widget                **indirect,
                const Layout&           layout,
                StringView_t            string,
                CharSizeGroup           group,
                TextAlign               alignment)
{
    return  Widget(WawtEnv::sBullet, indirect, layout)
            .method(makeToggleButtonDownMethod(GridFocusCb()))
            .labelSelect(layout.d_percentBorder <= 0.0)
            .text(string, group, alignment)
            .textMark(DrawData::BulletMark::eSQUARE,
                      alignment != TextAlign::eRIGHT);                // RETURN
}

Widget checkBox(const Layout&           layout,
                StringView_t            string,
                CharSizeGroup           group,
                TextAlign               alignment)
{
    return  Widget(WawtEnv::sBullet, layout)
            .method(makeToggleButtonDownMethod(GridFocusCb()))
            .labelSelect(layout.d_percentBorder <= 0.0)
            .text(string, group, alignment)
            .textMark(DrawData::BulletMark::eSQUARE,
                      alignment != TextAlign::eRIGHT);                // RETURN
}

Widget dropDownList(Widget                   **indirect,
                    Layout                     layout,
                    GridFocusCb&&              selectCb,
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
    auto selectLayout = gridLayoutGenerator(layout.d_percentBorder,
                                            1,
                                            labels.size()+1);
    auto parent
        = panel(indirect, layout.border(0.0))
            .addChild(Widget(WawtEnv::sPush, selectLayout())
                        .method(createDropDown(std::move(selectCb)))
                        .text(S(""), group, TextAlign::eLEFT)
                        .textMark(DrawData::BulletMark::eDOWNARROW))
            .addChild(fixedSizeList({{-1.0,1.0,0_wr},{1.0,1.0}},
                                    true,
                                    GridFocusCb(),
                                    group,
                                    labels).disabled(true).hidden(true));
    return parent;                                                    // RETURN
}

Widget dropDownList(const Layout&              listLayout,
                    GridFocusCb&&              selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels)
{
    return dropDownList(nullptr, listLayout, std::move(selectCb),
                        group, labels);
}

Widget fixedSizeList(Widget                **indirect,
                     const Layout&           listLayout,
                     bool                    singleSelect,
                     const GridFocusCb&      selectCb,
                     CharSizeGroup           group,
                     LabelList               labels)
{
    auto childLayout = gridLayoutGenerator(0.0, 1, labels.size());
    auto list        = Widget(WawtEnv::sList, indirect, listLayout);

    for (auto& label : labels) {
        list.addChild(
            Widget(WawtEnv::sItem, indirect, childLayout())
                .method(singleSelect
                    ? makeRadioButtonDownMethod(selectCb)
                    : makeToggleButtonDownMethod(selectCb))
                .text(label, group, TextAlign::eCENTER));

    }
    return list;                                                      // RETURN
}

Widget fixedSizeList(const Layout&           listLayout,
                     bool                    singleSelect,
                     const GridFocusCb&      selectCb,
                     CharSizeGroup           group,
                     LabelList               labels)
{
    return fixedSizeList(nullptr, listLayout, singleSelect,
                         selectCb, group, labels);
}

Widget label(Widget                   **indirect,
             const Layout&              layout,
             StringView_t               string,
             CharSizeGroup              group,
             TextAlign                  alignment)
{
    return  Widget(WawtEnv::sLabel, indirect, layout)
            .text(string, group, alignment);                          // RETURN
}

Widget label(const Layout&              layout,
             StringView_t               string,
             CharSizeGroup              group,
             TextAlign                  alignment)
{
    return  Widget(WawtEnv::sLabel, layout)
            .text(string, group, alignment);                          // RETURN
}

Widget label(Widget                   **indirect,
             const Layout&              layout,
             StringView_t               string,
             TextAlign                  alignment)
{
    return  Widget(WawtEnv::sLabel, indirect, layout)
            .text(string, alignment);                                 // RETURN
}

Widget label(const Layout& layout, StringView_t string, TextAlign alignment)
{
    return  Widget(WawtEnv::sLabel, layout)
            .text(string, alignment);                                 // RETURN
}

Widget panel(Widget **indirect, const Layout& layout, std::any options)
{
    return Widget(WawtEnv::sPanel, indirect, layout)
            .options(std::move(options));                             // RETURN
}

Widget panel(const Layout& layout, std::any options)
{
    return Widget(WawtEnv::sPanel, layout)
            .options(std::move(options));                             // RETURN
}

Widget pushButton(Widget              **indirect,
                  const Layout&         layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  CharSizeGroup         group,
                  TextAlign             alignment)
{
    return  Widget(WawtEnv::sPush, indirect, layout)
            .method(makePushButtonDownMethod(std::move(clicked)))
            .text(string, group, alignment);                          // RETURN
}

Widget pushButton(const Layout&         layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  CharSizeGroup         group,
                  TextAlign             alignment)
{
    return pushButton(nullptr, layout,
                      clicked, string, group, alignment);             // RETURN
}

Widget pushButton(const Layout&         layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  TextAlign             alignment)
{
    return pushButton(nullptr, layout, clicked,
                      string, CharSizeGroup(), alignment);            // RETURN
}

Widget pushButton(Widget              **indirect,
                  const Layout&         layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  TextAlign             alignment)
{
    return pushButton(indirect, layout, clicked,
                      string, CharSizeGroup(), alignment);            // RETURN
}

Widget pushButtonGrid(Widget                **indirect,
                      Layout                  layout,
                      int                     columns,
                      CharSizeGroup           group,
                      TextAlign               alignment,
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted)
{
    auto border      = layout.d_percentBorder;
    auto gridPanel   = panel(indirect, layout.border(0.0));
    auto rows        = std::size_t{};
    auto childLayout = gridLayoutGenerator(border,
                                           columns,
                                           buttonDefs.size(),
                                           &rows);
    auto bounds      = std::make_shared<Dimensions>();

    for (auto& [click, label] : buttonDefs) {
        gridPanel.addChild(pushButton(childLayout(),
                                      click,
                                      label,
                                      group,
                                      alignment));
        if (fitted) {
            gridPanel.children()
                     .back()
                     .method(genSpacedLayout(bounds,
                                             rows,
                                             columns,
                                             alignment));
        }
    }
    return gridPanel;                                                 // RETURN
}

Widget pushButtonGrid(const Layout&           layout,
                      int                     columns,
                      CharSizeGroup           group,
                      TextAlign               alignment,
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted)
{
    return pushButtonGrid(nullptr,  layout, columns, group,
                          alignment, buttonDefs, fitted);             // RETURN
}

Widget pushButtonGrid(Widget                **indirect,
                      const Layout&           layout,
                      int                     columns,
                      CharSizeGroup           group,
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted)
{
    return pushButtonGrid(indirect, layout, columns, group,
                          TextAlign::eCENTER, buttonDefs, fitted);    // RETURN
}

Widget pushButtonGrid(const Layout&           layout,
                      int                     columns,
                      CharSizeGroup           group,
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted)
{
    return pushButtonGrid(nullptr,  layout, columns, group,
                          TextAlign::eCENTER, buttonDefs, fitted);    // RETURN
}

Widget pushButtonGrid(Widget                **indirect,
                      const Layout&           layout,
                      CharSizeGroup           group,
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted)
{
    return pushButtonGrid(indirect, layout, buttonDefs.size(),group,
                          TextAlign::eCENTER, buttonDefs, fitted);    // RETURN
}

Widget pushButtonGrid(const Layout&           layout,
                      CharSizeGroup           group,
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted)
{
    return pushButtonGrid(nullptr, layout, buttonDefs.size(), group,
                          TextAlign::eCENTER, buttonDefs, fitted);    // RETURN
}

Widget radioButtonPanel(Widget                **indirect,
                        const Layout&           layout,
                        const GridFocusCb&      gridCb,
                        CharSizeGroup           group,
                        TextAlign               alignment,
                        LabelList               labels,
                        int                     columns)
{
    auto childLayout = gridLayoutGenerator(0.0, columns, labels.size());
    auto gridPanel   = Widget(WawtEnv::sPanel, indirect, layout);

    for (auto& label : labels) {
        gridPanel.addChild(
            Widget(WawtEnv::sItem, indirect, childLayout())
                .method(makeRadioButtonDownMethod(gridCb))
                .labelSelect(true)
                .text(label, group, alignment)
                .textMark(DrawData::BulletMark::eROUND,
                          alignment != TextAlign::eRIGHT));

    }
    return gridPanel;                                                 // RETURN
}

Widget radioButtonPanel(const Layout&           layout,
                        const GridFocusCb&      gridCb,
                        CharSizeGroup           group,
                        TextAlign               alignment,
                        LabelList               labels,
                        int                     columns)
{
    return radioButtonPanel(nullptr, layout,
                            gridCb, group, alignment, labels, columns);
}

Widget radioButtonPanel(Widget                **indirect,
                        const Layout&           layout,
                        const GridFocusCb&      gridCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        int                     columns)
{
    return radioButtonPanel(indirect, layout,
                            gridCb, group, TextAlign::eLEFT, labels, columns);
}

Widget radioButtonPanel(const Layout&           layout,
                        const GridFocusCb&      gridCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        int                     columns)
{
    return radioButtonPanel(nullptr, layout,
                            gridCb, group, TextAlign::eLEFT, labels, columns);
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
