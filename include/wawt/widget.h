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

inline constexpr CharSizeGroup operator ""_Sz(unsigned long long int n) {
    return CharSizeGroup{n};
}

constexpr CharSizeGroup kNOGROUP{};

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
        LayoutData(const Layout& layout) noexcept : d_layout(layout) { }
    };

    using DownEventMethod
        = std::function<EventUpCb(double    x,
                                  double    y,
                                  Widget   *widget,
                                  Widget   *parent)>;

    using DrawMethod
        = std::function<void(Widget *widget, DrawProtocol *adapter)>;

    using InputMethod
        = std::function<bool(Widget *widget, Char_t input)>;

    using LayoutMethod
        = std::function<void(Widget                  *widget,
                             const Widget&            parent,
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
                                   bool                     firstPass,
                                   DrawProtocol            *adapter) noexcept;

    static void      defaultSerialize(std::ostream&      os,
                                      std::string       *closeTag,
                                      const Widget&      widget,
                                      unsigned int       indent);

    static void      labelLayout(DrawData          *data,
                                 bool               firstPass,
                                 const LayoutData&  layoutData,
                                 DrawProtocol      *adapter)         noexcept;

    // PUBLIC CONSTRUCTORS
    Widget()                                = delete;

    Widget(const Widget&)                   = delete;

    Widget& operator=(const Widget&)        = delete;

    Widget(Widget&& copy)                                            noexcept;

    Widget& operator=(Widget&& rhs)                                  noexcept;

    Widget(char const * const optionName,
           Trackee&&          indirect,
           const Layout&      layout)                                noexcept
        : d_widgetLabel(std::move(indirect))
        , d_drawData(optionName)
        , d_layoutData(layout) {
            if (d_widgetLabel) d_widgetLabel.update(this);
        }

    Widget(char const * const optionName, const Layout& layout)      noexcept
        : d_drawData(optionName)
        , d_layoutData(layout) { }

    // PUBLIC Ref-Qualified MANIPULATORS

    Widget  addChild(Widget&& child) &&;
    Widget& addChild(Widget&& child) &;

    Widget  border(double thickness) &&                              noexcept {
        d_layoutData.d_layout.border(thickness);
        return std::move(*this);
    }

    Widget& border(double thickness) &                               noexcept {
        d_layoutData.d_layout.border(thickness);
        return *this;
    }

    Widget  optionName(char const * const optionName) &&             noexcept {
        d_drawData.d_optionName = optionName;
        return std::move(*this);
    }

    Widget  disabled(bool setting) &&                                noexcept {
        d_drawData.d_disableEffect = setting;
        return std::move(*this);
    }

    Widget& disabled(bool setting) &                                 noexcept {
        d_drawData.d_disableEffect = setting;
        return *this;
    }

    Widget  hidden(bool setting) &&                                  noexcept {
        d_drawData.d_hidden = setting;
        return std::move(*this);
    }

    Widget& hidden(bool setting) &                                   noexcept {
        d_drawData.d_hidden = setting;
        return *this;
    }

    Widget  labelSelect(bool setting) &&                             noexcept {
        d_textHit = setting;
        return std::move(*this);
    }

    Widget& labelSelect(bool setting) &                              noexcept {
        d_textHit = setting;
        return *this;
    }

    Widget  layout(const Layout& newLayout) &&                       noexcept {
        d_layoutData.d_layout = newLayout;
        return std::move(*this);
    }

    Widget& layout(const Layout& newLayout) &                        noexcept {
        d_layoutData.d_layout = newLayout;
        return *this;
    }

    Widget  method(DownEventMethod&& newMethod) &&                    noexcept;
    Widget  method(DrawMethod&&      newMethod) &&                    noexcept;
    Widget  method(InputMethod&&     newMethod) &&                    noexcept;
    Widget  method(LayoutMethod&&    newMethod) &&                    noexcept;
    Widget  method(NewChildMethod&&  newMethod) &&                    noexcept;
    Widget  method(SerializeMethod&& newMethod) &&                    noexcept;

    Widget& method(DownEventMethod&& newMethod) &                     noexcept;
    Widget& method(DrawMethod&&      newMethod) &                     noexcept;
    Widget& method(InputMethod&&     newMethod) &                     noexcept;
    Widget& method(LayoutMethod&&    newMethod) &                     noexcept;
    Widget& method(NewChildMethod&&  newMethod) &                     noexcept;
    Widget& method(SerializeMethod&& newMethod) &                     noexcept;

    Widget  options(std::any options) &&                             noexcept {
        d_drawData.d_options = std::move(options);
        return std::move(*this);
    }

    Widget& options(std::any options) &                              noexcept {
        d_drawData.d_options = std::move(options);
        return *this;
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

    uint16_t  assignWidgetIds(uint16_t         next       = 1,
                              uint16_t         relativeId = 0,
                              CharSizeMapPtr   mapPtr     = nullptr,
                              Widget          *root       = nullptr) noexcept;

    Children& children()                                             noexcept;

    void      clearTrackingPointer()                                 noexcept {
        d_widgetLabel.clear();
    }

    EventUpCb downEvent(double x, double y, Widget *parent = nullptr);

    void      draw(DrawProtocol *adapter = WawtEnv::drawAdapter())   noexcept;

    DrawData& drawData()                                             noexcept {
        return d_drawData;
    }

    bool inputEvent(Char_t input)                                    noexcept;

    Layout&   layout()                                               noexcept {
        return d_layoutData.d_layout;
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

    void      setFocus(Widget* target = nullptr)                     noexcept;

    void      setSelected(bool setting)                              noexcept {
        d_drawData.d_selected = setting;
    }

    // PUBLIC ACCESSORS

    const Children& children()                                 const noexcept;

    const char     *optionName()                               const noexcept {
        return d_drawData.d_optionName;
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

    bool            hasLabelLayout()                           const noexcept {
        return !d_drawData.d_label.empty()
            || d_drawData.d_labelMark != DrawData::BulletMark::eNONE;
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

    bool            isSelected()                               const noexcept {
        return d_drawData.d_selected;
    }

    const LayoutData& layoutData()                             const noexcept {
        return d_layoutData;
    }

    const Widget   *lookup(WidgetId id)                        const noexcept;

    const std::any& options()                                  const noexcept {
        return d_drawData.d_options;
    }

    uint16_t        relativeId()                               const noexcept {
        return d_drawData.d_relativeId;
    }

    const Widget   *screen()                                   const noexcept {
        return d_root;                    
    }

    Tracker        *tracker()                                  const noexcept {
        return d_widgetLabel.get();
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
        InputMethod         d_inputMethod;
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
    Trackee             d_widgetLabel{};
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
