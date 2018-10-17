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

#include "wawt/wawtenv.h"

#include "wawt/layout.h"
#include "wawt/draw.h"

#include <cassert>
#include <deque>
#include <initializer_list>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Wawt {

                                //=============
                                // class Widget
                                //=============

enum class  TextAlign { eINVALID, eLEFT, eCENTER, eRIGHT };
struct      CharSizeGroup : std::optional<uint16_t> { };

inline constexpr CharSizeGroup operator ""_F(unsigned long long int n) {
    return CharSizeGroup{n};
}

class  Widget final {
  public:
    using Children          = std::deque<Widget>;
    using ChildrenPtr       = std::unique_ptr<Children>;
    using OptionsFactory    = std::function<std::any(const std::string&)>;
    using BorderDefaults    = std::function<double(const std::string&)>;
    using CharSizeMap       = std::map<uint16_t, uint16_t>;
    using CharSizeMapPtr    = std::shared_ptr<CharSizeMap>;

    struct LayoutData {
        Layout              d_layout{};
        TextAlign           d_textAlign     = TextAlign::eCENTER;
        CharSizeGroup       d_charSizeGroup{};
        CharSizeMapPtr      d_charSizeMap{};
        bool                d_refreshBounds = false;

        LayoutData()                        = default;
        LayoutData(Layout&& layout) noexcept : d_layout(std::move(layout)) { }
    };

    using DownEventMethod
        = std::function<EventUpCb(double    x,
                                  double    y,
                                  Widget   *widget)>;

    using DrawMethod
        = std::function<void(Widget *widget, DrawProtocol *adapter)>;

    using LayoutMethod
        = std::function<void(Widget                  *widget,
                             const Widget&            parent,
                             const Widget&            root,
                             bool                     firstPass,
                             DrawProtocol            *adapter)>;

    using NewChildMethod
        = std::function<void(Widget* parent, Widget* newChild)>;

    using SerializeMethod
        = std::function<void(std::ostream&      os,
                             std::string       *closeTag,
                             const Widget&      widget,
                             unsigned int       indent)>;

    // PUBLIC CLASS METHODS
    static void      defaultDraw(Widget *widget, DrawProtocol *adapter);

    static void      defaultLayout(Widget                  *widget,
                                   const Widget&            parent,
                                   const Widget&            root,
                                   bool                     firstPass,
                                   DrawProtocol            *adapter) noexcept;

    static void     defaultSerialize(std::ostream&      os,
                                     std::string       *closeTag,
                                     const Widget&      widget,
                                     unsigned int       indent);


    // PUBLIC CONSTRUCTORS
    Widget()                                = delete;

    Widget(const Widget&)                   = delete;

    Widget& operator=(const Widget&)        = delete;

    Widget(Widget&& copy)                                            noexcept;

    Widget& operator=(Widget&& rhs)                                  noexcept;

    Widget(char const * const className,
           Widget           **indirect,
           Layout&&           layout)                                noexcept
        : d_widgetLabel(indirect)
        , d_drawData(className)
        , d_layoutData(std::move(layout)) {
            if (d_widgetLabel) *d_widgetLabel = this;
        }

    Widget(char const * const className, Layout&& layout)            noexcept
        : d_drawData(className)
        , d_layoutData(std::move(layout)) { }

    // PUBLIC DESTRUCTOR
    ~Widget() noexcept;

    // PUBLIC R-Value MANIPULATORS

    Widget addChild(Widget&& child) &&;

    Widget addMethod(DownEventMethod&& method) &&                    noexcept;
    Widget addMethod(DrawMethod&&      method) &&                    noexcept;
    Widget addMethod(LayoutMethod&&    method) &&                    noexcept;
    Widget addMethod(NewChildMethod&&  method) &&                    noexcept;
    Widget addMethod(SerializeMethod&& method) &&                    noexcept;

    Widget className(char const * const className) &&                noexcept {
        d_drawData.d_className = className;
        return std::move(*this);
    }

    Widget labelSelect(bool setting) &&                              noexcept {
        d_textHit = setting;
        return std::move(*this);
    }

    Widget layout(Layout&& newLayout) &&                             noexcept {
        d_layoutData.d_layout = std::move(newLayout);
        return std::move(*this);
    }

    Widget options(std::any options) &&                              noexcept {
        d_drawData.d_options = std::move(options);
        return std::move(*this);
    }

    Widget text(StringView_t   string,
                CharSizeGroup  group     = CharSizeGroup(),
                TextAlign      alignment = TextAlign::eCENTER) &&    noexcept;

    Widget text(StringView_t string, TextAlign alignment) &&         noexcept {
        return std::move(*this).text(string, CharSizeGroup(), alignment);
    }

    Widget textMark(DrawData::BulletMark  mark,
                    bool                  leftMark = true) &&        noexcept {
        d_drawData.d_labelMark = mark;
        d_drawData.d_leftMark  = leftMark;
        return std::move(*this);
    }

    // PUBLIC MANIPULATORS

    Widget&   addChild(Widget&& child) &;

