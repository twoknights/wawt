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

using ClickCb           = std::function<void(Widget *)>;
using ClickLabel        = std::pair<ClickCb, StringView_t>;
using ClickLabelList    = std::initializer_list<ClickLabel>;

using FocusChgCb        = std::function<FocusCb(Widget *)>;
using FocusChgLabel     = std::pair<FocusChgCb, StringView_t>;
using FocusChgLabelList = std::initializer_list<FocusChgLabel>;

using SelectCb          = std::function<void(Widget *,uint16_t relativeId)>;
using SelectFocusCb     = std::function<FocusCb(Widget *,uint16_t relativeId)>;

using LabelList         = std::initializer_list<StringView_t>;

Widget bulletButtonGrid(Widget                **indirect,
                        Layout&&                layout,
                        bool                    radioButtons,
                        SelectCb&&              selectCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        TextAlign               alignment = TextAlign::eLEFT,
                        int                     columns   = 1);

Widget bulletButtonGrid(Layout&&                layout,
                        bool                    radioButtons,
                        SelectCb&&              selectCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        TextAlign               alignment = TextAlign::eLEFT,
                        int                     columns   = 1);

Widget bulletButtonGrid(Widget                **indirect,
                        Layout&&                layout,
                        bool                    radioButtons,
                        SelectFocusCb&&         selectCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        TextAlign               alignment = TextAlign::eLEFT,
                        int                     columns   = 1);

Widget bulletButtonGrid(Layout&&                layout,
                        bool                    radioButtons,
                        SelectFocusCb&&         selectCb,
                        CharSizeGroup           group,
                        LabelList               labels,
                        TextAlign               alignment = TextAlign::eLEFT,
                        int                     columns   = 1);

Widget checkBox(Widget              **indirect,
                Layout&&              layout,
                StringView_t          string,
                CharSizeGroup         group     = CharSizeGroup(),
                TextAlign             alignment = TextAlign::eLEFT);

Widget checkBox(Layout&&              layout,
                StringView_t          string,
                CharSizeGroup         group     = CharSizeGroup(),
                TextAlign            alignment  = TextAlign::eLEFT);

Widget label(Widget               **indirect,
             Layout&&               layout,
             StringView_t           string,
             TextAlign              alignment = TextAlign::eCENTER,
             CharSizeGroup          group     = CharSizeGroup());

Widget label(Layout&&               layout,
             StringView_t           string,
             TextAlign              alignment = TextAlign::eCENTER,
             CharSizeGroup          group     = CharSizeGroup());

Widget label(Widget               **indirect,
             Layout&&               layout,
             StringView_t           string,
             CharSizeGroup          group);

Widget label(Layout&&               layout,
             StringView_t           string,
             CharSizeGroup          group);

Widget panel(Widget **indirect, Layout&& layout);

Widget panel(Layout&& layout = Layout());

Widget panelGrid(Widget       **indirect,
                 Layout&&       layout,
                 int            rows,
                 int            columns,
                 const Widget&  clonable);

Widget panelGrid(Layout&&       layout,
                 int            rows,
                 int            columns,
                 const Widget&  clonable);

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  TextAlign             alignment = TextAlign::eCENTER,
                  CharSizeGroup         group     = CharSizeGroup());

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  TextAlign             alignment = TextAlign::eCENTER,
                  CharSizeGroup         group     = CharSizeGroup());

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  CharSizeGroup         group);

Widget pushButton(Layout&&              layout,
                  ClickCb               clicked,
                  StringView_t          string,
                  CharSizeGroup         group);

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  TextAlign             alignment = TextAlign::eCENTER,
                  CharSizeGroup         group     = CharSizeGroup());

Widget pushButton(Layout&&              layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  TextAlign             alignment = TextAlign::eCENTER,
                  CharSizeGroup         group     = CharSizeGroup());

Widget pushButton(Widget              **indirect,
                  Layout&&              layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  CharSizeGroup         group);

Widget pushButton(Layout&&              layout,
                  FocusChgCb            clicked,
                  StringView_t          string,
                  CharSizeGroup         group);

Widget pushButtonGrid(Widget                **indirect,
                      Layout&&                layout,
                      CharSizeGroup           group,
                      int                     columns,
                      ClickLabelList          buttonDefs,
                      bool                    fitted    = false,
                      TextAlign               alignment = TextAlign::eCENTER);

Widget pushButtonGrid(Layout&&                layout,
                      CharSizeGroup           group,
                      int                     columns,
                      ClickLabelList          buttonDefs,
                      bool                    fitted    = false,
                      TextAlign               alignment = TextAlign::eCENTER);

Widget pushButtonGrid(Widget                **indirect,
                      Layout&&                layout,
                      CharSizeGroup           group,
                      int                     columns,
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted    = false,
                      TextAlign               alignment = TextAlign::eCENTER);

Widget pushButtonGrid(Layout&&                layout,
                      CharSizeGroup           group,
                      int                     columns,
                      FocusChgLabelList       buttonDefs,
                      bool                    fitted    = false,
                      TextAlign               alignment = TextAlign::eCENTER);

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
