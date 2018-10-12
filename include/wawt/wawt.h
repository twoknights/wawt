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
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_set>

namespace Wawt {

//
//! Character and string representations:
//

#ifdef WAWT_WIDECHAR
//! Character encoding:  wide-char (size & byte-order is system dependent)
using Char_t        = wchar_t
using String_t      = std::basic_string<wchar_t>;
using StringView_t  = std::basic_string_view<wchar_t>;
#else
//! Character encoding: utf32 char, utf8 strings
using Char_t        = char32_t;
using String_t      = std::basic_string<char>;
using StringView_t  = std::basic_string_view<char>;
#endif

inline
auto toString(int n) {
    if constexpr(std::is_same_v<String_t, std::string>) {
        return std::to_string(n);
    }
    else {
        return std::to_wstring(n);
    }
}

//! Compute the bytes required by a character when represented in a string.
inline
constexpr std::size_t sizeOfChar(const Char_t ch) {
    if constexpr(std::is_same_v<Char_t, wchar_t>) {
        return sizeof(wchar_t); // Wide-char is a fixed size encoding
    }
    else {
        auto c = static_cast<uint32_t>(ch);
        return (c & 0x1FF800) ? ((c & 0x1F0000) ? 4 : 3)
                              : ((c & 03600)    ? 2 : 1);
    }
}

//
//! Callback methods used in application's "event loops":
//
using FocusCb     = std::function<bool(Char_t)>;
using EventUpCb   = std::function<FocusCb(float x, float y, bool)>;

                            //==================
                            // struct Dimensions
                            //==================

struct  Dimensions {
    float           d_width             = 0;
    float           d_height            = 0;
};

                            //=================
                            // struct Rectangle
                            //=================

struct  Rectangle  {
    float           d_ux                = 0;
    float           d_uy                = 0;
    float           d_width             = 0;
    float           d_height            = 0;
    float           d_borderThickness   = 0.0;

    bool inside(float x, float y) const {
        auto dx = x - d_ux;
        auto dy = y - d_uy;
        return dx >= 0 && dy >= 0 && dx < d_width && dy < d_height;
    }
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

    // PUBLIC TYPES
    using IdType = decltype(d_id);

    // PUBLIC CONSTRUCTORS
    constexpr WidgetId()              = default;

    constexpr WidgetId(uint16_t value, bool isRelative) noexcept
        : d_id(value)
        , d_flags(isRelative ? 3 : 1) { }

    // PUBLIC MANIPULATORS
    constexpr WidgetId operator++()                   noexcept { // pre
        if (isSet()) {
            ++d_id;
        }
        return *this;
    }

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
        return (isSet() && rhs.isSet() && isRelative() == rhs.isRelative()
                                       && d_id         == rhs.d_id)
            || (!isSet() && !rhs.isSet());
    }

    constexpr bool operator!=(const WidgetId& rhs)  const noexcept {
        return !(*this == rhs);
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


                            //====================
                            // class WawtException
                            //====================

//! Wawt runtime exception
class  WawtException : public std::runtime_error {
  public:
    WawtException(const std::string& what_arg) : std::runtime_error(what_arg) {
    }

    WawtException(const std::string& what_arg, WidgetId widgetId)
        : std::runtime_error(what_arg + " id="
                                      + std::to_string(widgetId.value())) {
    }

    WawtException(const std::string& what_arg, int index)
        : std::runtime_error(what_arg + " index=" + std::to_string(index)) {
    }

    WawtException(const std::string& what_arg, WidgetId id, int index)
        : std::runtime_error(what_arg + " id=" + std::to_string(id.value())
                                      + " index=" + std::to_string(index)) {
    }
};

                                //==============
                                // class WawtEnv
                                //==============

class WawtEnv {
  public:
    // PUBLIC TYPES
    using Defaults      = std::pair<float , std::any>;

    template <typename Options>
    using OptionTuple = std::tuple<std::string, float, Options>;

    template <typename Options, std::size_t NumClasses>
    using DefaultArray  = std::array<OptionTuple<Options>,NumClasses>;

    // PUBLIC CLASS MEMBERS
    static WawtEnv *instance() {
        return d_instance.load();
    }
    // PUBLIC CLASS DATA
    static Char_t   kDownArrow;
    static Char_t   kUpArrow;
    static Char_t   kCursor;
    static Char_t   kFocusChg;

    static constexpr char    sScreen[] = "screen";
    static constexpr char    sDialog[] = "dialog";
    static constexpr char    sPanel[]  = "panel";
    static constexpr char    sLabel[]  = "label";
    static constexpr char    sPush[]   = "pushButton";
    static constexpr char    sCheck[]  = "checkBox";

    // PUBLIC CONSTRUCTOR
    WawtEnv() {
        WawtEnv* expected   = nullptr;
        WawtEnv* desired    = this;
        d_instance.compare_exchange_strong(expected, desired);
    }

    template <typename Options, std::size_t NumClasses>
    WawtEnv(const DefaultArray<Options, NumClasses>& classDefaults) {
        WawtEnv* expected   = nullptr;
        WawtEnv* desired    = this;
        d_instance.compare_exchange_strong(expected, desired);

        for (auto& [className, border, options] : classDefaults) {
            d_classDefaults.try_emplace(className, border, options);
        }
    }

    // PUBLIC DESTRUCTOR
    ~WawtEnv() {
        WawtEnv* desired    = nullptr;
        WawtEnv* expected   = this;
        d_instance.compare_exchange_strong(expected, desired);
    }

    // PUBLIC MANIPULATORS
    StringView_t      translate(const StringView_t& phrase) {
        return *d_strings.emplace(phrase).first; // TBD: translate
    }

    // PUBLIC ACCESSORS
    float       defaultBorderThickness(const std::string& className) const
                                                                     noexcept {
        auto it = d_classDefaults.find(className);
        return d_classDefaults.end() != it ? it->second.first : 0.0;
    }

    std::any    defaultOptions(const std::string& className) const
                                                                     noexcept {
        auto it = d_classDefaults.find(className);
        return d_classDefaults.end() != it ? it->second.second : std::any();
    }

  private:
    // PRIVATE CLASS MEMBERS
    static std::atomic<WawtEnv*> d_instance;

    // PRIVATE DATA MEMBERS
    std::map<std::string, Defaults> d_classDefaults{};
    std::unordered_set<String_t>    d_strings{};
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
