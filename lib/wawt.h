/** @file what.h
 *  @brief Provides definition of WAWT related types.
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

#ifndef BDS_WAWT_H
#define BDS_WAWT_H

#include <any>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace BDS {

                                    //==============
                                    // class Wawt_Id
                                    //==============

class Wawt_Id {
  public:

    template<typename IntType>
    class IntId {
      protected:
        IntType d_id;

      public:
        using value_type = IntType;

        constexpr IntId() : d_id() { }

        constexpr IntId(const IntId& copy) : d_id(copy.d_id) { }

        template<typename OtherInt>
        constexpr IntId(OtherInt value) : d_id(static_cast<IntType>(value)) { }

        constexpr bool operator==(const IntId& rhs) const {
            return d_id == rhs.d_id;
        }

        constexpr bool operator!=(const IntId& rhs) const {
            return !(*this == rhs);
        }

        constexpr bool operator<(const IntId& rhs) const {
            return d_id < rhs.d_id;
        }

        constexpr bool operator>(const IntId& rhs) const {
            return d_id > rhs.d_id;
        }
        
        constexpr IntType value() const {
            return d_id;
        }
    };

    template<typename IntType, typename Name>
    class Mixin {
      protected:
        IntType d_value;

      public:
        constexpr Mixin() : d_value() { }

        constexpr Mixin(bool value) : d_value(value) { }

        constexpr Mixin(const Mixin& copy) : d_value(copy.d_value) { }

        constexpr Mixin& operator=(const Mixin& rhs) {
            d_value = rhs.d_value;
            return *this;
        }

        constexpr bool operator==(const Mixin& rhs) const {
            return d_value == rhs.d_value;
        }

        constexpr bool operator!=(const Mixin& rhs) const {
            return !(*this == rhs);
        }
    };

    template<typename IdType, typename... Mixins>
    class Id : public Mixins... { // parameter pack
        IdType d_id;
      public:
        constexpr Id() : Mixins()..., d_id() { }

        template<typename OtherType>
        constexpr Id(OtherType id, Mixins... args)
            : Mixins(args)..., d_id(id) { }

        constexpr Id(const Id& copy) : Mixins(copy)..., d_id(copy.d_id) { }

        constexpr Id& operator=(const Id& rhs) {
            (Mixins::operator=(rhs), ...);
            d_id = rhs.d_id;
            return *this;
        }

        constexpr bool operator==(const Id& rhs) const { // fold expression
            return d_id == rhs.d_id && (... && Mixins::operator==(rhs));
        }

        constexpr bool operator!=(const Id& rhs) const {
            return !(*this == rhs);
        }

        constexpr bool operator<(const Id& rhs) const {
            return d_id < rhs.d_id;
        }

        constexpr bool operator>(const Id& rhs) const {
            return d_id > rhs.d_id;
        }
        
        constexpr typename IdType::value_type value() const {
            return d_id.value();
        }
        
        constexpr operator int() const {
            return d_id.value();
        }
    };

    class IsSet : public Mixin<bool, IsSet> {
      public:
        constexpr IsSet()              = default;
        constexpr IsSet(bool) : Mixin(true) { }

        constexpr bool isSet() const {
            return d_value;
        }
    };

    class IsRelative : public Mixin<bool, IsRelative> {
      public:
        constexpr IsRelative()     = default;
        constexpr IsRelative(bool flag) : Mixin(flag) { }

        constexpr bool isRelative() const {
            return d_value;
        }
    };

    using WidgetId   = Id<IntId<uint16_t>, IsSet, IsRelative>;

    static WidgetId inc(WidgetId& id) {
        assert(id.isSet());
        assert(!id.isRelative());
        auto next = Wawt_Id::WidgetId(id.value()+1, true, false);
        return std::exchange(id, std::move(next));
    }
};

                                    //===========
                                    // class Wawt
                                    //===========

class Wawt {
  public:
    class  Base;
    class  Button;
    class  DrawAdapter;
    class  List;
    class  Canvas;
    class  Text;
    class  Panel;

    // PUBLIC TYPES

    // Char type:
    // Note: fixed length encodings required. These would be: ANSI (char),
    // UCS-2 (wchar_t), and UTF-32 (char32_t).
    using Char_t   = char32_t;
    using String_t = std::u32string;

    // Identifiers:
    using OptInt      = std::optional<int>;

    using WidgetId    = Wawt_Id::WidgetId;

    using Scale       = std::pair<double, double>;

    enum class TieScale { eNONE, eUL_X, eUL_Y, eLR_X, eLR_Y, eCC_X, eCC_Y };

    // PUBLIC CONSTANTS
    static const WidgetId    kPARENT;
    static const WidgetId    kROOT;

    // PUBLIC TYPES

                                //===================
                                // class DrawSettings  
                                //===================

    enum class BulletType { eNONE, eRADIO, eCHECK };

    using PaintFn     = std::function<void(int ux, int uy, int lx, int ly)>;

    struct  DrawPosition  {
        double              d_x               = 0.0;
        double              d_y               = 0.0;
    };

    struct DrawDirective {
        using Tracking = std::tuple<int,int,int>;
        Tracking            d_tracking; // handy debug info

        DrawPosition        d_upperLeft       {};
        DrawPosition        d_lowerRight      {};
        double              d_borderThickness = 0.0;
        BulletType          d_bulletType      = BulletType::eNONE;
        bool                d_greyEffect      = false;
        bool                d_selected        = false;
        double              d_startx          = 0.0; // for text placement
        unsigned int        d_charSize        = 0u; // in pixels
        std::any            d_options; // default: transparent box, black text

        DrawDirective() : d_tracking{-1,-1,-1} { }

        DrawDirective(std::any&& options) : d_options(std::move(options)) { }

        double height() const {
            return d_lowerRight.d_y - d_upperLeft.d_y + 1;
        }

        double interiorHeight() const {
            return height() - 2*d_borderThickness;
        }

        double interiorWidth() const {
            return width() - 2*d_borderThickness;
        }

        bool verify() const {
            return d_upperLeft.d_x < d_lowerRight.d_x
                && d_upperLeft.d_y < d_lowerRight.d_y;
        }

        double width() const {
            return d_lowerRight.d_x - d_upperLeft.d_x + 1;
        }
    };

    class DrawSettings : DrawDirective {
        friend class Base;
        friend class Wawt;
        friend class List;

        bool                d_hidden;
        PaintFn             d_paintFn;

        bool draw(DrawAdapter *adapter, const String_t& txt) const;

      public:
        DrawSettings() : DrawDirective() , d_hidden(false) , d_paintFn() { }

        template<class OptionClass>
        DrawSettings(OptionClass&& options)
            : DrawDirective(std::any(std::move(options)))
            , d_hidden(false)
            , d_paintFn() { }

        DrawSettings&& paintFn(const PaintFn& paintFn) && {
            d_paintFn = paintFn;
            return std::move(*this);
        }

        DrawSettings&& bulletType(BulletType value) && {
            d_bulletType = value;
            return std::move(*this);
        }

        BulletType& bulletType() {
            return d_bulletType;
        }

        std::any& options() {
            return d_options;
        }

        bool& selected() {
            return d_selected;
        }

        const DrawDirective& adapterView() const {
            return static_cast<const DrawDirective&>(*this);
        }

        BulletType bulletType() const {
            return d_bulletType;
        }

        bool hidden() const {
            return d_hidden;
        }

        const std::any& options() const {
            return d_options;
        }

        bool selected() const {
            return d_selected;
        }
    };

                                //===================
                                // class InputHandler  
                                //===================

    using EnterFn     = std::function<bool(String_t*)>;

    using FocusCb     = std::function<bool(Char_t)>;

    using GroupCb     = std::function<FocusCb(List*, uint16_t)>;

    using EventUpCb   = std::function<FocusCb(int x, int y, bool)>;

    using OnClickCb   = std::function<FocusCb(bool, int x, int y, Base*)>;

    using SelectFn    = std::function<FocusCb(Text*)>;

    enum class ActionType { eINVALID, eCLICK, eTOGGLE, eBULLET, eENTRY };

    class InputHandler {
        friend class Base;
        friend class Wawt;
        friend class List;

        EventUpCb callOnClickCb(int               x,
                                int               y,
                                Base             *base,
                                const OnClickCb&  cb,
                                bool              callOnDown);

        EventUpCb callSelectFn(Text            *text,
                               const SelectFn&  cb,
                               bool             callOnDown);

        EventUpCb callSelectFn(Base            *base,
                               const SelectFn&  cb,
                               bool             callOnDown);

        bool                d_disabled;

      public:
        using Callback = std::variant<std::monostate,
                                      OnClickCb,
                                      SelectFn,
                                      std::pair<EnterFn,uint16_t>,
                                      std::pair<SelectFn,bool>,
                                      std::pair<OnClickCb,bool>>;

        ActionType          d_type;
        Callback            d_callback;

        InputHandler(Callback   cb     = Callback(),
                     ActionType action = ActionType::eINVALID)
            : d_disabled(d_type == ActionType::eINVALID)
            , d_type(action)
            , d_callback(std::move(cb)) { }

        InputHandler&& defaultAction(ActionType type) && {
            if (d_type == ActionType::eINVALID) {
                d_type     = type;
                d_disabled = d_type == ActionType::eINVALID;
            }
            return std::move(*this);
        }

        void defaultAction(ActionType type) & {
            if (d_callback.index() > 0 && d_type == ActionType::eINVALID) {
                d_type     = type;
                d_disabled = d_type == ActionType::eINVALID;
            }
        }

        FocusCb callSelectFn(Text* text);

        EventUpCb downEvent(int x, int y, Base*  base);

        EventUpCb downEvent(int x, int y, Text* text);

        bool contains(int x, int y, const Base* base) const;

        bool disabled() const {
            return d_disabled;
        }

        bool textcontains(int x, int y, const Text *text) const;
    };


                                //=============
                                // class Layout
                                //=============

    struct Metric {
        double              d_r;

        constexpr explicit Metric(double r) : d_r(r) { }

        constexpr Metric operator-() {
            return Metric(-d_r);
        }
    };

    struct Vertex {
        double              d_sx;
        double              d_sy;

        constexpr Vertex()                     : d_sx(0.0),    d_sy(0.0)    { }

        constexpr Vertex(Metric s)             : d_sx(s.d_r),  d_sy(s.d_r)  { }

        constexpr Vertex(Metric sx, Metric sy) : d_sx(sx.d_r), d_sy(sy.d_r) { }
    };

    enum ScaleBias : unsigned char {
                eOUTER    = 0  ///< Vertex is on outer edge
              , eOUTER1   = 1  ///< Vertex is 1 pixel past outer edge
              , eINNER    = 2  ///< Vertex is on inner edge
              , eINNER1   = 3  ///< Vertex is 1 pixel before inner edge
    };
    using BiasPair  = std::pair<ScaleBias,ScaleBias>; // x,y edge description

    class Position {
      public:
        Vertex               d_vertex;
        WidgetId             d_widgetId;
        BiasPair             d_endPoint;
        int                  d_x;
        int                  d_y;

        constexpr Position(Vertex vertex=Metric(-1.0), int x=0, int y=0)
            : d_vertex(vertex)
            , d_widgetId(kPARENT)
            , d_endPoint{eINNER1, eINNER1}
            , d_x(x)
            , d_y(y) { }

        constexpr Position(Vertex vertex, WidgetId id, int x=0, int y=0)
            : d_vertex(vertex)
            , d_widgetId(id)
            , d_endPoint{eOUTER, eOUTER}
            , d_x(x)
            , d_y(y) { }

        constexpr Position(Vertex          vertex,
                           BiasPair        endPoint,
                           WidgetId        id,
                           int             x = 0,
                           int             y = 0)
            : d_vertex(vertex)
            , d_widgetId(id)
            , d_endPoint(endPoint)
            , d_x(x)
            , d_y(y) { }
    };

    struct Layout {
        TieScale            d_tie;
        Position            d_upperLeft;
        Position            d_lowerRight;
        double              d_borderThickness;

        constexpr Layout(const Position& upperLeft       = {},
                         const Position& lowerRight      = {{Metric(1.0),
                                                             Metric(1.0)}},
                         OptInt          borderThickness = OptInt(),
                         TieScale        tie             = TieScale::eNONE)
            : d_tie(tie)
            , d_upperLeft(upperLeft)
            , d_lowerRight(lowerRight)
            , d_borderThickness(double(borderThickness.value_or(-1.0))) { }

        constexpr static Layout duplicate(WidgetId id,
                                          OptInt   thickness = OptInt()) {
            return Layout({{Metric(-1.0),Metric(-1.0)}, {eOUTER,eOUTER}, id},
                          {{Metric( 1.0),Metric( 1.0)}, {eOUTER,eOUTER}, id},
                          thickness);
        }

        Layout&& translate(double x, double y) && {
            this->d_upperLeft.d_vertex.d_sx  += x;
            this->d_upperLeft.d_vertex.d_sy  += y;
            this->d_lowerRight.d_vertex.d_sx += x;
            this->d_lowerRight.d_vertex.d_sy += y;
            return std::move(*this);
        }
    };

                                //================
                                // class TextBlock
                                //================

    using FontSizeGrp = std::optional<uint16_t>;

    using TextId      = uint16_t;

    using TextMapper  = std::function<String_t(const TextId&)>;

    enum class Align { eINVALID, eLEFT, eCENTER, eRIGHT };

    // PUBLIC CONSTANTS
    constexpr static const TextId kNOID = 0;

    struct  TextMetrics {
        double          d_textWidth           = 0; // includes bullet size
        double          d_textHeight          = 0;
    };

    struct TextString {
        TextId              d_id;
        String_t            d_string;
        Align               d_alignment;
        FontSizeGrp         d_fontSizeGrp;

        TextString(TextId       id        = kNOID,
                   FontSizeGrp  group     = FontSizeGrp(),
                   Align        alignment = Align::eINVALID)
            : d_id(id)
            , d_string()
            , d_alignment(alignment)
            , d_fontSizeGrp(group) { }

        TextString(TextId id, Align alignment)
            : d_id(id)
            , d_string()
            , d_alignment(alignment)
            , d_fontSizeGrp() { }

        TextString(String_t     string,
                   FontSizeGrp  group     = FontSizeGrp(),
                   Align        alignment = Align::eINVALID)
            : d_id()
            , d_string(std::move(string))
            , d_alignment(alignment)
            , d_fontSizeGrp(group) { }

        TextString(String_t string, Align alignment)
            : d_id()
            , d_string(std::move(string))
            , d_alignment(alignment)
            , d_fontSizeGrp() { }

        TextString(FontSizeGrp group, Align alignment = Align::eINVALID)
            : d_id()
            , d_string()
            , d_alignment(alignment)
            , d_fontSizeGrp(group) { }

        TextString&& defaultAlignment(Align alignment) && {
            if (d_alignment == Align::eINVALID) {
                d_alignment = alignment;
            }
            return std::move(*this);
        }
    };

    class TextBlock {
        friend class Base;
        friend class Wawt;
        friend class List;

        TextMetrics         d_metrics       {};
        TextString          d_block         {};
        bool                d_needRefresh   = false;

      public:
        TextBlock()                         = default; 

        TextBlock(const TextString& value) : d_metrics(), d_block(value) { }

        Align& alignment() {
            return d_block.d_alignment;
        }

        FontSizeGrp& fontSizeGrp() {
            return d_block.d_fontSizeGrp;
        }

        void initTextMetricValues(DrawDirective      *args,
                                  DrawAdapter        *adapter,
                                  uint16_t            upperLimit = 0);

        void setText(TextId id);

        void setText(String_t string);

        void setText(const TextMapper& mappingFn);

        void setText(const TextString& value) {
            d_block       = value;
            d_needRefresh = true;
        }

        Align alignment() const {
            return d_block.d_alignment;
        }

        FontSizeGrp fontSizeGrp() const {
            return d_block.d_fontSizeGrp;
        }

        const String_t& getText() const {
            return d_block.d_string;
        }

        const TextMetrics& metrics() const {
            return d_metrics;
        }

        bool needRefresh() const {
            return d_needRefresh;
        }
    };

    // WIDGET HEIRARCHY TYPES (no constexpr constructors)

                                //===========
                                // class Base
                                //===========

    enum class Enablement { eHIDDEN
                          , eSHOWN
                          , eDISABLED
                          , eENABLED
                          , eOFF
                          , eACTIVE };

    class  Base {
        friend class Wawt;

      protected:
        using Callback = InputHandler::Callback;

        // PROTECTED DATA MEMBERS
        Layout              d_layout{};
        InputHandler        d_input{};
        TextBlock           d_text{};
        DrawSettings        d_draw{};

        void serialize(std::ostream&  os,
                       const char    *widgetName,
                       bool           container,
                       unsigned int   indent) const;
      public:
        WidgetId            d_widgetId{};

        // PUBLIC CONSTRUCTORS
        Base()                        = default;

        Base(Layout&&          layout,
             InputHandler&&    input,
             TextString&&      text,
             DrawSettings&&    options)
            : d_layout(std::move(layout))
            , d_input(std::move(input))
            , d_text(std::move(text))
            , d_draw(std::move(options)) { }

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

        bool draw(DrawAdapter *adapter)    const {
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

    class  Canvas final : public Base {
      public:

        Canvas(Layout&&          layout          = Layout(),
               const PaintFn&    paintFn         = PaintFn(),
               InputHandler&&    onClick         = InputHandler(),
               DrawSettings&&    options         = DrawSettings())
            : Base(std::move(layout),
                   std::move(onClick).defaultAction(ActionType::eCLICK),
                   TextString(),
                   std::move(options).paintFn(paintFn)) { }
    };

                                //===========
                                // class Text
                                //===========

    class  Text : public Base {
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

        Text(Layout&&          layout,
             InputHandler&&    mouse,
             TextString&&      text,
             DrawSettings&&    options)
            : Base(std::move(layout),
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

        TextEntry(Layout&&          layout,
                  unsigned int      maxChars,
                  const EnterFn&    enterFn     = EnterFn(),
                  TextString&&      text        = TextString(),
                  DrawSettings&&    options     = DrawSettings())
            : Text(std::move(layout),
                   InputHandler(std::pair(enterFn,maxChars),
                                ActionType::eENTRY),
                   std::move(text).defaultAlignment(Align::eLEFT),
                   std::move(options)) { }
    };

                                //============
                                // class Label
                                //============

    class  Label final : public Text {
      public:
        Label(Layout&&          layout,
              TextString&&      text,
              DrawSettings&&    options = DrawSettings())
            : Text(std::move(layout),
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
        template <typename Callable>
        static InputHandler onPush(const Callable& callback) {
            auto wrap = [push = callback](Text* arg) {
                (void)push(arg);
                return FocusCb();
            };
            return InputHandler(std::move(wrap));
        }
        
        Button()                        = default;

        /// Used in 'ButtonBar' which determines the layout
        Button(TextString&&              text,
               InputHandler&&            onClick,
               DrawSettings&&            options = DrawSettings())
            : Text(Layout(),
                   std::move(onClick),
                   std::move(text),
                   std::move(options)) { }

        /// Used in push buttons:
        Button(Layout&&                  layout,
               InputHandler&&            onClick,
               TextString&&              text,
               DrawSettings&&            options = DrawSettings())
            : Text(std::move(layout),
                   std::move(onClick).defaultAction(ActionType::eCLICK),
                   std::move(text),
                   std::move(options)) { }
    };

                                //================
                                // class ButtonBar
                                //================

    class  ButtonBar final : public Base {
        friend class Wawt;

        void draw(DrawAdapter *adapter) const;

      public:
        std::vector<Button>        d_buttons;

        ButtonBar(Layout&&                          layout,
                  OptInt                            borderThickness,
                  std::initializer_list<Button>     buttons);

        ButtonBar(Layout&&                          layout,
                  std::initializer_list<Button>     buttons)
            : ButtonBar(std::move(layout), OptInt(), buttons) { }


        EventUpCb downEvent(int x, int y);

        void serialize(std::ostream& os, unsigned int indent = 0) const;
    };

                                    //===========
                                    // class List
                                    //===========

    enum class ListType { eCHECKLIST
                        , eRADIOLIST
                        , ePICKLIST
                        , eSELECTLIST
                        , eVIEWLIST
                        , eDROPDOWNLIST };

    class  List final : public Base {
        friend class Wawt;

        // PRIVATE DATA MEMBERS
        std::vector<Button>         d_buttons{};
        Panel                      *d_root;
        unsigned int                d_rows;
        int                         d_startRow;

        // PRIVATE MANIPULATORS
        void   popUpDropDown();

        void   initButton(unsigned int index, bool finalButton);

        void   draw(DrawAdapter *adapter) const;

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

        List(Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             DrawSettings&&                    options,
             ListType                          listType,
             Labels                            labels,
             const GroupCb&                    click = GroupCb(),
             Panel                            *root  = nullptr);

        List(Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             DrawSettings&&                    options,
             ListType                          listType,
             unsigned int                      rows,
             const GroupCb&                    click    = GroupCb(),
             Panel                            *root     = nullptr);

        List(Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             ListType                          listType,
             Labels                            labels,
             const GroupCb&                    click = GroupCb(),
             Panel                            *root  = nullptr)
            : List(std::move(layout), fontSizeGrp, DrawSettings(),
                   listType, labels, click, root) { }

        List(Layout&&                          layout,
             FontSizeGrp                       fontSizeGrp,
             ListType                          listType,
             unsigned int                      rows,
             const GroupCb&                    click    = GroupCb(),
             Panel                            *root     = nullptr)
            : List(std::move(layout), fontSizeGrp, DrawSettings(),
                   listType, rows, click, root) { }

        List(const List& copy);

        List(List&& copy);

        // PUBLIC MANIPULATORS
        List& operator=(const List& rhs);

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

    class  Panel final : public Base {
        friend class Wawt;

      public:
        using Widget = std::variant<Canvas,        // 0
                                    TextEntry,     // 1
                                    Label,         // 2
                                    Button,        // 3
                                    ButtonBar,     // 4
                                    List,          // 5
                                    Panel>;        // 6

        Panel()                             = default;
        
        Panel(Layout&& layout, DrawSettings&& options = DrawSettings())
            : Base(std::move(layout),
                   InputHandler().defaultAction(ActionType::eCLICK),
                   TextString(),
                   std::move(options))
            , d_widgets() { }

        Panel(Layout&&                      layout,
              std::initializer_list<Widget> widgets)
            : Base(std::move(layout),
                   InputHandler().defaultAction(ActionType::eCLICK),
                   TextString(),
                   DrawSettings())
            , d_widgets(widgets) { }

        Panel(Layout&&                      layout,
              DrawSettings&&                options,
              std::initializer_list<Widget> widgets)
            : Base(std::move(layout),
                   InputHandler().defaultAction(ActionType::eCLICK),
                   TextString(),
                   std::move(options))
            , d_widgets(widgets) { }


        template<class WIDGET>
        WIDGET&       lookup(WidgetId id, const std::string& whatInfo);

        EventUpCb     downEvent(int x, int y);

        template<class WIDGET>
        const WIDGET& lookup(WidgetId id, const std::string& whatInfo) const {
            return const_cast<Panel*>(this)->lookup<WIDGET>(id, whatInfo);
        }

        void serialize(std::ostream& os, unsigned int indent = 0) const;

        const std::list<Widget>& widgets() const {
            return d_widgets;
        }

      private:

        bool          findWidget(Widget **widget, WidgetId widgetId);

        // The ordering of widgets should not be changed during execution
        // EXCEPT for appending widgets to the "root" panel. This allows for
        // the showing of a pop-up (e.g. drop-down list).  This must NOT result
        // in a reallocation since that operation would be performed by a
        // callback whose closure would be contained in a moved widget.
        // Thus, a list is used instead of a vector.
        std::list<Widget> d_widgets;
    };

                                    //==================
                                    // class DrawAdapter
                                    //==================

    class  DrawAdapter {
      public:
        virtual void  draw(const Wawt::DrawDirective&  parameters,
                           const String_t&             text)               = 0;

        virtual void  getTextMetrics(Wawt::DrawDirective  *parameters,
                                     Wawt::TextMetrics    *metrics,
                                     const String_t&       text,
                                     double                upperLimit = 0) = 0;
    };

    //! Wawt runtime exception
    class  Exception : public std::runtime_error {
      public:
        Exception(const std::string& what_arg) : std::runtime_error(what_arg) {
        }

        Exception(const std::string& what_arg, WidgetId widgetId)
            : std::runtime_error(what_arg + " id="
                                          + std::to_string(widgetId.value())) {
        }

        Exception(const std::string& what_arg, const Panel::Widget& widget)
            : std::runtime_error(what_arg + " index="
                                          + std::to_string(widget.index())) {
        }

        Exception(const std::string&    what_arg,
                  WidgetId              id,
                  const Panel::Widget&  widget)
            : std::runtime_error(what_arg + " id=" + std::to_string(id.value())
                                          + " index="
                                          + std::to_string(widget.index())) {
        }
    };

    struct  BorderThicknessDefaults {
        unsigned int d_canvasThickness    = 1u;
        unsigned int d_textEntryThickness = 0u;
        unsigned int d_labelThickness     = 0u;
        unsigned int d_buttonThickness    = 2u;
        unsigned int d_buttonBarThickness = 1u;
        unsigned int d_listThickness      = 2u;
        unsigned int d_panelThickness     = 0u;
    };

    struct  WidgetOptionDefaults {
        std::any     d_screenOptions;    ///! Options for root Panel
        std::any     d_canvasOptions;    ///! Default Canvas options
        std::any     d_textEntryOptions; ///! Default TextEntry options
        std::any     d_labelOptions;     ///! Default Label options
        std::any     d_buttonOptions;    ///! Default Button options
        std::any     d_buttonBarOptions; ///! Default ButtonBar options
        std::any     d_listOptions;      ///! Default List options
        std::any     d_panelOptions;     ///! Default Panel options
    };

    // PUBLIC CLASS DATA
    static Char_t   s_downArrow;
    static Char_t   s_upArrow;
    static Char_t   s_cursor;

    static const std::any  s_noOptions;

    // PUBLIC CLASS MEMBERS
    static void    scalePosition(Panel::Widget  *widget,
                                 const Scale&    scale,
                                 double          borderScale);

    static Panel   scrollableList(List          list,
                                  bool          buttonsOnLeft = true,
                                  unsigned int  scrollLines   = 1);

    static void    removePopUp(Panel *root);

    static void    setScrollableListStartingRow(Wawt::List   *list,
                                              unsigned int row);

    // PUBLIC CONSTRUCTOR
    explicit Wawt(const TextMapper&  mappingFn = TextMapper(),
                  DrawAdapter       *adapter   = 0);

    explicit Wawt(DrawAdapter *adapter) : Wawt(TextMapper(), adapter) { }

    // PUBLIC MANIPULATORS
    void  draw(const Panel& panel);

    void  popUpModalDialogBox(Panel *root, Panel&& dialogBox);

    void  refreshTextMetrics(Panel *panel);

    void  resizeRootPanel(Panel   *root, double  width, double  height);

    void  resolveWidgetIds(Panel *root);

    void setBorderThicknessDefaults(const BorderThicknessDefaults& defaults) {
        d_borderDefaults = defaults;
    }

    void setWidgetOptionDefaults(const WidgetOptionDefaults& defaults) {
        d_optionDefaults = defaults;
    }

    // PUBLIC ACCESSORS
    const WidgetOptionDefaults& getWidgetOptionDefaults() const {
        return d_optionDefaults;
    }

    const std::any& defaultScreenOptions()            const {
        return d_optionDefaults.d_screenOptions;
    }

    const std::any& defaultCanvasOptions()            const {
        return d_optionDefaults.d_canvasOptions;
    }

    const std::any& defaultTextEntryOptions()         const {
        return d_optionDefaults.d_textEntryOptions;
    }

    const std::any& defaultLabelOptions()             const {
        return d_optionDefaults.d_labelOptions;
    }

    const std::any& defaultButtonOptions()            const {
        return d_optionDefaults.d_buttonOptions;
    }

    const std::any& defaultButtonBarOptions()         const {
        return d_optionDefaults.d_buttonBarOptions;
    }

    const std::any& defaultPanelOptions()             const {
        return d_optionDefaults.d_panelOptions;
    }

    const std::any& defaultListOptions(ListType type) const {
        auto  usePanel = (type==ListType::eCHECKLIST
                       || type==ListType::eRADIOLIST);
        return usePanel ? d_optionDefaults.d_panelOptions
                        : d_optionDefaults.d_listOptions;
    }

  private:
    // PRIVATE TYPE
    using FontIdMap  = std::map<FontSizeGrp, uint16_t>;

    // PRIVATE CLASS MEMBERS
    static void setIds(Panel::Widget *widget, WidgetId& id);

    static void setWidgetAdapterPositions(
            Panel::Widget                  *widget,
            Panel                          *root,
            const Panel&                    panel,
            const Scale&                    scale,
            const BorderThicknessDefaults&  border,
            const WidgetOptionDefaults&     option);

    // PRIVATE MANIPULATORS
    void  setFontSizeEntry(Base *args);

    void  setTextAndFontValues(Panel *root);

    // PRIVATE DATA
    DrawAdapter             *d_adapter_p;
    TextMapper               d_idToString;
    FontIdMap                d_fontIdToSize;
    BorderThicknessDefaults  d_borderDefaults;
    WidgetOptionDefaults     d_optionDefaults;
};

template<class WIDGET>
WIDGET& 
Wawt::Panel::lookup(Wawt::WidgetId id, const std::string& whatInfo)
{
    Widget *widget;

    if (!id.isSet()) {
        throw Wawt::Exception(whatInfo + " ID is invalid.", id);
    }

    if (!findWidget(&widget, id)) {
        throw Wawt::Exception(whatInfo + " not found.", id);
    }

    if (!std::holds_alternative<WIDGET>(*widget)) {
        throw Wawt::Exception(whatInfo + " ID is wrong.", id, *widget);
    }
    return std::get<WIDGET>(*widget);                                 // RETURN
}

                            //================
                            // class  WawtDump
                            //================

class  WawtDump : public Wawt::DrawAdapter {
  public:
    struct Indent {
        unsigned int d_indent;

        Indent& operator+=(unsigned int change) {
            d_indent += change;
            return *this;
        }

        Indent& operator-=(unsigned int change) {
            d_indent -= change;
            return *this;
        }

        Indent(unsigned int indent = 0) : d_indent(indent) { }
    };

    explicit WawtDump(std::ostream& os) : d_dumpOs(os) { }

    void     draw(const Wawt::DrawDirective&  widget,
                  const Wawt::String_t&       text)                override;

    void     getTextMetrics(Wawt::DrawDirective   *options,
                            Wawt::TextMetrics     *metrics,
                            const Wawt::String_t&  text,
                            double                 upperLimit = 0) override;

  private:
    Indent         d_indent;
    std::ostream&  d_dumpOs;
};

// FREE OPERATORS

inline
constexpr Wawt::WidgetId operator "" _w(unsigned long long int n) {
    return Wawt::WidgetId(n, true, false);
}

inline
constexpr Wawt::WidgetId operator "" _wr(unsigned long long int n) {
    return Wawt::WidgetId(n, true, true);
}

inline
constexpr Wawt::FontSizeGrp operator "" _F(unsigned long long int n) {
    return Wawt::FontSizeGrp(n);
}

constexpr Wawt::Metric operator ""_M (long double s) {
    return Wawt::Metric(s);
}

constexpr Wawt::Vertex operator|(Wawt::Metric lhs, Wawt::Metric rhs) {
    return Wawt::Vertex(lhs, rhs);
}

} // end BDS namespace

#endif

// vim: ts=4:sw=4:et:ai
