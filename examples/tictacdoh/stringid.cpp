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

#undef  S
#undef  C

#ifdef WAWT_WIDECHAR
#define S(str) (L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#define S(str) (u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace {

Wawt::StringView_t english(StringId id) {
    switch (id) {
      case StringId::eGameSettings:
          return S("Game Settings");
      case StringId::eSelectLanguage:
          return S("Select Language");
      case StringId::eWaitForConnection:
          return S("Wait for opponent to connect: ");
      case StringId::eConnectToOpponent:
          return S("Connect to your opponent's computer.");
      case StringId::ePlayAsX:
          return S("Play using 'X' marker.");
      case StringId::ePlayAsO:
          return S("Play using 'O' marker.");
      default: abort();
    };
}

Wawt::StringView_t german(StringId id) {
    switch (id) {
      case StringId::eGameSettings:
          return S("Spieleinstellungen");
      case StringId::eSelectLanguage:
          return S("Sprache Auswählen");
      case StringId::eWaitForConnection:
          return S("Warte auf Gegner, um zu verbinden: ");
      case StringId::eConnectToOpponent:
          return S("Verbinde dich mit dem Computer deines Gegners.");
      case StringId::ePlayAsX:
          return S("Spielen Sie mit der 'X' Markierung.");
      case StringId::ePlayAsO:
          return S("Spielen Sie mit der 'O' Markierung.");
      default: abort();
    };
}

Wawt::StringView_t spanish(StringId id) {
    switch (id) {
      case StringId::eGameSettings:
          return S("Configuración del Juego");
      case StringId::eSelectLanguage:
          return S("Seleccione el Idioma");
      case StringId::eWaitForConnection:
          return S("Espere a que el oponente se conecte: ");
      case StringId::eConnectToOpponent:
          return S("Conéctate a la computadora de tu oponente.");
      case StringId::ePlayAsX:
          return S("Juega usando el marcador 'X'.");
      case StringId::ePlayAsO:
          return S("Juega usando el marcador 'O'.");
      default: abort();
    };
}

Wawt::StringView_t french(StringId id) {
    switch (id) {
      case StringId::eGameSettings:
          return S("Configuration du Jeu");
      case StringId::eSelectLanguage:
          return S("Choisir la Langue");
      case StringId::eWaitForConnection:
          return S("Attendez que l'adversaire se connecte: ");
      case StringId::eConnectToOpponent:
          return S("Connectez-vous à l'ordinateur de votre adversaire.");
      case StringId::ePlayAsX:
          return S("Jouer avec le marqueur 'X'.");
      case StringId::ePlayAsO:
          return S("Jouer avec le marqueur 'O'.");
      default: abort();
    };
}

Wawt::StringView_t italian(StringId id) {
    switch (id) {
      case StringId::eGameSettings:
          return S("Setup del Gioco");
      case StringId::eSelectLanguage:
          return S("Seleziona la Lingua");
      case StringId::eWaitForConnection:
          return S("Attendere che l'avversario si connetta: ");
      case StringId::eConnectToOpponent:
          return S("Collegarsi al computer dell'avversario.");
      case StringId::ePlayAsX:
          return S("Gioca usando il marcatore 'X'.");
      case StringId::ePlayAsO:
          return S("Gioca usando il marcatore 'O'.");
      default: abort();
    };
}

Wawt::StringView_t polish(StringId id) {
    switch (id) {
      case StringId::eGameSettings:
          return S("Ustawienia Gry");
      case StringId::eSelectLanguage:
          return S("Wybierz Język");
      case StringId::eWaitForConnection:
          return S("Zaczekaj, aż przeciwnik się połączy: ");
      case StringId::eConnectToOpponent:
          return S("Połącz się z komputerem przeciwnika.");
      case StringId::ePlayAsX:
          return S("Zagraj za pomocą znacznika 'X'.");
      case StringId::ePlayAsO:
          return S("Zagraj za pomocą znacznika 'O'.");
      default: abort();
    };
}

Wawt::StringView_t russian(StringId id) {
    switch (id) {
      case StringId::eGameSettings:
          return S("Настройка Игры");
      case StringId::eSelectLanguage:
          return S("Выберите Язык");
      case StringId::eWaitForConnection:
          return S("Подождите, пока противник подключится: ");
      case StringId::eConnectToOpponent:
          return S("Подключитесь к компьютеру вашего оппонента.");
      case StringId::ePlayAsX:
          return S("Играйте с помощью маркера «X».");
      case StringId::ePlayAsO:
          return S("Играйте с помощью маркера «O».");
      default: abort();
    };
}

} // unnamed namespace

                            //---------------------
                            // class StringIdLookup
                            //---------------------

Wawt::StringView_t
StringIdLookup::operator()(int id)
{
    Language current  = static_cast<Language>(d_currentLanguage.load());
    StringId stringId = static_cast<StringId>(id);

    switch (current) {
      case Language::eENGLISH: return english(stringId);
      case Language::eGERMAN:  return german(stringId);
      case Language::eSPANISH: return spanish(stringId);
      case Language::eFRENCH:  return french(stringId);
      case Language::eITALIAN: return italian(stringId);
      case Language::ePOLISH:  return polish(stringId);
      case Language::eRUSSIAN: return russian(stringId);
      default: abort();
    }
}

StringIdLookup::Language
StringIdLookup::currentLanguage(Language newCurrent) {
    unsigned int newValue = static_cast<unsigned int>(newCurrent);
    return static_cast<Language>(d_currentLanguage.exchange(newValue));
}

// vim: ts=4:sw=4:et:ai
