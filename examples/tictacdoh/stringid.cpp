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

namespace {

Wawt::String_t english(StringId id) {
    switch (id) {
      case eGameSettings:
          return S("Game Settings");
      case eSelectLanguage:
          return S("Select Language");
      case eWaitForConnection:
          return S("Wait for opponent to connect.");
      case eConnectToOpponent:
          return S("Connect to your opponent's computer.");
      case ePlayAsX:
          return S("Play using 'X' marker.");
      case ePlayAsO:
          return S("Play using 'O' marker.");
      default: abort();
    };
}

Wawt::String_t german(StringId id) {
    switch (id) {
      case eGameSettings:
          return S("Spieleinstellungen");
      case eSelectLanguage:
          return S("Sprache Auswählen");
      case eWaitForConnection:
          return S("Warte auf Gegner, um zu verbinden.");
      case eConnectToOpponent:
          return S("Verbinde dich mit dem Computer deines Gegners.");
      case ePlayAsX:
          return S("Spielen Sie mit der 'X' Markierung.");
      case ePlayAsO:
          return S("Spielen Sie mit der 'O' Markierung.");
      default: abort();
    };
}

Wawt::String_t spanish(StringId id) {
    switch (id) {
      case eGameSettings:
          return S("Configuración del Juego");
      case eSelectLanguage:
          return S("Seleccione el Idioma");
      case eWaitForConnection:
          return S("Espere a que el oponente se conecte.");
      case eConnectToOpponent:
          return S("Conéctate a la computadora de tu oponente.");
      case ePlayAsX:
          return S("Juega usando el marcador 'X'.");
      case ePlayAsO:
          return S("Juega usando el marcador 'O'.");
      default: abort();
    };
}

Wawt::String_t french(StringId id) {
    switch (id) {
      case eGameSettings:
          return S("Configuration du Jeu");
      case eSelectLanguage:
          return S("Choisir la Langue");
      case eWaitForConnection:
          return S("Attendez que l'adversaire se connecte.");
      case eConnectToOpponent:
          return S("Connectez-vous à l'ordinateur de votre adversaire.");
      case ePlayAsX:
          return S("Jouer avec le marqueur 'X'.");
      case ePlayAsO:
          return S("Jouer avec le marqueur 'O'.");
      default: abort();
    };
}

Wawt::String_t italian(StringId id) {
    switch (id) {
      case eGameSettings:
          return S("Setup del Gioco");
      case eSelectLanguage:
          return S("Seleziona la Lingua");
      case eWaitForConnection:
          return S("Attendere che l'avversario si connetta.");
      case eConnectToOpponent:
          return S("Collegarsi al computer dell'avversario.");
      case ePlayAsX:
          return S("Gioca usando il marcatore 'X'.");
      case ePlayAsO:
          return S("Gioca usando il marcatore 'O'.");
      default: abort();
    };
}

Wawt::String_t polish(StringId id) {
    switch (id) {
      case eGameSettings:
          return S("Ustawienia Gry");
      case eSelectLanguage:
          return S("Wybierz Język");
      case eWaitForConnection:
          return S("Zaczekaj, aż przeciwnik się połączy.");
      case eConnectToOpponent:
          return S("Połącz się z komputerem przeciwnika.");
      case ePlayAsX:
          return S("Zagraj za pomocą znacznika 'X'.");
      case ePlayAsO:
          return S("Zagraj za pomocą znacznika 'O'.");
      default: abort();
    };
}

Wawt::String_t russian(StringId id) {
    switch (id) {
      case eGameSettings:
          return S("Настройка Игры");
      case eSelectLanguage:
          return S("Выберите Язык");
      case eWaitForConnection:
          return S("Подождите, пока противник подключится.");
      case eConnectToOpponent:
          return S("Подключитесь к компьютеру вашего оппонента.");
      case ePlayAsX:
          return S("Играйте с помощью маркера «X».");
      case ePlayAsO:
          return S("Играйте с помощью маркера «O».");
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
