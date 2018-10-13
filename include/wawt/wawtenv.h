/** @file wawtenv.h
 *  @brief Provides definition of Wawt environment singleton
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

#ifndef WAWT_WAWTENV_H
#define WAWT_WAWTENV_H

#include "wawt/wawt.h"
#include "wawt/draw.h"

#include <any>
#include <array>
#include <atomic>
#include <cstdlib>
#include <map>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <unordered_set>

namespace Wawt {

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
    static float         defaultBorderThickness(const std::string& className) {
        return d_instance ? d_instance->_defaultBorderThickness(className)
                          : 0.0;
    }

    static std::any      defaultOptions(const std::string& className)  {
        return d_instance ? d_instance->_defaultOptions(className)
                          : std::any();
    }

    static DrawProtocol *drawAdapter() {
        return d_instance ? d_instance->d_drawAdapter : nullptr;
    }

    static WawtEnv      *instance() {
        return d_instance;
    }

    static StringView_t  translate(const StringView_t& phrase) {
        return d_instance->_translate(phrase);
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
    WawtEnv()
    : d_classDefaults{}
    , d_strings{}
    {
        while (d_atomicFlag.test_and_set()) {};
        
        if (d_instance == nullptr) {
            d_instance = this;
        }
        d_atomicFlag.clear();
    }

    template <typename Options, std::size_t NumClasses>
    WawtEnv(const DefaultArray<Options, NumClasses>&  classDefaults,
            DrawProtocol                             *adapter = nullptr)
    : d_classDefaults{}
    , d_strings{}
    {
        while (d_atomicFlag.test_and_set()) {};
        
        if (d_instance == nullptr) {
            d_instance     = this;
            d_drawAdapter  = adapter;

            for (auto& [className, border, options] : classDefaults) {
                d_classDefaults.try_emplace(className, border, options);
            }
        }
        d_atomicFlag.clear();
    }

    // PUBLIC DESTRUCTOR
    ~WawtEnv() {
        while (d_atomicFlag.test_and_set()) {};
        if (d_instance == this) {
            d_instance = nullptr;
        }
        d_atomicFlag.clear();
    }

  private:
    // PRIVATE CLASS MEMBERS
    static std::atomic_flag     d_atomicFlag;
    static WawtEnv             *d_instance;
    static DrawProtocol        *d_drawAdapter;

    // PRIVATE MANIPULATORS
    StringView_t      _translate(const StringView_t& phrase) {
        return *d_strings.emplace(phrase).first; // TBD: translate
    }

    // PRIVATE ACCESSORS
    float       _defaultBorderThickness(const std::string& className) const
                                                                     noexcept {
        auto it = d_classDefaults.find(className);
        return d_classDefaults.end() != it ? it->second.first : 0.0;
    }

    std::any    _defaultOptions(const std::string& className) const
                                                                     noexcept {
        auto it = d_classDefaults.find(className);
        return d_classDefaults.end() != it ? it->second.second : std::any();
    }

    // PRIVATE DATA MEMBERS
    std::map<std::string, Defaults> d_classDefaults{};
    std::unordered_set<String_t>    d_strings{};
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
