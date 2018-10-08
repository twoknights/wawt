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

auto toString(int n) {
    if constexpr(std::is_same_v<String_t, std::string>) {
        return std::to_string(n);
    }
    else {
        return std::to_wstring(n);
    }
}

//! Compute the bytes required by a character when represented in a string.
constexpr std::size_t sizeOfChar(const Char_t ch) {
    if constexpr(std::is_same_v<Char_t, wchar_t>) {
        return sizeof(wchar_t); // Wide-char is a fixed size encoding
    }
    else {
        auto c = *ch; // utf8 uses a variable size encoding.
        return (c&0340) == 0340 ? ((c&020) ? 4 : 3) : ((c&0200) ? 2 : 1);
    }
}

//
//! Callback methods used in application's "event loops":
//
using FocusCb     = std::function<bool(Char_t)>;
using EventUpCb   = std::function<FocusCb(int x, int y, bool)>;

                            //==================
                            // struct Dimensions
                            //==================

struct  Dimensions {
    double          d_width             = 0;
    double          d_height            = 0;
};

                            //=================
                            // struct Rectangle
                            //=================

struct  Rectangle  {
    double          d_ux                = 0;
    double          d_uy                = 0;
    double          d_width             = 0;
    double          d_height            = 0;
    double          d_borderThickness   = 0.0;
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

                                //===========
                                // class Wawt
                                //===========

class Wawt {
    static std::atomic<Wawt*> d_instance;

    std::map<std::string, std::any> d_defaultOptions{};
    std::map<std::string, double>   d_defaultBorders{};
    std::unordered_set<String_t> d_strings{};

  public:
    static Wawt *instance() {
        return d_instance.load();
    }

    template <int NumClasses>
    Wawt(const std::Array<std::pair<std::string,std::any>,NumClasses>& options,
         const std::Array<std::pair<std::string,double>,NumClasses>& borders) {
        d_instance.compare_exchange_strong(nullptr, this);

        for (auto& [className, value] : options) {
            d_defaultOptions.emplace(className, value);
        }

        for (auto& [className, thickness] : borders) {
            d_defaultBorders.emplace(className, thickness);
        }
    }

    ~Wawt() {
        d_instance.compare_exchange_strong(this, nullptr);
    }

    std::any  defaultOptions(const std::string& className) const noexcept{
        auto it = d_defaultOptions.find(className);
        return d_defaultOptions.end() != it ? it->second : std::any();
    }

    double defaultBorderThickness(const std::string& className) const noexcept{
        auto it = d_defaultBorders.find(className);
        return d_defaultBorders.end() != it ? it->second : 0.0;
    }

    StringView_t      translate(const StringView_t& phrase) {
        return d_strings.emplace(phrase).first->second; // TBD: translate
    }
}

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
