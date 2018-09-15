/** stringid.cpp
 *
 *  Map StringId to string; permit setting current language
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

#include "stringid.h"

#include <array>

// String literals must have the appropriate prefix.
// See: Wawt::Char_t and Wawt::String_t

#undef  C
//#define C(str) (str)      // ANSI strings  (std::string)
//#define C(str) (L"" str)  // UCS-2 strings (std::wstring)
#define C(str) (U"" str)  // UTF-32 strings (std::u32string)

namespace BDS {

namespace {

Wawt::String_t english(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Game Settings");
      case eSelectLanguage:
          return C("Select Language");
      case ePortNumber:
          return C("PortNumber");
      case eConnect:
          return C("Connect");
      case ePlayAsX:
          return C("Play using 'X' marker.");
      case ePlayAsO:
          return C("Play using 'O' marker.");
      default: abort();
    };
}

Wawt::String_t german(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Spieleinstellungen");
      case eSelectLanguage:
          return C("Sprache Auswählen");
      case ePortNumber:
          return C("PortNumber");
      case eConnect:
          return C("Connect");
      case ePlayAsX:
          return C("Play using 'X' marker.");
      case ePlayAsO:
          return C("Play using 'O' marker.");
      default: abort();
    };
}

Wawt::String_t spanish(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Configuración del Juego");
      case eSelectLanguage:
          return C("Seleccione el Idioma");
      case ePortNumber:
          return C("PortNumber");
      case eConnect:
          return C("Connect");
      case ePlayAsX:
          return C("Play using 'X' marker.");
      case ePlayAsO:
          return C("Play using 'O' marker.");
      default: abort();
    };
}

Wawt::String_t french(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Configuration du Jeu");
      case eSelectLanguage:
          return C("Choisir la Langue");
      case ePortNumber:
          return C("PortNumber");
      case eConnect:
          return C("Connect");
      case ePlayAsX:
          return C("Play using 'X' marker.");
      case ePlayAsO:
          return C("Play using 'O' marker.");
      default: abort();
    };
}

Wawt::String_t italian(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Setup del Gioco");
      case eSelectLanguage:
          return C("Seleziona la Lingua");
      case ePortNumber:
          return C("PortNumber");
      case eConnect:
          return C("Connect");
      case ePlayAsX:
          return C("Play using 'X' marker.");
      case ePlayAsO:
          return C("Play using 'O' marker.");
      default: abort();
    };
}

Wawt::String_t polish(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Ustawienia Gry");
      case eSelectLanguage:
          return C("Wybierz Język");
      case ePortNumber:
          return C("PortNumber");
      case eConnect:
          return C("Connect");
      case ePlayAsX:
          return C("Play using 'X' marker.");
      case ePlayAsO:
          return C("Play using 'O' marker.");
      default: abort();
    };
}

Wawt::String_t russian(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Настройка Игры");
      case eSelectLanguage:
          return C("Выберите Язык");
      case ePortNumber:
          return C("PortNumber");
      case eConnect:
          return C("Connect");
      case ePlayAsX:
          return C("Play using 'X' marker.");
      case ePlayAsO:
          return C("Play using 'O' marker.");
      default: abort();
    };
}

} // unnamed namespace

                            //---------------------
                            // class StringIdLookup
                            //---------------------

Wawt::String_t
StringIdLookup::operator()(StringId id) const
{
    Language current = static_cast<Language>(d_currentLanguage.load());

    switch (current) {
      case Language::eENGLISH: return english(id);
      case Language::eGERMAN:  return german(id);
      case Language::eSPANISH: return spanish(id);
      case Language::eFRENCH:  return french(id);
      case Language::eITALIAN: return italian(id);
      case Language::ePOLISH:  return polish(id);
      case Language::eRUSSIAN: return russian(id);
      default: abort();
    }
}

}  // namespace BDS

// vim: ts=4:sw=4:et:ai
