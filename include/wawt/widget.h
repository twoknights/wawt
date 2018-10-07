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
#include "wawt/input.h"
#include "wawt/text.h"
#include "wawt/draw.h"

#include <initializer_list>
#include <list>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Wawt {


                                //=============
                                // class Widget
                                //=============

class  Widget {
  protected:
    using Callback = InputHandler::Callback;

    // PROTECTED DATA MEMBERS
    Widget            **d_widgetLabel = nullptr;
    Layout              d_layout{};
    InputHandler        d_input{};
    TextBlock           d_text{};
    DrawSettings        d_draw{};

    void serialize(std::ostream&  os,
                   const char    *widgetName,
                   bool           container,
                   unsigned int   indent) const;
  public:
    enum class Enablement { eHIDDEN
                          , eSHOWN
                          , eDISABLED
                          , eENABLED
                          , eOFF
                          , eACTIVE };

    // PUBLIC DATA MEMBERS
    WidgetId            d_widgetId{};

    // PUBLIC CONSTRUCTORS
    Widget()                                = default;

    Widget& operator=(const Widget& rhs)    = delete;

    Widget(const Widget& copy);

    Widget(Widget&& copy);

    Widget& operator=(Widget&& rhs);

    Widget(Widget            **indirect,
           Layout&&          layout,
           InputHandler&&    input,
           TextString&&      text,
           DrawSettings&&    options);

    // PUBLIC MANIPULATORS
    EventUpCb downEvent(int x, int y) { // overriden in Text
        return d_input.downEvent(x, y, this);
    }

    DrawSettings& drawView() {
        return d_draw;
    }

    InputHandler&   inputView() {
        return d_input;
    }

    Layout&         layoutView() {
        return d_layout;
    }

    TextBlock&      textView() {
        return d_text;
    }

    void setEnablement(Enablement newSetting);

    // PUBLIC ACCESSORS
    const DrawDirective& adapterView() const {
        return static_cast<const DrawDirective&>(d_draw);
    }

    bool draw(DrawProtocol *adapter)    const {
        return d_draw.draw(adapter, d_text.getText());
    }

    void serialize(std::ostream& os) const;

    const DrawSettings&    drawView()   const {
        return d_draw;
    }

    const InputHandler&   inputView()  const {
        return d_input;
    }

    const Layout&         layoutView() const {
        return d_layout;
    }

