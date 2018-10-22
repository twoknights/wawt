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

Widget checkBox(Widget                       **indirect,
                const Layout&                  layout,
                StringView_t                   string,
                CharSizeGroup                  group     = CharSizeGroup(),
                TextAlign                      alignment = TextAlign::eLEFT);

Widget checkBox(const Layout&                  layout,
                StringView_t                   string,
                CharSizeGroup                  group      = CharSizeGroup(),
                TextAlign                      alignment  = TextAlign::eLEFT);

Widget dropDownList(Widget                   **indirect,
                    Layout                     listLayout,
                    GridFocusCb&&              selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels);

Widget dropDownList(const Layout&              listLayout,
                    GridFocusCb&&              selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels);

Widget fixedSizeList(Widget                  **indirect,
                     const Layout&             listLayout,
                     bool                      singleSelect,
                     const GridFocusCb&        selectCb,
                     CharSizeGroup             group,
                     LabelList                 labels);

Widget fixedSizeList(const Layout&             listLayout,
                     bool                      singleSelect,
                     const GridFocusCb&        selectCb,
                     CharSizeGroup             group,
                     LabelList                 labels);

Widget label(Widget                          **indirect,
             const Layout&                     layout,
             StringView_t                      string,
             CharSizeGroup                     group     = CharSizeGroup(),
             TextAlign                         alignment = TextAlign::eCENTER);

Widget label(const Layout&                     layout,
             StringView_t                      string,
             CharSizeGroup                     group     = CharSizeGroup(),
             TextAlign                         alignment = TextAlign::eCENTER);

Widget label(Widget                          **indirect,
             const Layout&                     layout,
             StringView_t                      string,
             TextAlign                         alignment);

Widget label(const Layout&                     layout,
             StringView_t                      string,
             TextAlign                         alignment);

Widget panel(Widget                          **indirect,
             const Layout&                     layout  = Layout(),
             std::any                          options = std::any());

Widget panel(const Layout&                     layout = Layout(),
             std::any                          options = std::any());

template<typename... WIDGET>
Widget panelLayout(Widget                    **indirect,
                   const Layout&               layoutPanel,
                   const LayoutGenerator&      generator,
                   WIDGET&&...                 widgets)
{
    auto container = panel(indirect, layoutPanel);
    (container.addChild(std::move(widgets).layout(generator())),...);
    return container;                                                 // RETURN
}

template<typename... WIDGET>
Widget panelLayout(const Layout&               layoutPanel,
                   const LayoutGenerator&      generator,
                   WIDGET&&...                 widgets)
{
    return panelLayout(nullptr, layoutPanel, generator,
                       std::forward<WIDGET>(widgets)...);             // RETURN
}

template<typename... WIDGET>
Widget panelLayout(Widget                     **indirect,
                   const Layout&                layoutPanel,
                   double                       widgetBorder,
                   int                          columns,
                   WIDGET&&...                  widgets)
{
    auto layoutFn  = gridLayoutGenerator(widgetBorder,
                                         columns,
                                         sizeof...(widgets));
    auto grid = panel(indirect, layoutPanel);
    (grid.addChild(std::move(widgets).layout(layoutFn())),...);
    return grid;                                                      // RETURN
}

template<typename... WIDGET>
Widget panelLayout(const Layout&                layoutPanel,
                   double                       widgetBorder,
                   int                          columns,
                   WIDGET&&...                  widgets)
{
    return panelLayout(nullptr, layoutPanel, widgetBorder, columns,
                      std::forward<WIDGET>(widgets)...);               // RETURN
}

Widget pushButton(Widget                     **indirect,
                  const Layout&                layout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  CharSizeGroup                group     = CharSizeGroup(),
                  TextAlign                    alignment = TextAlign::eCENTER);

Widget pushButton(const Layout&                layout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  CharSizeGroup                group     = CharSizeGroup(),
                  TextAlign                    alignment = TextAlign::eCENTER);

Widget pushButton(Widget                     **indirect,
                  const Layout&                layout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  TextAlign                    alignment);

Widget pushButton(const Layout&                layout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  TextAlign                    alignment);

Widget pushButtonGrid(Widget                 **indirect,
                      Layout                   layout,
                      int                      columns,
                      CharSizeGroup            group,
                      TextAlign                alignment,
                      FocusChgLabelList        buttonDefs,
                      bool                     fitted    = true);

Widget pushButtonGrid(const Layout&            layout,
                      int                      columns,
                      CharSizeGroup            group,
                      TextAlign                alignment,
                      FocusChgLabelList        buttonDefs,
                      bool                     fitted    = true);

Widget pushButtonGrid(Widget                 **indirect,
                      const Layout&            layout,
                      int                      columns,
                      CharSizeGroup            group,
                      FocusChgLabelList        buttonDefs,
                      bool                     fitted    = true);

Widget pushButtonGrid(const Layout&            layout,
                      int                      columns,
                      CharSizeGroup            group,
                      FocusChgLabelList        buttonDefs,
                      bool                     fitted    = true);

Widget pushButtonGrid(Widget                 **indirect,
                      const Layout&            layout,
                      CharSizeGroup            group,
                      FocusChgLabelList        buttonDefs,
                      bool                     fitted    = true);

Widget pushButtonGrid(const Layout&            layout,
                      CharSizeGroup            group,
                      FocusChgLabelList        buttonDefs,
                      bool                     fitted    = true);

Widget radioButtonPanel(Widget               **indirect,
                        const Layout&          layout,
                        const GridFocusCb&     gridCb,
                        CharSizeGroup          group,
                        TextAlign              alignment,
                        LabelList              labels,
                        int                    columns   = 1);

Widget radioButtonPanel(const Layout&          layout,
                        const GridFocusCb&     gridCb,
                        CharSizeGroup          group,
                        TextAlign              alignment,
                        LabelList              labels,
                        int                    columns   = 1);

Widget radioButtonPanel(Widget               **indirect,
                        const Layout&          layout,
                        const GridFocusCb&     gridCb,
                        CharSizeGroup          group,
                        LabelList              labels,
                        int                    columns   = 1);

Widget radioButtonPanel(const Layout&          layout,
                        const GridFocusCb&     gridCb,
                        CharSizeGroup          group,
                        LabelList              labels,
                        int                    columns   = 1);

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
