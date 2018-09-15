/** @file sfmladapter.h
 *  @brief Adapts SFML Graphics and Events to Wawt.
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

#ifndef BDS_SFMLADAPTER_H
#define BDS_SFMLADAPTER_H

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "wawt.h"
#include "wawtconnector.h"

#include <chrono>

namespace BDS {

class SfmlAdapter : public Wawt::DrawAdapter {
  public:

    // PUBLIC CREATORS
    SfmlAdapter(sf::RenderWindow&   window,
                const std::string&  fontPath,
                bool                noArrow);

    // PUBLIC Wawt::DrawAdapter INTERFACE

    void  draw(const Wawt::DrawDirective&  widget,
               const Wawt::String_t&       text)                   override;

    void  getTextMetrics(Wawt::DrawDirective   *parameters,
                         Wawt::TextMetrics     *metrics,
                         const Wawt::String_t&  text,
                         double                 upperLimit = 0)    override;

  private:
    sf::RenderWindow&        d_window;
    sf::Font                 d_font;
};

struct SfmlWindow {
    static void eventLoop(sf::RenderWindow&                 window,
                          WawtConnector&                    wawtManager,
                          const std::chrono::milliseconds&  pollInterval,
                          int                               minWidth,
                          int                               minHeight);
};

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
