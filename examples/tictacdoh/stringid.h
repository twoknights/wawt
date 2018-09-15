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

#ifndef BDS_TICTACDOH_STRINGID_H
#define BDS_TICTACDOH_STRINGID_H

#include "wawt.h"

#include <atomic>

namespace BDS {

// Note: StringId should be implicitly castable to 'uint16_t'
enum StringId {
    eNone   // 0 reserved by Wawt framework.
  , eGameSettings
  , eSelectLanguage
  , eWaitFor
  , eConnectTo
  , eAddress
  , ePortNumber
  , eConnect
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
    Language currentLanguage(Language newCurrent) {
        return static_cast<Language>(d_currentLanguage.exchange(
                    static_cast<unsigned int>(newCurrent)));
    }

    // PUBLIC ACCESSORS
    // Map StringId to string.
    Wawt::String_t operator()(StringId lookup) const;

    Language currentLanguage() const {
        return static_cast<Language>(d_currentLanguage.load());
    }
};

} // end BDS namespace

#endif

// vim: ts=4:sw=4:et:ai
