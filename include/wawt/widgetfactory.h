/** @file widgetfactory.h
 *  @brief Factory methods to create different widget "classes".
 *
 * Copyright 2018 Bruce Szablak
 *
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

#ifndef WAWT_WIDGETFACTORY_H
#define WAWT_WIDGETFACTORY_H

#include "wawt/widget.h"

#include <functional>
#include <initializer_list>
#include <utility>

namespace Wawt {

using FocusChgCb        = std::function<FocusCb(Widget *)>;
using FocusChgLabel     = std::pair<FocusChgCb, StringView_t>;
using FocusChgLabelList = std::initializer_list<FocusChgLabel>;

using GridFocusCb       = std::function<FocusCb(Widget *,uint16_t relativeId)>;

using LabelList         = std::initializer_list<StringView_t>;

inline
FocusChgCb focusWrap(std::function<void(Widget *w)>&& vcb) {
    return
        [cb=std::move(vcb)](Widget *w) -> FocusCb {
            cb(w);
            return FocusCb();
        };                                                            // RETURN
}

inline
GridFocusCb focusWrap(std::function<void(Widget *w, uint16_t id)>&& vcb) {
    return
        [cb = std::move(vcb)](Widget *w, uint16_t id) -> FocusCb {
            cb(w, id);
            return FocusCb();
        };                                                            // RETURN
}

Widget radioButtonPanel(Widget               **indirect,
                        Layout&&               layout,
                        const GridFocusCb&     gridCb,
                        CharSizeGroup          group,
                        LabelList              labels,
                        TextAlign              alignment = TextAlign::eLEFT,
                        int                    columns   = 1);

Widget radioButtonPanel(Layout&&               layout,
                        const GridFocusCb&     gridCb,
                        CharSizeGroup          group,
                        LabelList              labels,
                        TextAlign              alignment = TextAlign::eLEFT,
                        int                    columns   = 1);

Widget checkBox(Widget                       **indirect,
                Layout&&                       layout,
                StringView_t                   string,
                CharSizeGroup                  group     = CharSizeGroup(),
                TextAlign                      alignment = TextAlign::eLEFT);

Widget checkBox(Layout&&                       layout,
                StringView_t                   string,
                CharSizeGroup                  group      = CharSizeGroup(),
                TextAlign                      alignment  = TextAlign::eLEFT);

Widget dropDownList(Widget                   **indirect,
                    Layout&&                   layout,
                    GridFocusCb&&              selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels);

Widget dropDownList(Layout&&                   layout,
                    GridFocusCb&&              selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels);

Widget fixedSizeList(Widget                  **indirect,
                     Layout&&                  layout,
                     bool                      singleSelect,
                     const GridFocusCb&        selectCb,
                     CharSizeGroup             group,
                     LabelList                 labels);

Widget fixedSizeList(Layout&&                  layout,
                     bool                      singleSelect,
                     const GridFocusCb&        selectCb,
                     CharSizeGroup             group,
                     LabelList                 labels);

Widget label(Widget                          **indirect,
             Layout&&                          layout,
             StringView_t                      string,
             TextAlign                         alignment = TextAlign::eCENTER,
             CharSizeGroup                     group     = CharSizeGroup());

Widget label(Layout&&                          layout,
             StringView_t                      string,
             TextAlign                         alignment = TextAlign::eCENTER,
             CharSizeGroup                     group     = CharSizeGroup());

Widget label(Widget                          **indirect,
             Layout&&                          layout,
             StringView_t                      string,
             CharSizeGroup                     group);

Widget label(Layout&&                          layout,
             StringView_t                      string,
             CharSizeGroup                     group);

Widget panel(Widget                          **indirect,
             Layout&&                          layout  = Layout(),
             std::any                          options = std::any());

Widget panel(Layout&&                          layout = Layout(),
             std::any                          options = std::any());

template<typename... WIDGET>
Widget layoutPanel(Widget                    **indirect,
                   Layout&&                    panelLayout,
                   const LayoutGenerator&      generator,
                   WIDGET&&...                 widgets)
{
    auto container = panel(indirect, std::move(panelLayout));
    (container.addChild(std::move(widgets).layout(generator())),...);
    return container;                                                 // RETURN
}

template<typename... WIDGET>
Widget layoutPanel(Layout&&                    panelLayout,
                   const LayoutGenerator&      generator,
                   WIDGET&&...                 widgets)
{
    return layoutPanel(nullptr, std::move(panelLayout), generator,
                       std::forward<WIDGET>(widgets)...);             // RETURN
}

template<typename... WIDGET>
Widget layoutPanel(Widget                     **indirect,
                   Layout&&                     panelLayout,
                   double                       widgetBorder,
                   int                          columns,
                   WIDGET&&...                  widgets)
{
    auto layoutFn  = gridLayoutGenerator(widgetBorder,
                                         columns,
                                         sizeof...(widgets));
    auto grid = panel(indirect, std::move(panelLayout));
    (grid.addChild(std::move(widgets).layout(layoutFn())),...);
    return grid;                                                      // RETURN
}

template<typename... WIDGET>
Widget layoutPanel(Layout&&                     panelLayout,
                   double                       widgetBorder,
                   int                          columns,
                   WIDGET&&...                  widgets)
{
    return layoutPanel(nullptr, std::move(panelLayout), widgetBorder, columns,
                      std::forward<WIDGET>(widgets)...);               // RETURN
}

Widget pushButton(Widget                     **indirect,
                  Layout&&                     layout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  TextAlign                    alignment = TextAlign::eCENTER,
                  CharSizeGroup                group     = CharSizeGroup());

Widget pushButton(Layout&&                     layout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  TextAlign                    alignment = TextAlign::eCENTER,
                  CharSizeGroup                group     = CharSizeGroup());

Widget pushButton(Widget                     **indirect,
                  Layout&&                     layout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  CharSizeGroup                group);

Widget pushButton(Layout&&                     layout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  CharSizeGroup                group);

Widget pushButtonGrid(Widget                 **indirect,
                      Layout&&                 layout,
                      CharSizeGroup            group,
                      int                      columns,
                      FocusChgLabelList        buttonDefs,
                      bool                     fitted    = false,
                      TextAlign                alignment = TextAlign::eCENTER);

Widget pushButtonGrid(Layout&&                 layout,
                      CharSizeGroup            group,
                      int                      columns,
                      FocusChgLabelList        buttonDefs,
                      bool                     fitted    = false,
                      TextAlign                alignment = TextAlign::eCENTER);

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