    const TextBlock&      textView()   const {
        return d_text;
    }
};

                                //=============
                                // class Canvas
                                //=============

    class  Canvas final : public Widget {
      public:

        Canvas(Canvas          **indirect        = nullptr,
               Layout&&          layout          = Layout(),
               const PaintFn&    paintFn         = PaintFn(),
               InputHandler&&    onClick         = InputHandler(),
               DrawSettings&&    options         = DrawSettings())
            : Widget(reinterpret_cast<Widget**>(indirect),
                   std::move(layout),
                   std::move(onClick).defaultAction(ActionType::eCLICK),
                   TextString(),
                   std::move(options).paintFn(paintFn)) { }

        Canvas(Layout&&          layout,
               const PaintFn&    paintFn         = PaintFn(),
               InputHandler&&    onClick         = InputHandler(),
               DrawSettings&&    options         = DrawSettings())
            : Widget(nullptr,
                   std::move(layout),
                   std::move(onClick).defaultAction(ActionType::eCLICK),
                   TextString(),
                   std::move(options).paintFn(paintFn)) { }
    };

                                //===========
                                // class Text
                                //===========

    class  Text : public Widget {
        friend class Wawt;

      public:
        FocusCb callSelectFn() {
            return d_input.callSelectFn(this);
        }

        EventUpCb downEvent(int x, int y) {
            return d_input.downEvent(x, y, this);
        }

      protected:
        // PROTECTED DATA MEMBERS

        Text()                          = default;

        Text(Widget            **indirect,
             Layout&&          layout,
             InputHandler&&    mouse,
             TextString&&      text,
             DrawSettings&&    options)
            : Widget(indirect,
                   std::move(layout),
                   std::move(mouse),
                   std::move(text),
                   std::move(options)) { }
    };

                                //================
                                // class TextEntry
                                //================

    class  TextEntry final : public Text {
        friend class Wawt;

      public:
        TextEntry()                     = default;

        TextEntry(TextEntry       **indirect,
                  Layout&&          layout,
                  unsigned int      maxChars,
                  const EnterFn&    enterFn     = EnterFn(),
                  TextString&&      text        = TextString(),
                  DrawSettings&&    options     = DrawSettings())
            : Text(reinterpret_cast<Widget**>(indirect),
                   std::move(layout),
                   InputHandler(std::pair(enterFn,maxChars),
                                ActionType::eENTRY),
                   std::move(text).defaultAlignment(Align::eLEFT),
                   std::move(options)) { }

        TextEntry(Layout&&          layout,
                  unsigned int      maxChars,
                  const EnterFn&    enterFn     = EnterFn(),
                  TextString&&      text        = TextString(),
                  DrawSettings&&    options     = DrawSettings())
            : Text(nullptr,
                   std::move(layout),
                   InputHandler(std::pair(enterFn,maxChars),
                                ActionType::eENTRY),
                   std::move(text).defaultAlignment(Align::eLEFT),
                   std::move(options)) { }

        TextEntry(TextEntry       **indirect,
                  Layout&&          layout,
                  unsigned int      maxChars,
                  TextString&&      text        = TextString(),
                  DrawSettings&&    options     = DrawSettings())
            : Text(reinterpret_cast<Widget**>(indirect),
                   std::move(layout),
                   InputHandler(std::pair(EnterFn(),maxChars),
                                ActionType::eENTRY),
                   std::move(text).defaultAlignment(Align::eLEFT),
                   std::move(options)) { }

        TextEntry(Layout&&          layout,
                  unsigned int      maxChars,
                  TextString&&      text        = TextString(),
                  DrawSettings&&    options     = DrawSettings())
            : Text(nullptr,
                   std::move(layout),
                   InputHandler(std::pair(EnterFn(),maxChars),
                                ActionType::eENTRY),
                   std::move(text).defaultAlignment(Align::eLEFT),
                   std::move(options)) { }

        FocusCb getFocusCb() {
            return d_input.callSelectFn(this);
        }
    };

                                //============
                                // class Label
                                //============

    class  Label final : public Text {
      public:
        Label(Label           **indirect,
              Layout&&          layout,
              TextString&&      text,
              DrawSettings&&    options = DrawSettings())
            : Text(reinterpret_cast<Widget**>(indirect),
                   std::move(layout),
                   InputHandler(),
                   std::move(text).defaultAlignment(Align::eCENTER),
                   std::move(options)) { }

        Label(Layout&&          layout,
              TextString&&      text,
              DrawSettings&&    options = DrawSettings())
            : Text(nullptr,
                   std::move(layout),
                   InputHandler(),
                   std::move(text).defaultAlignment(Align::eCENTER),
                   std::move(options)) { }

    };

                                //=============
                                // class Button
                                //=============

    class  Button final : public Text {
        friend class Wawt;

      public:
        Button()                        = default;

        /// Used in 'ButtonBar' which determines the layout
        Button(TextString&&              text,
               InputHandler&&            onClick,
               DrawSettings&&            options = DrawSettings())
            : Text(nullptr,
                   Layout(),
                   std::move(onClick),
                   std::move(text),
                   std::move(options)) { }

        /// Used in push buttons:
        Button(Button                  **indirect,
               Layout&&                  layout,
               InputHandler&&            onClick,
               TextString&&              text,
               DrawSettings&&            options = DrawSettings())
            : Text(reinterpret_cast<Widget**>(indirect),
                   std::move(layout),
                   std::move(onClick).defaultAction(ActionType::eCLICK),
                   std::move(text).defaultAlignment(Align::eCENTER),
                   std::move(options)) { }

        Button(Layout&&                  layout,
               InputHandler&&            onClick,
               TextString&&              text,
               DrawSettings&&            options = DrawSettings())
            : Text(nullptr,
                   std::move(layout),
                   std::move(onClick).defaultAction(ActionType::eCLICK),
                   std::move(text).defaultAlignment(Align::eCENTER),
                   std::move(options)) { }
    };

                                //================
                                // class ButtonBar
                                //================

    class  ButtonBar final : public Widget {
        friend class Wawt;

        void draw(DrawProtocol *adapter) const;

      public:
        std::vector<Button>        d_buttons;

        ButtonBar(ButtonBar                       **indirect,
                  Layout&&                          layout,
                  double                            borderThickness,
                  std::initializer_list<Button>     buttons);

        ButtonBar(Layout&&                          layout,
                  double                            borderThickness,
                  std::initializer_list<Button>     buttons)
            : ButtonBar(nullptr,
                        std::move(layout),
                        borderThickness,
                        buttons) { }

        ButtonBar(ButtonBar                       **indirect,
                  Layout&&                          layout,
                  std::initializer_list<Button>     buttons)
            : ButtonBar(indirect, std::move(layout), -1.0, buttons) { }

        ButtonBar(Layout&&                          layout,
                  std::initializer_list<Button>     buttons)
            : ButtonBar(nullptr, std::move(layout), -1.0, buttons) { }

        Button&  button(uint16_t index) {
            return d_buttons.at(index);
        }

        EventUpCb downEvent(int x, int y);

        void serialize(std::ostream& os, unsigned int indent = 0) const;
    };

                                    //===========
                                    // class List
                                    //===========

    class  List final : public Widget {
        friend class Wawt;

        // PRIVATE DATA MEMBERS
        std::vector<Button>         d_buttons{};
        Panel                      *d_root;
        unsigned int                d_rows;
        int                         d_startRow;

        // PRIVATE MANIPULATORS
        void   popUpDropDown();

        void   initButton(unsigned int index, bool finalButton);

        void   draw(DrawProtocol *adapter) const;

      public:
        // PUBLIC TYPES
        struct Label {
            TextString d_text;
            bool       d_checked;

            Label(TextString text, bool checked = false)
                : d_text(std::move(text))
                , d_checked(checked) { }
        };
        using Labels = std::initializer_list<Label>;
        using Scroll = std::pair<bool,bool>;

        // PUBLIC DATA MEMBERS
        GroupCb                     d_buttonClick;
        ListType                    d_type;
        FontSizeGrp                 d_fontSizeGrp;
        double                      d_rowHeight;
        
        // PUBLIC CONSTRUCTORS
        List()             = default;

        List(List                            **indirect,
             Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             DrawSettings&&                    options,
             ListType                          listType,
             Labels                            labels,
             const GroupCb&                    click = GroupCb(),
             Panel                            *root  = nullptr);

        List(List                            **indirect,
             Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             DrawSettings&&                    options,
             ListType                          listType,
             unsigned int                      rows,
             const GroupCb&                    click    = GroupCb(),
             Panel                            *root     = nullptr);

        List(List                            **indirect,
             Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             ListType                          listType,
             Labels                            labels,
             const GroupCb&                    click = GroupCb(),
             Panel                            *root  = nullptr)
            : List(indirect, std::move(layout), fontSizeGrp, DrawSettings(),
                   listType, labels, click, root) { }

        List(List                            **indirect,
             Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             ListType                          listType,
             unsigned int                      rows,
             const GroupCb&                    click    = GroupCb(),
             Panel                            *root     = nullptr)
            : List(indirect, std::move(layout), fontSizeGrp, DrawSettings(),
                   listType, rows, click, root) { }

        List(Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             DrawSettings&&                    options,
             ListType                          listType,
             Labels                            labels,
             const GroupCb&                    click = GroupCb(),
             Panel                            *root  = nullptr)
            : List(nullptr, std::move(layout), fontSizeGrp, std::move(options),
                   listType, labels, click, root) { }

        List(Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             DrawSettings&&                    options,
             ListType                          listType,
             unsigned int                      rows,
             const GroupCb&                    click    = GroupCb(),
             Panel                            *root     = nullptr)
            : List(nullptr, std::move(layout), fontSizeGrp, std::move(options),
                   listType, rows, click, root) { }

        List(Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             ListType                          listType,
             Labels                            labels,
             const GroupCb&                    click = GroupCb(),
             Panel                            *root  = nullptr)
            : List(nullptr, std::move(layout), fontSizeGrp, DrawSettings(),
                   listType, labels, click, root) { }

        List(Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             ListType                          listType,
             unsigned int                      rows,
             const GroupCb&                    click    = GroupCb(),
             Panel                            *root     = nullptr)
            : List(nullptr, std::move(layout), fontSizeGrp, DrawSettings(),
                   listType, rows, click, root) { }

        List(const List& copy);

        List(List&& copy);

        // PUBLIC MANIPULATORS
        List& operator=(List&& rhs);

        Button&       append() {
            return row(rows().size());
        }

        void          itemEnablement(unsigned int index, Enablement setting);

        EventUpCb     downEvent(int x, int y);

        void          resetRows();

        Button&       row(unsigned int index);

        void          setButtonPositions(bool resizeListBox = false);

        Scroll        setStartingRow(int           row,
                                     Button       *upButton   = nullptr,
                                     Button       *downButton = nullptr);

        // PUBLIC ACCESSORS
        const Button& row(unsigned int index) const {
            return d_buttons.at(index);
        }

        const std::vector<Button>& rows() const {
            return d_buttons;
        }

        std::vector<uint16_t> selectedRows();

        void serialize(std::ostream& os, unsigned int indent = 0) const;

        int startRow() const {
            return d_startRow;
        }

        unsigned int windowSize() const {
            return d_rows;
        }
    };

                                //============
                                // class Panel
                                //============

