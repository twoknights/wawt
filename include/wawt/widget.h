/** @file widget.h
 *  @brief Widget class for widgets in the Wawt framework.
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

#ifndef WAWT_WIDGET_H
#define WAWT_WIDGET_H

#include "wawt/wawt.h"

#include "wawt/layout.h"
#include "wawt/text.h"
#include "wawt/draw.h"

#include <cassert>
#include <deque>
#include <initializer_list>
#include <numeric_limits>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Wawt {

                                //=============
                                // class Widget
                                //=============

class  Widget final {
  public:
    using Children          = std::deque<Widget>;
    using OptionsFactory    = std::function<std::any(const std::string&)>;
    using BorderDefaults    = std::function<double(const std::string&)>;

    struct DrawData {
        enum class BulletMark { eNONE, eSQUAREBOX, eROUNDBOX };

        WidgetId            d_widgetId{};
        Rectangle           d_rectangle{};
        Rectangle           d_labelBounds{};
        StringView_t        d_label{};
        BulletMark          d_labelMark         = BulletMark::eNONE;
        bool                d_selected          = false;
        bool                d_disableEffect     = false;
        std::any            d_options{};
        std::string         d_className{};

        DrawData()                          = default;

        DrawData(std::string&& className) noexcept
            : d_className(std::move(className)) { }
    };

    struct LayoutData {
        Layout              d_layout{};
        Text::Align         d_textAlign     = Text::Align::eCENTER;
        Text::CharSizeGroup d_charSizeGroup{};

        LayoutData()                        = default;
        LayoutData(Layout&& layout) noexcept : d_layout(std::move(layout)) { }
    };

    using DrawMethod
        = std::function<void(DrawProtocol      *adapter,
                             const DrawData&    data,
                             const Children&    widgets);

    using LayoutMethod
        = std::function<void(DrawData                *data,
                             Text::CharSizeMap       *charSizeMap,
                             const Widget&            root,
                             const Widget&            parent,
                             const Children&          children,
                             const LayoutData&        layoutData,
                             const OptionsFactory&    factory,
                             const BorderDefaults&    borderDefaults,
                             DrawProtocol            *adapter);

    using SerializeMethod
        = std::function<void(std::ostream&      os,
                             const DrawData&    drawData,
                             const LayoutData&  layoutData,
                             const Children&    children,
                             unsigned int       indent)>;

    // PUBLIC CONSTRUCTORS
    Widget()                                = default;

    Widget& operator=(const Widget& rhs)    = delete;

    Widget(const Widget& copy); // ONLY usable in initializer lists

    Widget(Widget&& copy)           noexcept;

    Widget& operator=(Widget&& rhs) noexcept;

    Widget(Widget         **indirect,
           std::string      className,
           Layout&&         layout,
           LayoutMethod&&   method = LayoutMethod(&Widget::defaultLayout))
                                                                     noexcept
        : d_widgetLabel(indirect)
        , d_drawData(std::move(className))
        , d_layoutData(std::move(layout))
        , d_layoutMethod(std::move(method)) { }

    Widget(Layout&&         layout,
           std::string      className,
           LayoutMethod&&   method = LayoutMethod(&Widget::defaultLayout))
                                                                     noexcept
        : d_drawData(std::move(className))
        , d_layoutData(std::move(layout))
        , d_layoutMethod(std::move(method)) { }

    // PUBLIC MANIPULATORS

    WidgetId assignWidgetIds(WidgetId next, Widget *root)            noexcept;

    Widget drawMethod(DrawMethod&& method) &&                        noexcept {
        d_drawMethod = std::move(method);
        return *this;
    }

    Widget options(std::any options) &&                              noexcept {
        d_drawData.d_options = std::move(options);
        return *this;
    }

    Widget serializeMethod(SerializeMethod&& method) &&              noexcept {
        d_serialize = std::move(method);
        return *this;
    }

    Widget text(Text&&          textInfo,
                BulletMark      mark = BulletMark::eNONE)            noexcept {
        d_drawData.d_labelMark          = mark;
        d_layoutData.d_charSizeGroup    = textInfo.d_charSizeGroup;
        d_layoutData.d_textAlign        = textInfo.d_textAlign;
        d_drawData.d_label              = std::move(textInfo.d_string);
        return *this;
    }

    // PUBLIC ACCESSORS

    const Children& children()                                 const noexcept {
        return d_widgets;
    }

    const Widget* lookup(const WidgetId& id)                   const noexcept;

    WidgetId widgetId()                                        const noexcept {
        return d_drawData.d_widgetId;
    }

  private:
    // PRIVATE CLASS METHODS
    static void      defaultDraw(DrawProtocol      *adapter,
                                 const DrawData&    data,
                                 const Children&    widgets);

    static void      defaultLayout(DrawData                *data,
                                   Text::CharSizeMap       *charSizeMap,
                                   const Widget&            root,
                                   const Widget&            parent,
                                   const Children&          children,
                                   const LayoutData&        layoutData,
                                   const OptionsFactory&    factory,
                                   const BorderDefaults&    borderDefaults,
                                   DrawProtocol            *adapter);

    static void     defaultSerialize(std::ostream&      os,
                                     const DrawData&    drawData,
                                     const LayoutData&  layoutData,
                                     const Children&    children,
                                     unsigned int       indent) noexcept;

    // PRIVATE METHODS

    // PRIVATE DATA MEMBERS
    Widget            **d_widgetLabel   = nullptr;
    Widget             *d_root          = nullptr;
    LayoutMethod        d_layoutMethod  = LayoutMethod(&Widget::defaultLayout);
    DrawMethod          d_drawMethod    = DrawMethod(&Widget::defaultDraw);
    SerializeMethod     d_serialize     = SerializeMethod(&Widget
                                                           ::defaultSerialize);

    LayoutData          d_layoutData{};
    DrawData            d_drawData{};

    // The ordering of widgets should not be changed during execution
    // EXCEPT for appending widgets to the "root" panel. This allows for
    // the showing of a pop-up (e.g. drop-down list).  This must NOT result
    // in the widget being moved (e.g. vector resizing) since that operation
    // would be performed by a callback whose closure would be contained
    // in a moved widget.
    Children            d_widgets{};

};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
