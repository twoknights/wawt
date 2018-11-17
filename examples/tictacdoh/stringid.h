/** @file stringid.h
 *  @brief Multi-Lingual strings for Tic-Tac-DOH!
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

#include <wawt/wawtenv.h>

#include <atomic>

enum class StringId {
    eGameSettings
  , eSelectLanguage
  , eWaitForConnection
  , eConnectToOpponent
  , ePlayAsX
  , ePlayAsO
};

class StringIdLookup : public Wawt::WawtEnv::Translator {
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
    Wawt::StringView_t operator()(int stringId)                     override;
    
    // Set current language and return previous setting.
    Language currentLanguage(Language newCurrent);

    // PUBLIC ACCESSORS
    Language currentLanguage() const {
        return static_cast<Language>(d_currentLanguage.load());
    }
};

#endif

// vim: ts=4:sw=4:et:ai
