/** @file sfmldrawadapter.h
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

#ifndef BDS_SFMLDRAWADAPTER_H
#define BDS_SFMLDRAWADAPTER_H

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <wawt.h>
#include <string>

namespace BDS {

                            //======================
                            // class SfmlDrawAdapter
                            //======================

class SfmlDrawAdapter : public Wawt::DrawAdapter {
    // PRIVATE MANIPULATORS
    sf::Font& getFont(uint8_t index);

  public:
    // PUBLIC CLASS MEMBERS
    static std::string toAnsiString(const Wawt::String_t& string);

    // PUBLIC CREATORS
    SfmlDrawAdapter(sf::RenderWindow&   window,
                    const std::string&  defaultFontPath,
                    bool                noArrow,
                    const std::string&  otherFontPath = "");

    // PUBLIC Wawt::DrawAdapter INTERFACE

    void  draw(const Wawt::DrawDirective&  widget,
               const Wawt::String_t&       text)                   override;

    void  getTextMetrics(Wawt::DrawDirective   *parameters,
                         Wawt::TextMetrics     *metrics,
                         const Wawt::String_t&  text,
                         double                 upperLimit = 0)    override;

    // PUBLIC ACCESSORS
    bool   isGood() const {
        return d_defaultOk;
    }

    bool   otherFontAvailable() const {
        return d_otherOk;
    }

  private:
    sf::RenderWindow&        d_window;
    sf::Font                 d_defaultFont;
    sf::Font                 d_otherFont;
    bool                     d_defaultOk;
    bool                     d_otherOk;
};

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
