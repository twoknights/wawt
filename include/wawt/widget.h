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
#include "wawt/text.h"

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

class DrawProtocol;

class  Widget final {
  public:

                            //========================
                            // struct Widget::Settings
                            //========================

    struct Settings {
        const char         *d_optionName        = nullptr;
        std::any            d_options{};
        uint16_t            d_widgetIdValue     = 0;
        uint16_t            d_selected:1,
                            d_disabled:1,
                            d_hidden:1,
                            d_hideSelect:1,
                            d_successfulLayout:1,
                            d_relativeId:11;

        Settings()              = default;

        Settings(char const * const optionName)
            : d_optionName(optionName)
            , d_selected(false)
            , d_disabled(false)
            , d_hidden(false)
            , d_hideSelect(false)
            , d_successfulLayout(false)
            , d_relativeId(0) { }
    };

    // PUBLIC TYPES
    using Children          = std::deque<Widget>;
    using ChildrenPtr       = std::unique_ptr<Children>;
    using OptionsFactory    = std::function<std::any(const std::string&)>;
    using BorderDefaults    = std::function<double(const std::string&)>;
    using CharSizeMapPtr    = Text::CharSizeMapPtr;

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

    // PUBLIC CONSTRUCTORS
    Widget()                                = delete;

    Widget(const Widget&)                   = delete;

    Widget& operator=(const Widget&)        = delete;

    Widget(Widget&& move)                                            noexcept;

    Widget& operator=(Widget&& rhs)                                  noexcept;

    Widget(char const * const optionName,
           Trackee&&          indirect,
           const Layout&      layout)                                noexcept
        : d_widgetLabel(std::move(indirect))
        , d_root(nullptr)
        , d_downMethod()
        , d_methods()
        , d_layout(layout)
        , d_rectangle()
        , d_settings(optionName)
        , d_text() {
            if (d_widgetLabel) d_widgetLabel.update(this);
        }

    Widget(char const * const optionName, const Layout& layout)      noexcept
        : Widget(optionName, Trackee(), layout) { }

    // PUBLIC Ref-Qualified MANIPULATORS

    Widget  addChild(Widget&& child) &&;
    Widget& addChild(Widget&& child) &;

    Widget  border(double thickness) &&                              noexcept {
        d_layout.border(thickness);
        return std::move(*this);
    }

    Widget& border(double thickness) &                               noexcept {
        d_layout.border(thickness);
        return *this;
    }

    Widget  charSizeGroup(CharSizeGroup group) &&                    noexcept {
        this->charSizeGroup(group);
        return std::move(*this);
    }

    Widget& charSizeGroup(CharSizeGroup group) &                     noexcept {
        text().d_layout.d_charSizeGroup = group;
        return *this;
    }

    Widget  disabled(bool setting) &&                                noexcept {
        d_settings.d_disabled = setting;
        return std::move(*this);
    }

    Widget& disabled(bool setting) &                                 noexcept {
        d_settings.d_disabled = setting;
        return *this;
    }

    Widget  downEventMethod(DownEventMethod&& newMethod) &&          noexcept;

    Widget& downEventMethod(DownEventMethod&& newMethod) &           noexcept;

    Widget  drawMethod(DrawMethod&&           newMethod) &&          noexcept;

    Widget& drawMethod(DrawMethod&&           newMethod) &           noexcept;

    Widget  hidden(bool setting) &&                                  noexcept {
        d_settings.d_hidden = setting;
        return std::move(*this);
    }

    Widget& hidden(bool setting) &                                   noexcept {
        d_settings.d_hidden = setting;
        return *this;
    }

    Widget  horizontalAlign(TextAlign alignment) &&                  noexcept {
        this->horizontalAlign(alignment);
        return std::move(*this);
    }

    Widget& horizontalAlign(TextAlign alignment) &                   noexcept {
        text().d_layout.d_horizontalAlign
                                    = static_cast<unsigned int>(alignment);
        return *this;
    }

    Widget  inputMethod(InputMethod&&         newMethod) &&          noexcept;

    Widget& inputMethod(InputMethod&&         newMethod) &           noexcept;

    Widget  layout(const Layout& newLayout) &&                       noexcept {
        d_layout = newLayout;
        return std::move(*this);
    }

    Widget& layout(const Layout& newLayout) &                        noexcept {
        d_layout = newLayout;
        return *this;
    }

    Widget  layoutMethod(LayoutMethod&&       newMethod) &&          noexcept;

    Widget& layoutMethod(LayoutMethod&&       newMethod) &           noexcept;

    Widget  newChildMethod(NewChildMethod&&   newMethod) &&          noexcept;

    Widget& newChildMethod(NewChildMethod&&   newMethod) &           noexcept;

    Widget  optionName(char const * const optionName) &&             noexcept {
        d_settings.d_optionName = optionName;
        return std::move(*this);
    }

    Widget& optionName(char const * const optionName) &              noexcept {
        d_settings.d_optionName = optionName;
        return *this;
    }

    Widget  options(std::any options) &&                             noexcept {
        d_settings.d_options = std::move(options);
        return std::move(*this);
    }

    Widget& options(std::any options) &                              noexcept {
        d_settings.d_options = std::move(options);
        return *this;
    }

    Widget  serializeMethod(SerializeMethod&& newMethod) &&          noexcept;

    Widget& serializeMethod(SerializeMethod&& newMethod) &           noexcept;

    Widget  text(Text::View_t&& string) &&                           noexcept{
        this->text(std::move(string));
        return std::move(*this);
    }

    Widget& text(Text::View_t&& string) &                             noexcept{
        text().d_layout.d_viewFn = std::move(string.d_viewFn);
        return *this;
    }