    uint16_t  assignWidgetIds(uint16_t         next       = 1,
                              uint16_t         relativeId = 0,
                              CharSizeMapPtr   mapPtr     = nullptr,
                              Widget          *root       = nullptr) noexcept;

    Children& children()                                             noexcept;

    void      clearTrackingPointer()                                 noexcept {
        if (d_widgetLabel) {
            *d_widgetLabel = nullptr;
            d_widgetLabel  = nullptr;
        }
    }

    EventUpCb downEvent(double x, double y);

    void      draw(DrawProtocol *adapter = WawtEnv::drawAdapter())   noexcept;

    DrawData& drawData()                                             noexcept {
        return d_drawData;
    }

    LayoutData& layoutData()                                         noexcept {
        return d_layoutData;
    }

    void      popDialog();

    WidgetId  pushDialog(Widget&&       child,
                         DrawProtocol  *adapter = WawtEnv::drawAdapter());

    void      resetLabel(StringView_t newLabel, bool copy = true);

    void      resizeScreen(double         width,
                           double         height,
                           DrawProtocol  *adapter = WawtEnv::drawAdapter());

    Widget   *screen()                                               noexcept {
        return d_root;                    
    }

    Widget&   setDisabled(bool setting) &                            noexcept {
        d_drawData.d_disableEffect = setting;
        return *this;
    }

    Widget&   setHidden(bool setting)   &                            noexcept {
        d_drawData.d_hidden = setting;
        return *this;
    }

    Widget&   setMethod(DownEventMethod&& method) &                  noexcept;
    Widget&   setMethod(DrawMethod&&      method) &                  noexcept;
    Widget&   setMethod(LayoutMethod&&    method) &                  noexcept;
    Widget&   setMethod(NewChildMethod&&  method) &                  noexcept;
    Widget&   setMethod(SerializeMethod&& method) &                  noexcept;

    Widget&   setLabelSelect(bool setting)        &                  noexcept {
        d_textHit = setting;
        return *this;
    }

    Widget&   setOptions(std::any options)        &                  noexcept {
        d_drawData.d_options = std::move(options);
        return *this;
    }

    // PUBLIC ACCESSORS

    const Children& children()                                 const noexcept;

    const char     *className()                                const noexcept {
        return d_drawData.d_className;
    }

    Widget          clone()                                    const;

    const DrawData& drawData()                                 const noexcept {
        return d_drawData;
    }

    template<class Method>
    Method          getInstalled()                             const noexcept;

    bool            hasChildren()                              const noexcept {
        return d_children && !d_children->empty();
    }

    bool            hasLabelSelect()                           const noexcept {
        return d_textHit;
    }

    bool            inside(double x, double y)                 const noexcept {
        return d_textHit ? d_drawData.d_labelBounds.inside(x, y)
                         : d_drawData.d_rectangle.inside(x, y);
    }

    bool            isDisabled()                               const noexcept {
        return d_drawData.d_disableEffect;
    }

    bool            isHidden()                                 const noexcept {
        return d_drawData.d_hidden;
    }

    const LayoutData& layoutData()                             const noexcept {
        return d_layoutData;
    }

    const Widget   *lookup(WidgetId id)                        const noexcept;

    uint16_t        relativeId()                               const noexcept {
        return d_drawData.d_relativeId;
    }

    const Widget   *screen()                                   const noexcept {
        return d_root;                    
    }

    void            serialize(std::ostream&     os,
                              unsigned int      indent = 0)    const noexcept;

    WidgetId::IdType widgetIdValue()                           const noexcept {
        return d_drawData.d_widgetId;
    }

  private:
    // PRIVATE TYPES
    struct Methods {
        DrawMethod          d_drawMethod;
        LayoutMethod        d_layoutMethod;
        NewChildMethod      d_newChildMethod;
        SerializeMethod     d_serializeMethod;
    };
    using MethodsPtr = std::unique_ptr<Methods>;

    // PRIVATE METHODS
    template <typename Callable, typename R, typename... Args>
    void call(Callable *defaultFn, R(Methods::*fn), Args&&... args) const {
        if (d_methods && (*d_methods.get()).*fn) {
            ((*d_methods.get()).*fn)(std::forward<Args>(args)...);
        }
        else {
            defaultFn(std::forward<Args>(args)...);
        }
    }

    void layout(DrawProtocol       *adapter,
                bool                firstPass,
                const Widget&       parent);

    // PRIVATE DATA MEMBERS
    Widget            **d_widgetLabel   = nullptr;
    Widget             *d_root          = nullptr;
    bool                d_textHit       = false;
    DownEventMethod     d_downMethod{};
    MethodsPtr          d_methods{};
    DrawData            d_drawData{};
    LayoutData          d_layoutData{};

    // The ordering of widgets should not be changed during execution
    // EXCEPT for appending widgets to the "root" panel. This allows for
    // the showing of a pop-up (e.g. drop-down list).  This must NOT result
    // in the widget being moved (e.g. vector resizing) since that operation
    // would be performed by a callback whose closure would be contained
    // in a moved widget.  This can cause errors when the callback returns.
    ChildrenPtr         d_children{};

};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