class  Panel final : public Widget {
    friend class Wawt;

  public:
    using Widget_ = std::variant<Canvas,        // 0
                                TextEntry,     // 1
                                Label,         // 2
                                Button,        // 3
                                ButtonBar,     // 4
                                List,          // 5
                                Panel>;        // 6

    using Widgets = std::vector<Widget_>;

    Panel()                             = default;
    
    Panel(Panel                         **indirect,
          Layout&&                        layout,
          DrawSettings&&                  options = DrawSettings())
        : Widget(reinterpret_cast<Widget**>(indirect),
               std::move(layout),
               InputHandler().defaultAction(ActionType::eCLICK),
               TextString(),
               std::move(options))
        , d_widgets() { }

    Panel(Panel                         **indirect,
          Layout&&                        layout,
          std::initializer_list<Widget_>   widgets)
        : Widget(reinterpret_cast<Widget**>(indirect),
               std::move(layout),
               InputHandler().defaultAction(ActionType::eCLICK),
               TextString(),
               DrawSettings())
        , d_widgets(widgets) { }

    Panel(Panel                         **indirect,
          Layout&&                        layout,
          DrawSettings&&                  options,
          std::initializer_list<Widget_>   widgets)
        : Widget(reinterpret_cast<Widget**>(indirect),
               std::move(layout),
               InputHandler().defaultAction(ActionType::eCLICK),
               TextString(),
               std::move(options))
        , d_widgets(widgets) { }

