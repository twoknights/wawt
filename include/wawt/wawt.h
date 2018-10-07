/** @file wawt.h
 *  @brief Provides definition of Wawt fundamental types.
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

#ifndef WAWT_WAWT_H
#define WAWT_WAWT_H

#include <any>
#include <cassert>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <string>
#include <type_traits>

namespace Wawt {

#ifndef WAWT_WIDECHAR
// Char encoding: utf8
using Char_t        = char[4];
using String_t      = std::basic_string<char>;
using StringView_t  = std::basic_string_view<char>;
#else
// Char encoding:  wide-char (compiler dependent)
using Char_t        = wchar_t
using String_t      = std::basic_string<wchar_t>;
using StringView_t  = std::basic_string_view<wchar_t>;
#endif

using TextId        = std::size_t;

constexpr static const TextId kNOID = 0;

auto toString(int n) {
    if constexpr(std::is_same_v<String_t, std::string>) {
        return std::to_string(n);
    }
    else {
        return std::to_wstring(n);
    }
}

constexpr std::size_t sizeOfChar(const Char_t ch) {
    if constexpr(std::is_same_v<Char_t, wchar_t>) {
        return sizeof(wchar_t);
    }
    else {
        auto c = *ch;
        return (c&0340) == 0340 ? ((c&020) ? 4 : 3) : ((c&0200) ? 2 : 1);
    }
}

enum class ListType { eCHECKLIST
                    , eRADIOLIST
                    , ePICKLIST
                    , eSELECTLIST
                    , eVIEWLIST
                    , eDROPDOWNLIST };

using FocusCb     = std::function<bool(Char_t)>;

using EventUpCb   = std::function<FocusCb(int x, int y, bool)>;

                            //==================
                            // struct Dimensions
                            //==================

struct  Dimensions {
    double          d_width           = 0;
    double          d_height          = 0;
};

                                //===============
                                // class WidgetId
                                //===============


class WidgetId {
    uint16_t    d_id    = 0;
    uint16_t    d_flags = 0;
  public:
    // PUBLIC CLASS MEMBERS
    static const WidgetId kPARENT;
    static const WidgetId kROOT;

    // PUBLIC CONSTRUCTORS
    constexpr WidgetId()              = default;

    constexpr WidgetId(uint16_t value, bool isRelative) noexcept
        : d_id(value)
        , d_flags(isRelative ? 3 : 1) { }

    // PUBLIC MANIPULATORS
    constexpr WidgetId operator++(int)                noexcept { // post
        WidgetId post(*this);

        if (isSet()) {
            ++d_id;
        }
        return post;
    }

    // PUBLIC ACCESSORS
    constexpr bool isSet()                          const noexcept {
        return 0 != (d_flags & 1);
    }

    constexpr bool isRelative()                     const noexcept {
        return 0 != (d_flags & 2);
    }

    constexpr bool operator==(const WidgetId& rhs)  const noexcept {
        return isSet() && rhs.isSet()
            && isRelative() == rhs.isRelative()
            && d_id         == rhs.d_id;
    }

    constexpr bool operator!=(const WidgetId& rhs)  const noexcept {
        return isSet()      != rhs.isSet()
            || isRelative() != rhs.isRelative()
            || d_id         != rhs.d_id;
    }

    constexpr bool operator<(const WidgetId& rhs)   const noexcept {
        return d_id < rhs.d_id;
    }

    constexpr bool operator>(const WidgetId& rhs)   const noexcept {
        return d_id > rhs.d_id;
    }
    
    constexpr uint16_t value()                      const noexcept {
        return d_id;
    }
    
    constexpr explicit operator int()               const noexcept {
        return static_cast<int>(d_id);
    }
};

constexpr WidgetId WidgetId::kPARENT(UINT16_MAX,true);
constexpr WidgetId WidgetId::kROOT(UINT16_MAX-1,true);

// FREE OPERATORS

inline
constexpr WidgetId operator "" _w(unsigned long long int n)     noexcept {
    return WidgetId(n, false);
}

inline
constexpr WidgetId operator "" _wr(unsigned long long int n)    noexcept {
    return WidgetId(n, true);
}

//! Wawt runtime exception
class  WawtException : public std::runtime_error {
  public:
    WawtException(const std::string& what_arg) : std::runtime_error(what_arg) {
    }

    WawtException(const std::string& what_arg, WidgetId widgetId)
        : std::runtime_error(what_arg + " id="
                                      + std::to_string(int(widgetId))) {
    }

    WawtException(const std::string& what_arg, int index)
        : std::runtime_error(what_arg + " index=" + std::to_string(index)) {
    }

    WawtException(const std::string& what_arg, WidgetId id, int index)
        : std::runtime_error(what_arg + " id=" + std::to_string(int(id))
                                      + " index=" + std::to_string(index)) {
    }
};

struct  BorderThicknessDefaults {
    double d_canvasThickness    = 1.0;
    double d_textEntryThickness = 0.0;
    double d_labelThickness     = 0.0;
    double d_buttonThickness    = 2.0;
    double d_buttonBarThickness = 1.0;
    double d_listThickness      = 2.0;
    double d_panelThickness     = 0.0;
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

constexpr std::any kNoWidgetOptions();

class Panel;
class List;
class DrawProtocol;
class Widget;

class Wawt {
  public:
    using StringMapper = std::function<String_t(int)>;

    // PUBLIC CLASS DATA
    static Char_t   kDownArrow;
    static Char_t   kUpArrow;
    static Char_t   kCursor;
    static Char_t   kFocusChg;

    // PUBLIC CLASS MEMBERS
    static Panel   scrollableList(const List&   list,
                                  bool          buttonsOnLeft = true,
                                  unsigned int  scrollLines   = 1);

    static void    removePopUp(Panel *root);

    static void    setScrollableListStartingRow(List   *list,
                                              unsigned int row);

    // PUBLIC CONSTRUCTOR
    explicit Wawt(const StringMapper&  mappingFn = StringMapper(),
                  DrawProtocol      *adapter   = 0);

    explicit Wawt(DrawProtocol *adapter) : Wawt(StringMapper(), adapter) { }

    // PUBLIC MANIPULATORS
    void     draw(const Panel& panel);

    WidgetId popUpModalDialogBox(Panel *root, Panel&& dialogBox);

    void     refreshTextMetrics(Panel *panel);

    void     resizeRootPanel(Panel   *root, double  width, double  height);

    void     resolveWidgetIds(Panel *root);

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

    const BorderThicknessDefaults& getBorderDefaults() const {
        return d_borderDefaults;
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
    using FontIdMap  = std::map<uint16_t, uint16_t>;

    // PRIVATE CLASS MEMBERS
#if 0
    static void setIds(Panel::Widget *widget, WidgetId& id);

    static void setWidgetAdapterPositions(
            Panel::Widget                  *widget,
            Panel                          *root,
            const Panel&                    panel,
            const BorderThicknessDefaults&  border,
            const WidgetOptionDefaults&     option);
#endif

    // PRIVATE MANIPULATORS
    void  setFontSizeEntry(Widget *args);

    void  setTextAndFontValues(Panel *root);

    // PRIVATE DATA
    DrawProtocol            *d_adapter_p;
    StringMapper             d_idToString;
    FontIdMap                d_fontIdToSize;
    BorderThicknessDefaults  d_borderDefaults;
    WidgetOptionDefaults     d_optionDefaults;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
