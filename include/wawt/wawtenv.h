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
#include <atomic>
#include <cstdlib>
#include <map>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <unordered_set>
#include <vector>

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

    template <typename Options>
    using DefaultOptions  = std::vector<OptionTuple<Options>>;

    // PUBLIC CLASS MEMBERS
    static float         defaultBorderThickness(const std::string& className) {
        return _instance ? _instance->_defaultBorderThickness(className)
                         : 0.0;
    }

    static const std::any& defaultOptions(const std::string& className)  {
        return _instance ? _instance->_defaultOptions(className)
                         : _any;
    }

    static DrawProtocol *drawAdapter() {
        return _instance ? _instance->d_drawAdapter : nullptr;
    }

    static WawtEnv      *instance() {
        return _instance;
    }

    static StringView_t  translate(const StringView_t& phrase) {
        return _instance->_translate(phrase);
    }

    // PUBLIC CLASS DATA
    static Char_t   kCursor;
    static Char_t   kFocusChg;

    static char    sScreen[];
    static char    sDialog[];
    static char    sPanel[];
    static char    sLabel[];
    static char    sPush[];
    static char    sBullet[];
    static char    sItem[];
    static char    sList[];

    // PUBLIC CONSTRUCTOR
    WawtEnv()
    : d_classDefaults{}
    , d_strings{}
    , d_drawAdapter(nullptr)
    {
        while (_atomicFlag.test_and_set()) {};
        
        if (_instance == nullptr) {
            _instance = this;
        }
        _atomicFlag.clear();
    }

    WawtEnv(DrawProtocol *adapter)
    : d_classDefaults{}
    , d_strings{}
    , d_drawAdapter(adapter)
    {
        while (_atomicFlag.test_and_set()) {};
        
        if (_instance == nullptr) {
            _instance = this;
        }
        _atomicFlag.clear();
    }

    template <typename Options>
    WawtEnv(const DefaultOptions<Options>&  classDefaults,
            DrawProtocol                   *adapter = nullptr)
    : d_classDefaults{}
    , d_strings{}
    , d_drawAdapter(adapter)
    {
        for (auto& [className, border, options] : classDefaults) {
            d_classDefaults.try_emplace(className, border, options);
        }

        while (_atomicFlag.test_and_set()) {};
        
        if (_instance == nullptr) {
            _instance     = this;
        }
        _atomicFlag.clear();
    }

    // PUBLIC DESTRUCTOR
    ~WawtEnv() {
        while (_atomicFlag.test_and_set()) {};
        if (_instance == this) {
            _instance = nullptr;
        }
        _atomicFlag.clear();
    }

  private:
    // PRIVATE CLASS MEMBERS
    static std::atomic_flag     _atomicFlag;
    static WawtEnv             *_instance;
    static std::any             _any;

    // PRIVATE MANIPULATORS
    StringView_t      _translate(const StringView_t& phrase) {
        return *d_strings.emplace(phrase).first; // TBD: translate
    }

    // PRIVATE ACCESSORS
    float          _defaultBorderThickness(const std::string& className) const
                                                                     noexcept {
        auto it = d_classDefaults.find(className);
        return d_classDefaults.end() != it ? it->second.first : 0.0;
    }

    const std::any& _defaultOptions(const std::string& className) const
                                                                     noexcept {
        auto it = d_classDefaults.find(className);
        return d_classDefaults.end() != it ? it->second.second : _any;
    }

    // PRIVATE DATA MEMBERS
    std::map<std::string, Defaults> d_classDefaults{};
    std::unordered_set<String_t>    d_strings{};
    DrawProtocol                   *d_drawAdapter;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