    Widget  textMark(Text::BulletMark  mark,
                     bool              leftAlignMark = true) &&      noexcept {
        this->textMark(mark, leftAlignMark);
        return std::move(*this);
    }

    Widget& textMark(Text::BulletMark  mark,
                     bool              leftAlignMark = true) &       noexcept;

    Widget  useTextBounds(bool setting) &&                           noexcept {
        text().d_data.d_useTextBounds = setting;
        return std::move(*this);
    }

    Widget& useTextBounds(bool setting) &                            noexcept {
        text().d_data.d_useTextBounds = setting;
        return *this;
    }

    Widget  verticalAlign(TextAlign alignment) &&                    noexcept {
        this->verticalAlign(alignment);
        return std::move(*this);
    }

    Widget& verticalAlign(TextAlign alignment) &                     noexcept {
        text().d_layout.d_verticalAlign = static_cast<unsigned int>(alignment);
        return *this;
    }

    // PUBLIC MANIPULATORS

    uint16_t  assignWidgetIds(uint16_t         next       = 1,
                              uint16_t         relativeId = 0,
                              CharSizeMapPtr   mapPtr     = nullptr,
                              Widget          *root       = nullptr) noexcept;

    Children& children()                                             noexcept;

    void      changeTracker(Trackee&& newTracker)                    noexcept {
        d_widgetLabel = std::move(newTracker);
        d_widgetLabel.update(this);
    }

    void      clearTrackingPointer()                                 noexcept {
        d_widgetLabel.clear();
    }

    EventUpCb downEvent(double x, double y, Widget *parent = nullptr);

    void      draw(DrawProtocol *adapter = WawtEnv::drawAdapter())   noexcept;

    void      focus(Widget* target = nullptr)                        noexcept;

    bool      inputEvent(Char_t input)                               noexcept;

    Layout&   layout()                                               noexcept {
        return d_layout;
    }

    Layout::Result& layoutData()                                     noexcept {
        return d_rectangle;
    }

    void resolveLayout(DrawProtocol       *adapter,
                       bool                firstPass,
                       const Widget&       parent);

    void      popDialog();

    WidgetId  pushDialog(Widget&&       child,
                         DrawProtocol  *adapter = WawtEnv::drawAdapter());

    bool      resetLabel(Text::View_t&& newLabel);

    void      resizeScreen(double         width,
                           double         height,
                           DrawProtocol  *adapter = WawtEnv::drawAdapter());

    Widget   *screen()                                               noexcept {
        return d_root;                    
    }

    void      selected(bool setting)                                 noexcept {
        d_settings.d_selected = setting;
    }

    Settings& settings()                                             noexcept {
        return d_settings;  
    }

    void      synchronizeTextView(bool recurse = true)               noexcept;

    Text&     text()                                                 noexcept {
        if (!d_text) {
            d_text = std::make_unique<Text>();
        }
        return *d_text;
    }

    WidgetId::IdType& widgetIdValue()                                noexcept {
        return d_settings.d_widgetIdValue;
    }

    // PUBLIC ACCESSORS

    const Children& children()                                 const noexcept;

    const char     *optionName()                               const noexcept {
        return d_settings.d_optionName;
    }

    Widget          clone()                                    const;

    DownEventMethod downEventMethod()                          const noexcept;

    DrawMethod      drawMethod()                               const noexcept;

    bool            hasChildren()                              const noexcept {
        return d_children && !d_children->empty();
    }

    bool            hasText()                                  const noexcept {
        return bool(d_text);
    }

    InputMethod     inputMethod()                              const noexcept;

    bool            inside(double x, double y)                 const noexcept {
        return testTextBounds() ? d_text->d_data.inside(x, y)
                                : d_rectangle.inside(x, y);
    }

    bool            isDisabled()                               const noexcept {
        return d_settings.d_disabled;
    }

    bool            isHidden()                                 const noexcept {
        return d_settings.d_hidden;
    }

    bool            isSelected()                               const noexcept {
        return d_settings.d_selected;
    }

    const Layout&   layout()                                   const noexcept {
        return d_layout;
    }

    const Layout::Result& layoutData()                         const noexcept {
        return d_rectangle;
    }

    LayoutMethod    layoutMethod()                             const noexcept;

    NewChildMethod  newChildMethod()                           const noexcept;

    const Text&     text()                                     const noexcept;

    Widget         *lookup(WidgetId id)                        const noexcept;

    const std::any& options()                                  const noexcept {
        return d_settings.d_options;
    }

    uint16_t        relativeId()                               const noexcept {
        return d_settings.d_relativeId;
    }

    const Widget   *screen()                                   const noexcept {
        return d_root;                    
    }

    SerializeMethod serializeMethod()                          const noexcept;

    const Settings& settings()                                 const noexcept {
        return d_settings;  
    }

    bool            testTextBounds()                           const noexcept {
        return d_text && d_text->d_data.d_useTextBounds;
    }

    Tracker        *tracker()                                  const noexcept {
        return d_widgetLabel.get();
    }

    void            serialize(std::ostream&     os,
                              unsigned int      indent = 0)    const noexcept;

    WidgetId::IdType widgetIdValue()                           const noexcept {
        return d_settings.d_widgetIdValue;
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
    using TextPtr    = std::unique_ptr<Text>;

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

    // PRIVATE DATA MEMBERS (Note: default constructor deleted)
    Trackee             d_widgetLabel;
    Widget             *d_root;
    DownEventMethod     d_downMethod;
    MethodsPtr          d_methods;
    Layout              d_layout;
    Layout::Result      d_rectangle;
    Settings            d_settings;
    TextPtr             d_text;

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
