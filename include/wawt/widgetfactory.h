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

using ClickCb         = std::function<void(Widget *)>;
using ClickLabel      = std::pair<ClickCb, StringView_t>;
using ClickLabelList  = std::initializer_list<ClickLabel>;

Widget::NewChildMethod genGridParent(bool         spacedLayout,
                                     int          columns,
                                     std::size_t  count,
                                     float        thickness);

Widget checkBox(Widget              **indirect,
                Layout&&              layout,
                StringView_t          string,
                Widget::CharSizeGroup group,
                ClickCb               clicked,
                TextAlign             alignment = TextAlign::eLEFT,
                bool                  leftBox   = true);

Widget checkBox(Layout&&              layout,
                StringView_t          string,
                Widget::CharSizeGroup group,
                ClickCb               clicked,
                TextAlign             alignment = TextAlign::eLEFT,
                bool                  leftBox   = true);

Widget checkBox(Widget              **indirect,
                Layout&&              layout,
                StringView_t          string,
                Widget::CharSizeGroup group     = Widget::CharSizeGroup(),
                TextAlign             alignment = TextAlign::eLEFT);

Widget checkBox(Layout&&              layout,
                StringView_t          string,
                Widget::CharSizeGroup group     = Widget::CharSizeGroup(),
                TextAlign            alignment  = TextAlign::eLEFT);

Widget label(Widget               **indirect,
             Layout&&               layout,
             StringView_t           string,
             TextAlign              alignment = TextAlign::eCENTER,
             Widget::CharSizeGroup  group     = Widget::CharSizeGroup());

Widget label(Layout&&               layout,
             StringView_t           string,
             TextAlign              alignment = TextAlign::eCENTER,
             Widget::CharSizeGroup  group     = Widget::CharSizeGroup());

Widget label(Widget               **indirect,
             Layout&&               layout,
             StringView_t           string,
             Widget::CharSizeGroup  group);

Widget label(Layout&&               layout,
             StringView_t           string,
             Widget::CharSizeGroup  group);

Widget panel(Widget **indirect, Layout&& layout);

Widget panel(Layout&& layout = Layout());

Widget panelGrid(Widget   **indirect,
                 Layout&&   layout,
                 int        rows,
                 int        columns,
                 Widget&&   clonable);

Widget panelGrid(Layout&& layout, int rows, int columns, Widget&& clonable);

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  TextAlign             alignment = TextAlign::eCENTER,
                  Widget::CharSizeGroup group     = Widget::CharSizeGroup());

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  TextAlign             alignment = TextAlign::eCENTER,
                  Widget::CharSizeGroup group     = Widget::CharSizeGroup());

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Widget::CharSizeGroup group);

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  Widget::CharSizeGroup group);

Widget pushButtonGrid(Widget                **indirect,
                      Layout&&                layout,
                      bool                    spacedLayout,
                      int                     columns,
                      TextAlign               alignment,
                      Widget::CharSizeGroup   group,
                      ClickLabelList          buttonDefs);

Widget pushButtonGrid(Layout&&                layout,
                      bool                    spacedLayout,
                      int                     columns,
                      TextAlign               alignment,
                      Widget::CharSizeGroup   group,
                      ClickLabelList          buttonDefs);

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
