/** @file stringid.h
 *  @brief String IDs and mappings for Tic-Tac-DOH!
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

#ifndef TICTACDOH_STRINGID_H
#define TICTACDOH_STRINGID_H

#include <wawt.h>
#include <wawtscreen.h>
#include <wawteventrouter.h>

#include <drawoptions.h>
#include <sfmldrawadapter.h>

#include <atomic>

#undef  S
#undef  C
//#define S(str) Wawt::String_t(str)      // ANSI strings  (std::string)
//#define C(c) (c)
//#define S(str) Wawt::String_t(L"" str)  // UCS-2 strings (std::wstring)
//#define C(c) (L ## c)
#define S(str) Wawt::String_t(U"" str)  // UTF-32 strings (std::u32string)
#define C(c) (U ## c)


using namespace BDS;

// Note: StringId should be implicitly castable to 'uint16_t'
enum StringId {
    eNone   // 0 reserved by Wawt framework.
  , eGameSettings
  , eSelectLanguage
  , eWaitForConnection
  , eConnectToOpponent
  , ePlayAsX
  , ePlayAsO
};

class StringIdLookup {
    std::atomic_uint        d_currentLanguage; // Thread-safe

  public:
    // PUBLIC TYPES

    enum class Language : unsigned int { /// Supported languages.
        eENGLISH
      , eGERMAN
      , eSPANISH
      , eFRENCH
      , eITALIAN
      , ePOLISH
      , eRUSSIAN
    };

    // PUBLIC CONSTRUCTORS
    // Current language on startup is English.
    StringIdLookup()
        : d_currentLanguage(static_cast<unsigned int>(Language::eENGLISH)) { }

    // PUBLIC MANIPULATORS
    
    // Set current language and return previous setting.
    Language currentLanguage(Language newCurrent);

    // PUBLIC ACCESSORS
    // Map StringId to string.
    Wawt::String_t operator()(StringId lookup) const;

    operator Wawt::TextMapper() {
        return [this](Wawt::TextId lookup) {
            return this->operator()(static_cast<StringId>(lookup));
        };
    }

    Language currentLanguage() const {
        return static_cast<Language>(d_currentLanguage.load());
    }
};

#endif

// vim: ts=4:sw=4:et:ai
