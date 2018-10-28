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

#ifndef SFMLDRAWADAPTER_H
#define SFMLDRAWADAPTER_H

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <wawt/layout.h>
#include <wawt/text.h>
#include <wawt/widget.h>
#include <wawt/drawprotocol.h>

#include <string>

                            //======================
                            // class SfmlDrawAdapter
                            //======================

class SfmlDrawAdapter : public Wawt::DrawProtocol {
    // PRIVATE MANIPULATORS
    sf::Font& getFont(uint8_t index);

  public:
    // PUBLIC CLASS MEMBERS
    static std::string toAnsiString(const std::wstring& string);

    static std::string toAnsiString(const std::string& string); // utf8?

    // PUBLIC CREATORS
    SfmlDrawAdapter(sf::RenderWindow&   window,
                    const std::string&  defaultFontPath,
                    const std::string&  otherFontPath = "");

    // PUBLIC Wawt::DrawAdapter INTERFACE
    bool  draw(const Wawt::Layout::Result&    box,
               const Wawt::Widget::Settings&  settings)      noexcept override;

    bool  draw(const Wawt::Text::Data&        text,
               const Wawt::Widget::Settings&  settings)      noexcept override;

    bool  setTextValues(Wawt::Bounds          *textBounds,
                        Wawt::Text::CharSize  *charSize,
                        const Wawt::Bounds&    container,
                        bool                   hasBulletMark,
                        Wawt::StringView_t     string,
                        Wawt::Text::CharSize   upperLimit,
                        const std::any&        options)      noexcept override;

    // PUBLIC ACCESSORS
    bool   isGood()                                          const noexcept {
        return d_defaultOk;
    }

    bool   otherFontAvailable()                              const noexcept {
        return d_otherOk;
    }

  private:
    sf::RenderWindow&        d_window;
    sf::Font                 d_defaultFont;
    sf::Font                 d_otherFont;
    bool                     d_defaultOk;
    bool                     d_otherOk;
};

#endif
// vim: ts=4:sw=4:et:ai
