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
#include <iostream>

// String literals must have the appropriate prefix.
// See: Wawt::Char_t and Wawt::String_t

#undef  C
//#define C(str) (str)      // ANSI strings  (std::string)
//#define C(str) (L"" str)  // UCS-2 strings (std::wstring)
#define C(str) (U"" str)  // UTF-32 strings (std::u32string)

namespace {

Wawt::String_t english(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Game Settings");
      case eSelectLanguage:
          return C("Select Language");
      case eWaitForConnection:
          return C("Wait for opponent to connect.");
      case eConnectToOpponent:
          return C("Connect to your opponent's computer.");
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
      case eWaitForConnection:
          return C("Warte auf Gegner, um zu verbinden.");
      case eConnectToOpponent:
          return C("Verbinde dich mit dem Computer deines Gegners.");
      case ePlayAsX:
          return C("Spielen Sie mit der 'X' Markierung.");
      case ePlayAsO:
          return C("Spielen Sie mit der 'O' Markierung.");
      default: abort();
    };
}

Wawt::String_t spanish(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Configuración del Juego");
      case eSelectLanguage:
          return C("Seleccione el Idioma");
      case eWaitForConnection:
          return C("Espere a que el oponente se conecte.");
      case eConnectToOpponent:
          return C("Conéctate a la computadora de tu oponente.");
      case ePlayAsX:
          return C("Juega usando el marcador 'X'.");
      case ePlayAsO:
          return C("Juega usando el marcador 'O'.");
      default: abort();
    };
}

Wawt::String_t french(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Configuration du Jeu");
      case eSelectLanguage:
          return C("Choisir la Langue");
      case eWaitForConnection:
          return C("Attendez que l'adversaire se connecte.");
      case eConnectToOpponent:
          return C("Connectez-vous à l'ordinateur de votre adversaire.");
      case ePlayAsX:
          return C("Jouer avec le marqueur 'X'.");
      case ePlayAsO:
          return C("Jouer avec le marqueur 'O'.");
      default: abort();
    };
}

Wawt::String_t italian(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Setup del Gioco");
      case eSelectLanguage:
          return C("Seleziona la Lingua");
      case eWaitForConnection:
          return C("Attendere che l'avversario si connetta.");
      case eConnectToOpponent:
          return C("Collegarsi al computer dell'avversario.");
      case ePlayAsX:
          return C("Gioca usando il marcatore 'X'.");
      case ePlayAsO:
          return C("Gioca usando il marcatore 'O'.");
      default: abort();
    };
}

Wawt::String_t polish(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Ustawienia Gry");
      case eSelectLanguage:
          return C("Wybierz Język");
      case eWaitForConnection:
          return C("Zaczekaj, aż przeciwnik się połączy.");
      case eConnectToOpponent:
          return C("Połącz się z komputerem przeciwnika.");
      case ePlayAsX:
          return C("Zagraj za pomocą znacznika 'X'.");
      case ePlayAsO:
          return C("Zagraj za pomocą znacznika 'O'.");
      default: abort();
    };
}

Wawt::String_t russian(StringId id) {
    switch (id) {
      case eGameSettings:
          return C("Настройка Игры");
      case eSelectLanguage:
          return C("Выберите Язык");
      case eWaitForConnection:
          return C("Подождите, пока противник подключится.");
      case eConnectToOpponent:
          return C("Подключитесь к компьютеру вашего оппонента.");
      case ePlayAsX:
          return C("Играйте с помощью маркера «X».");
      case ePlayAsO:
          return C("Играйте с помощью маркера «O».");
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

StringIdLookup::Language
StringIdLookup::currentLanguage(Language newCurrent) {
    unsigned int newValue = static_cast<unsigned int>(newCurrent);
    return static_cast<Language>(d_currentLanguage.exchange(newValue));
}

// vim: ts=4:sw=4:et:ai
