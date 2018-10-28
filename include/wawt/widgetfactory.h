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

struct LabelDef {
    StringView_t    d_string{};
    CharSizeGroup   d_group{};
};

using OnClickCb         = std::function<void(Widget *)>;
using ClickLabelPair    = std::pair<OnClickCb, StringView_t>;
using ClickLabelList    = std::initializer_list<ClickLabelPair>;
using GroupClickCb      = std::function<void(Widget *,uint16_t relativeId)>;
using LabelList         = std::initializer_list<StringView_t>;
using LabelDefList      = std::initializer_list<LabelDef>;
using WidgetGenerator   = std::function<Widget(int row, int column)>;

Widget checkBox(Trackee&&                      indirect,
                const Layout&                  layout,
                StringView_t                   string,
                CharSizeGroup                  group     = CharSizeGroup(),
                TextAlign                      alignment = TextAlign::eLEFT);

Widget checkBox(const Layout&                  layout,
                StringView_t                   string,
                CharSizeGroup                  group      = CharSizeGroup(),
                TextAlign                      alignment  = TextAlign::eLEFT);

Widget dialogBox(Trackee&&                     indirect,
                 const Layout&                 dialogLayout,
                 Widget&&                      buttons,
                 LabelDefList                  dialog);

Widget dialogBox(const Layout&                 dialogLayout,
                 Widget&&                      buttons,
                 LabelDefList                  dialog);

Widget dropDownList(Trackee&&                  indirect,
                    Layout                     listLayout,
                    GroupClickCb&&             selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels);

Widget dropDownList(const Layout&              listLayout,
                    GroupClickCb&&             selectCb,
                    CharSizeGroup              group,
                    LabelList                  labels);

Widget fixedSizeList(Trackee&&                 indirect,
                     const Layout&             listLayout,
                     bool                      singleSelect,
                     const GroupClickCb&       selectCb,
                     CharSizeGroup             group,
                     LabelList                 labels);

Widget fixedSizeList(const Layout&             listLayout,
                     bool                      singleSelect,
                     const GroupClickCb&       selectCb,
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
                  OnClickCb                    clicked,
                  StringView_t                 string,
                  CharSizeGroup                group     = CharSizeGroup(),
                  TextAlign                    alignment = TextAlign::eCENTER);

Widget pushButton(const Layout&                buttonLayout,
                  OnClickCb                    clicked,
                  StringView_t                 string,
                  CharSizeGroup                group     = CharSizeGroup(),
                  TextAlign                    alignment = TextAlign::eCENTER);

Widget pushButton(Trackee&&                    indirect,
                  const Layout&                buttonLayout,
                  OnClickCb                    clicked,
                  StringView_t                 string,
                  TextAlign                    alignment);

Widget pushButton(const Layout&                buttonLayout,
                  OnClickCb                    clicked,
                  StringView_t                 string,
                  TextAlign                    alignment);

Widget pushButtonGrid(Trackee&&                indirect,
                      Layout                   gridLayout,
                      int                      columns,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      TextAlign                alignment,
                      ClickLabelList           buttonDefs,
                      bool                     spaced    = true);

Widget pushButtonGrid(const Layout&            gridLayout,
                      int                      columns,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      TextAlign                alignment,
                      ClickLabelList           buttonDefs,
                      bool                     spaced    = true);

Widget pushButtonGrid(Trackee&&                indirect,
                      const Layout&            gridLayout,
                      int                      columns,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      ClickLabelList           buttonDefs,
                      bool                     spaced    = true);

Widget pushButtonGrid(const Layout&            gridLayout,
                      int                      columns,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      ClickLabelList           buttonDefs,
                      bool                     spaced    = true);

Widget pushButtonGrid(Trackee&&                indirect,
                      const Layout&            gridLayout,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      ClickLabelList           buttonDefs,
                      bool                     spaced    = true);

Widget pushButtonGrid(const Layout&            gridLayout,
                      double                   borderThickness,
                      CharSizeGroup            group,
                      ClickLabelList           buttonDefs,
                      bool                     spaced    = true);

Widget radioButtonPanel(Trackee&&              indirect,
                        const Layout&          panelLayout,
                        const GroupClickCb&    gridCb,
                        CharSizeGroup          group,
                        TextAlign              alignment,
                        LabelList              labels,
                        int                    columns   = 1);

Widget radioButtonPanel(const Layout&          panelLayout,
                        const GroupClickCb&    gridCb,
                        CharSizeGroup          group,
                        TextAlign              alignment,
                        LabelList              labels,
                        int                    columns   = 1);

Widget radioButtonPanel(Trackee&&              indirect,
                        const Layout&          panelLayout,
                        const GroupClickCb&    gridCb,
                        CharSizeGroup          group,
                        LabelList              labels,
                        int                    columns   = 1);

Widget radioButtonPanel(const Layout&          panelLayout,
                        const GroupClickCb&    gridCb,
                        CharSizeGroup          group,
                        LabelList              labels,
                        int                    columns   = 1);

Widget widgetGrid(Trackee&&                    indirect,
                  const Layout&                layoutPanel,
                  int                          rows,
                  int                          columns,
                  const WidgetGenerator&       generator,
                  bool                         spaced = false);

Widget widgetGrid(const Layout&                layoutPanel,
                  int                          rows,
                  int                          columns,
                  const WidgetGenerator&       generator,
                  bool                         spaced = false);

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
