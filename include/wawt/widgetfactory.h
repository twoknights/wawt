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

Widget checkBox(Trackee&&                      indirect,
                const Layout&                  layout,
                StringView_t                   string,
                CharSizeGroup                  group     = CharSizeGroup(),
                TextAlign                      alignment = TextAlign::eLEFT);

Widget checkBox(const Layout&                  layout,
                StringView_t                   string,
                CharSizeGroup                  group      = CharSizeGroup(),
                TextAlign                      alignment  = TextAlign::eLEFT);

Widget dropDownList(Trackee&&                  indirect,
                    Layout                     listLayout,
                    GridFocusCb&&              selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels);

Widget dropDownList(const Layout&              listLayout,
                    GridFocusCb&&              selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels);

Widget fixedSizeList(Trackee&&                 indirect,
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

Widget label(Trackee&&                         indirect,
             const Layout&                     layout,
             StringView_t                      string,
             CharSizeGroup                     group     = CharSizeGroup(),
             TextAlign                         alignment = TextAlign::eCENTER);

Widget label(const Layout&                     layout,
             StringView_t                      string,
             CharSizeGroup                     group     = CharSizeGroup(),
             TextAlign                         alignment = TextAlign::eCENTER);

Widget label(Trackee&&                         indirect,
             const Layout&                     layout,
             StringView_t                      string,
             TextAlign                         alignment);

Widget label(const Layout&                     layout,
             StringView_t                      string,
             TextAlign                         alignment);

Widget panel(Trackee&&                         indirect,
             const Layout&                     layout,
             std::any                          options = std::any());

Widget panel(const Layout&                     layout,
             std::any                          options = std::any());

inline
Widget panel() {
    return Widget(WawtEnv::sPanel, Trackee(), Layout());
}

template<typename... WIDGET>
Widget panelLayout(Trackee&&                   indirect,
                   const Layout&               layoutPanel,
                   const LayoutGenerator&      generator,
                   WIDGET&&...                 widgets)
{
    auto container = panel(std::move(indirect), layoutPanel);
    (container.addChild(std::move(widgets).layout(generator())),...);
    return container;                                                 // RETURN
}

template<typename... WIDGET>
Widget panelLayout(const Layout&               layoutPanel,
                   const LayoutGenerator&      generator,
                   WIDGET&&...                 widgets)
{
    return panelLayout(Trackee(), layoutPanel, generator,
                       std::forward<WIDGET>(widgets)...);             // RETURN
}

template<typename... WIDGET>
Widget panelLayout(Trackee&&                    indirect,
                   const Layout&                layoutPanel,
                   double                       widgetBorder,
                   int                          columns,
                   WIDGET&&...                  widgets)
{
    auto layoutFn  = gridLayoutGenerator(widgetBorder,
                                         sizeof...(widgets),
                                         columns);
    auto grid = panel(std::move(indirect), layoutPanel);
    (grid.addChild(std::move(widgets).layout(layoutFn())),...);
    return grid;                                                      // RETURN
}

template<typename... WIDGET>
Widget panelLayout(const Layout&                layoutPanel,
                   double                       widgetBorder,
                   int                          columns,
                   WIDGET&&...                  widgets)
{
    return panelLayout(Trackee(), layoutPanel, widgetBorder, columns,
                      std::forward<WIDGET>(widgets)...);               // RETURN
}

Widget pushButton(Trackee&&                    indirect,
                  const Layout&                buttonLayout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  CharSizeGroup                group     = CharSizeGroup(),
                  TextAlign                    alignment = TextAlign::eCENTER);

Widget pushButton(const Layout&                buttonLayout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  CharSizeGroup                group     = CharSizeGroup(),
                  TextAlign                    alignment = TextAlign::eCENTER);

Widget pushButton(Trackee&&                    indirect,
                  const Layout&                buttonLayout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  TextAlign                    alignment);

Widget pushButton(const Layout&                buttonLayout,
                  FocusChgCb                   clicked,
                  StringView_t                 string,
                  TextAlign                    alignment);

Widget pushButtonGrid(Trackee&&                indirect,
                      Layout                   gridLayout,
                      int                      columns,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      TextAlign                alignment,
                      FocusChgLabelList        buttonDefs,
                      bool                     spaced    = true);

Widget pushButtonGrid(const Layout&            gridLayout,
                      int                      columns,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      TextAlign                alignment,
                      FocusChgLabelList        buttonDefs,
                      bool                     spaced    = true);

Widget pushButtonGrid(Trackee&&                indirect,
                      const Layout&            gridLayout,
                      int                      columns,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      FocusChgLabelList        buttonDefs,
                      bool                     spaced    = true);

Widget pushButtonGrid(const Layout&            gridLayout,
                      int                      columns,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      FocusChgLabelList        buttonDefs,
                      bool                     spaced    = true);

Widget pushButtonGrid(Trackee&&                indirect,
                      const Layout&            gridLayout,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      FocusChgLabelList        buttonDefs,
                      bool                     spaced    = true);

Widget pushButtonGrid(const Layout&            gridLayout,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      FocusChgLabelList        buttonDefs,
                      bool                     spaced    = true);

Widget radioButtonPanel(Trackee&&              indirect,
                        const Layout&          panelLayout,
                        const GridFocusCb&     gridCb,
                        CharSizeGroup          group,
                        TextAlign              alignment,
                        LabelList              labels,
                        int                    columns   = 1);

Widget radioButtonPanel(const Layout&          panelLayout,
                        const GridFocusCb&     gridCb,
                        CharSizeGroup          group,
                        TextAlign              alignment,
                        LabelList              labels,
                        int                    columns   = 1);

Widget radioButtonPanel(Trackee&&              indirect,
                        const Layout&          panelLayout,
                        const GridFocusCb&     gridCb,
                        CharSizeGroup          group,
                        LabelList              labels,
                        int                    columns   = 1);

Widget radioButtonPanel(const Layout&          panelLayout,
                        const GridFocusCb&     gridCb,
                        CharSizeGroup          group,
                        LabelList              labels,
                        int                    columns   = 1);

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
