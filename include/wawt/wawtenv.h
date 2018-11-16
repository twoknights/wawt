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
#include <vector>

namespace Wawt {

                                //==============
                                // class WawtEnv
                                //==============

class DrawProtocol;

class WawtEnv {
  public:
    // PUBLIC TYPES
    class Translator {
      public:
        virtual StringView_t operator()(const StringView_t& string) {
            return string;
        }

        virtual StringView_t operator()(int) {
            return StringView_t();
        }
    };
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

    static StringView_t translate(const StringView_t& string) {
        return _instance ? _instance->_translate(string) : string;
    }

    static StringView_t translate(int stringId) {
        return _instance ? _instance->_translate(stringId) : StringView_t();
    }

    static WawtEnv      *instance() {
        return _instance;
    }

    // PUBLIC CLASS DATA
    static Char_t  kFocusChg;

    static char    sButton[];
    static char    sCanvas[];
    static char    sDialog[];
    static char    sEntry[];
    static char    sItem[];
    static char    sLabel[];
    static char    sList[];
    static char    sPanel[];
    static char    sScreen[];
    static char    sScrollbox[];

    // PUBLIC CONSTRUCTOR
    WawtEnv(Translator *translator = nullptr)
    : d_optionDefaults{}, d_drawAdapter(nullptr), d_translator{translator} {
        _init();
    }

    WawtEnv(DrawProtocol *adapter, Translator *translator = nullptr)
    : d_optionDefaults{}, d_drawAdapter(adapter), d_translator{translator} {
        _init();
    }

    template <typename Options>
    WawtEnv(const DefaultOptions<Options>&  optionDefaults,
            DrawProtocol                   *adapter     = nullptr,
            Translator                     *translator  = nullptr)
    : d_optionDefaults{}, d_drawAdapter(adapter), d_translator{translator}
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
    static Translator           _translator;

    // PRIVATE MANIPULATORS
    void _init() {
        if (d_translator == nullptr) {
            d_translator = &_translator;
        }
        while (_atomicFlag.test_and_set()) {};
        
        if (_instance == nullptr) {
            _instance     = this;
        }
        _atomicFlag.clear();
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

    StringView_t    _translate(const StringView_t& string) const {
        return (*d_translator)(string);
    }

    StringView_t    _translate(int stringId) const {
        return (*d_translator)(stringId);
    }

    // PRIVATE DATA MEMBERS
    std::map<std::string, Defaults> d_optionDefaults{};
    DrawProtocol                   *d_drawAdapter    = nullptr;
    Translator                     *d_translator     = nullptr;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
