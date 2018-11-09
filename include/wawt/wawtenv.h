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

class DrawProtocol;

class WawtEnv {
  public:
    // PUBLIC TYPES
    using Defaults      = std::pair<float , std::any>;

    template <typename Options>
    using OptionTuple = std::tuple<std::string, float, Options>;

    template <typename Options>
    using DefaultOptions  = std::vector<OptionTuple<Options>>;

    // PUBLIC CLASS MEMBERS
    static float        defaultBorderThickness(const std::string& optionName) {
        return _instance ? _instance->_defaultBorderThickness(optionName)
                         : 0.0;
    }

    static const std::any& defaultOptions(const std::string& optionName)  {
        return _instance ? _instance->_defaultOptions(optionName)
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
    static Char_t  kFocusChg;

    static char    sButton[];
    static char    sDialog[];
    static char    sEntry[];
    static char    sItem[];
    static char    sLabel[];
    static char    sList[];
    static char    sPanel[];
    static char    sScreen[];
    static char    sScrollbox[];

    // PUBLIC CONSTRUCTOR
    WawtEnv() : d_optionDefaults{}, d_strings{}, d_drawAdapter(nullptr) {
        _init();
    }

    WawtEnv(DrawProtocol *adapter)
    : d_optionDefaults{}, d_strings{}, d_drawAdapter(adapter) {
        _init();
    }

    template <typename Options>
    WawtEnv(const DefaultOptions<Options>&  optionDefaults,
            DrawProtocol                   *adapter = nullptr)
    : d_optionDefaults{}, d_strings{}, d_drawAdapter(adapter)
    {
        for (auto& [optionName, border, options] : optionDefaults) {
            d_optionDefaults.try_emplace(optionName, border, options);
        }

        _init();
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
    void _init() {
        while (_atomicFlag.test_and_set()) {};
        
        if (_instance == nullptr) {
            _instance     = this;
        }
        _atomicFlag.clear();
    }

    StringView_t      _translate(const StringView_t& phrase) {
        // MUST BE THREAD-SAFE
        return *d_strings.emplace(phrase).first; // TBD: translate
    }

    // PRIVATE ACCESSORS
    float          _defaultBorderThickness(const std::string& optionName) const
                                                                     noexcept {
        auto it = d_optionDefaults.find(optionName);
        return d_optionDefaults.end() != it ? it->second.first : 0.0;
    }

    const std::any& _defaultOptions(const std::string& optionName) const
                                                                     noexcept {
        auto it = d_optionDefaults.find(optionName);
        return d_optionDefaults.end() != it ? it->second.second : _any;
    }

    // PRIVATE DATA MEMBERS
    std::map<std::string, Defaults> d_optionDefaults{};
    std::unordered_set<String_t>    d_strings{};
    DrawProtocol                   *d_drawAdapter;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
