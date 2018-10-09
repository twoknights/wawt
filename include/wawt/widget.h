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
#include <limits>
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
                             const Children&    widgets)>;

    using LayoutMethod
        = std::function<void(DrawData                *data,
                             Text::CharSizeMap       *charSizeMap,
                             bool                     firstPass,
                             const Widget&            root,
                             const Widget&            parent,
                             const LayoutData&        layoutData,
                             DrawProtocol            *adapter)>;

    using PopMethod
        = std::function<void(Children*)>;

    using PushMethod
        = std::function<bool(Children*, Widget&&)>;

    using SerializeMethod
        = std::function<void(std::ostream&      os,
                             std::string       *closeTag,
                             const DrawData&    drawData,
                             const LayoutData&  layoutData,
                             unsigned int       indent)>;

    // PUBLIC CLASS METHODS
    static void      defaultDraw(DrawProtocol      *adapter,
                                 const DrawData&    data,
                                 const Children&    children);

    static void      defaultLayout(DrawData                *data,
                                   Text::CharSizeMap       *charSizeMap,
                                   bool                     firstPass,
                                   const Widget&            root,
                                   const Widget&            parent,
                                   const LayoutData&        layoutData,
                                   DrawProtocol            *adapter);

    static void     defaultPop(Children *children);

    static bool     defaultPush(Children *children, Widget&& child);

    static void     defaultSerialize(std::ostream&      os,
                                     std::string       *closeTag,
                                     const DrawData&    drawData,
                                     const LayoutData&  layoutData,
                                     unsigned int       indent);


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
        , d_layoutMethod(std::move(method))
        , d_drawData(std::move(className))
        , d_layoutData(std::move(layout)) { }

    Widget(Layout&&         layout,
           std::string      className,
           LayoutMethod&&   method = LayoutMethod(&Widget::defaultLayout))
                                                                     noexcept
        : d_layoutMethod(std::move(method))
        , d_drawData(std::move(className))
        , d_layoutData(std::move(layout)) { }

    // PUBLIC R-Value MANIPULATORS

    Widget drawMethod(DrawMethod&& method) &&                        noexcept {
        d_drawMethod = std::move(method);
        return std::move(*this);
    }

    Widget options(std::any options) &&                              noexcept {
        d_drawData.d_options = std::move(options);
        return std::move(*this);
    }

    Widget popMethod(PopMethod&& method) &&                          noexcept {
        d_popMethod = std::move(method);
        return std::move(*this);
    }

    Widget push(Widget&& child) && {
        d_pushMethod(&d_children, std::move(child));
        return std::move(*this);
    }

    Widget push(Children&& children) && {
        for (auto& child : children) {
            d_pushMethod(&d_children, std::move(child));
        }
        return std::move(*this);
    }

    Widget pushMethod(PushMethod&& method) &&                        noexcept {
        d_pushMethod = std::move(method);
        return std::move(*this);
    }

    Widget serializeMethod(SerializeMethod&& method) &&              noexcept {
        d_serialize = std::move(method);
        return std::move(*this);
    }

    Widget text(const Text&             textInfo,
                DrawData::BulletMark    mark = DrawData::BulletMark::eNONE) &&
                                                                     noexcept {
        d_drawData.d_labelMark          = mark;
        d_layoutData.d_charSizeGroup    = textInfo.d_charSizeGroup;
        d_layoutData.d_textAlign        = textInfo.d_alignment;
        d_drawData.d_label              = textInfo.d_stringView;
        return std::move(*this);
    }

    // PUBLIC MANIPULATORS

    WidgetId  assignWidgetIds(WidgetId next, Widget *root = nullptr) noexcept;

    void      popDialog();

    WidgetId  pushDialog(Widget&& child, DrawProtocol *adapter);

    void      rootLayout(DrawProtocol *adapter, double width, double height);

    WidgetId& widgetId()                                             noexcept {
        return d_drawData.d_widgetId;
    }

    // PUBLIC ACCESSORS

    const Children& children()                                 const noexcept {
        return d_children;
    }

    void            draw(DrawProtocol *adapter)                const noexcept {
        // TBD: enablement check
        d_drawMethod(adapter, d_drawData, d_children);
    }

    const DrawData& drawData()                                 const noexcept {
        return d_drawData;
    }

    const Widget   *lookup(WidgetId id)                        const noexcept;

    void            serialize(std::ostream&     os,
                              unsigned int      indent = 0)    const noexcept {
        auto closeTag = std::string{};
        d_serialize(os, &closeTag, d_drawData, d_layoutData, indent);
        indent += 2;

        for (auto& child : d_children) {
            child.serialize(os, indent);
        }
        os << closeTag;
    }

    WidgetId        widgetId()                                 const noexcept {
        return d_drawData.d_widgetId;
    }

  private:
    // PRIVATE METHODS
    void layout(Text::CharSizeMap  *charSizeMap,
                DrawProtocol       *adapter,
                bool                firstPass,
                const Widget&       parent) {
        d_layoutMethod(&d_drawData,
                        charSizeMap,
                        firstPass,
                        *d_root,
                        parent,
                        d_layoutData,
                        adapter);

        for (auto& child : d_children) {
            child.layout(charSizeMap, adapter, firstPass, *this);
        }
    }

    // PRIVATE DATA MEMBERS
    Widget            **d_widgetLabel   = nullptr;
    Widget             *d_root          = nullptr;
    DrawMethod          d_drawMethod    = DrawMethod(&Widget::defaultDraw);
    LayoutMethod        d_layoutMethod  = LayoutMethod(&Widget::defaultLayout);
    PopMethod           d_popMethod     = PopMethod(&Widget::defaultPop);
    PushMethod          d_pushMethod    = PushMethod(&Widget::defaultPush);
    SerializeMethod     d_serialize     = SerializeMethod(&Widget
                                                           ::defaultSerialize);

    DrawData            d_drawData{};
    LayoutData          d_layoutData{};

    // The ordering of widgets should not be changed during execution
    // EXCEPT for appending widgets to the "root" panel. This allows for
    // the showing of a pop-up (e.g. drop-down list).  This must NOT result
    // in the widget being moved (e.g. vector resizing) since that operation
    // would be performed by a callback whose closure would be contained
    // in a moved widget.
    Children            d_children{};

};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