    Panel(Panel                         **indirect,
          Layout&&                        layout,
          const Widgets&                  widgets)
        : Widget(reinterpret_cast<Widget**>(indirect),
               std::move(layout),
               InputHandler().defaultAction(ActionType::eCLICK),
               TextString(),
               DrawSettings())
        , d_widgets(widgets.begin(), widgets.end()) { }

    Panel(Panel                         **indirect,
          Layout&&                        layout,
          DrawSettings&&                  options,
          const Widgets&                  widgets)
        : Widget(reinterpret_cast<Widget**>(indirect),
               std::move(layout),
               InputHandler().defaultAction(ActionType::eCLICK),
               TextString(),
               std::move(options))
        , d_widgets(widgets.begin(), widgets.end()) { }
    
    Panel(Layout&& layout, DrawSettings&& options = DrawSettings())
        : Widget(nullptr,
               std::move(layout),
               InputHandler().defaultAction(ActionType::eCLICK),
               TextString(),
               std::move(options))
        , d_widgets() { }

    Panel(Layout&&                      layout,
          std::initializer_list<Widget_> widgets)
        : Widget(nullptr,
               std::move(layout),
               InputHandler().defaultAction(ActionType::eCLICK),
               TextString(),
               DrawSettings())
        , d_widgets(widgets) { }

    Panel(Layout&&                      layout,
          DrawSettings&&                options,
          std::initializer_list<Widget_> widgets)
        : Widget(nullptr,
               std::move(layout),
               InputHandler().defaultAction(ActionType::eCLICK),
               TextString(),
               std::move(options))
        , d_widgets(widgets) { }

    Panel(Layout&&                      layout,
          const Widgets&                widgets)
        : Widget(nullptr,
               std::move(layout),
               InputHandler().defaultAction(ActionType::eCLICK),
               TextString(),
               DrawSettings())
        , d_widgets(widgets.begin(), widgets.end()) { }

    Panel(Layout&&                      layout,
          DrawSettings&&                options,
          const Widgets&                widgets)
        : Widget(nullptr,
               std::move(layout),
               InputHandler().defaultAction(ActionType::eCLICK),
               TextString(),
               std::move(options))
        , d_widgets(widgets.begin(), widgets.end()) { }


    template<class WIDGET>
    WIDGET&       lookup(WidgetId id, const std::string& whatInfo);

    EventUpCb     downEvent(int x, int y);

    template<class WIDGET>
    const WIDGET& lookup(WidgetId id, const std::string& whatInfo) const {
        return const_cast<Panel*>(this)->lookup<WIDGET>(id, whatInfo);
    }

    void serialize(std::ostream& os, unsigned int indent = 0) const;

    const std::list<Widget_>& widgets() const {
        return d_widgets;
    }

  private:

    bool          findWidget(Widget_ **widget, WidgetId widgetId);

    // The ordering of widgets should not be changed during execution
    // EXCEPT for appending widgets to the "root" panel. This allows for
    // the showing of a pop-up (e.g. drop-down list).  This must NOT result
    // in a reallocation since that operation would be performed by a
    // callback whose closure would be contained in a moved widget.
    // Thus, a list is used instead of a vector.
    std::list<Widget_> d_widgets;
};

template<class WIDGET>
WIDGET& 
Panel::lookup(WidgetId id, const std::string& whatInfo)
{
    Widget_ *widget;

    if (!id.isSet()) {
        throw WawtException(whatInfo + " ID is invalid.", id);
    }

    if (!findWidget(&widget, id)) {
        throw WawtException(whatInfo + " not found.", id);
    }

    if (!std::holds_alternative<WIDGET>(*widget)) {
        throw WawtException(whatInfo + " ID is wrong.", id, *widget);
    }
    return std::get<WIDGET>(*widget);                                 // RETURN
}

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
