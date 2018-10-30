/** @file sfmldrawadapter.cpp
 *  @brief Implements Wawt::Adapter protocol for SFML.
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

#include "sfmldrawadapter.h"
#include "drawoptions.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Text.hpp>

#include <cassert>
#include <cmath>
#include <cstring>
#include <chrono>
#include <thread>
#include <type_traits>

#include <iostream>

#ifdef WAWT_WIDECHAR
#define S(str) Wawt::String_t(L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) Wawt::String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace {

void drawBox(sf::RenderWindow  *window,
             float              x,
             float              y,
             float              width,
             float              height,
             sf::Color          lineColor,
             sf::Color          fillColor,
             float              border) {
    sf::RectangleShape rectangle({width, height});

    if (lineColor.a > 0 && border > 0) {
        rectangle.setOutlineColor(lineColor);
        rectangle.setOutlineThickness(-border);
    }
    rectangle.setFillColor(fillColor);
    rectangle.setPosition(x, y);
    window->draw(rectangle);
}

void drawArrow(sf::RenderWindow  *window,
               float              centerx,
               float              centery,
               float              radius,
               sf::Color          fillColor,
               bool               up) {
    sf::CircleShape triangle(radius, 3);
    triangle.setOrigin(radius, radius);

    triangle.setFillColor(fillColor);
    triangle.setPosition(centerx, centery);

    if (!up) {
        triangle.setRotation(180.f);
    }
    window->draw(triangle);
}

void drawCircle(sf::RenderWindow  *window,
                float              centerx,
                float              centery,
                float              radius,
                sf::Color          lineColor,
                sf::Color          fillColor,
                float              borderThickness) {
    sf::CircleShape circle(radius, 4+int(radius));
    circle.setOrigin(radius, radius);

    if (lineColor.a > 0 && borderThickness > 0) {
        circle.setOutlineThickness(borderThickness);
        circle.setOutlineColor(lineColor);
    }
    circle.setFillColor(fillColor);
    circle.setPosition(centerx, centery);
    window->draw(circle);
}

inline
sf::String toString(const Wawt::String_t& str) {
    if constexpr(std::is_same_v<Wawt::String_t, std::u32string>) {
        return sf::String(reinterpret_cast<const sf::Uint32*>(str.data()));
    }
    else if constexpr(std::is_same_v<Wawt::String_t, std::wstring>) {
        return sf::String(reinterpret_cast<const wchar_t*>(str.data()));
    }
    else if constexpr(std::is_same_v<Wawt::String_t, std::string>) {
        std::basic_string<sf::Uint32> to;
        sf::Utf8::toUtf32(str.begin(), str.end(), std::back_inserter(to));
        return sf::String(to);
    }
    else {
        assert(!"Unsupported string type");
    }
}

} // end unnamed namespace

                                //----------------------
                                // class SfmlDrawAdapter
                                //----------------------

// PRIVATE MANIPULATORS
sf::Font&
SfmlDrawAdapter::getFont(uint8_t index)
{
    if (!d_otherOk || index == 0) {
        return d_defaultFont;
    }
    return d_otherFont;
}

// PUBLIC CLASS MEMBERS
std::string
SfmlDrawAdapter::toAnsiString(const std::wstring& string)
{
    return sf::String(string).toAnsiString();
}

std::string
SfmlDrawAdapter::toAnsiString(const std::string& string)
{
    std::string result;

    for (auto it = string.begin(); it != string.end(); ) {
        char c   = *it;
        auto lng = (c&0340) == 0340 ? ((c&020) ? 4 : 3) : ((c&0200) ? 2 : 1);
        result += (c & 0177);
        it += lng;
    }
    return result;
}

// PUBLIC CONSTRUCTORS
SfmlDrawAdapter::SfmlDrawAdapter(sf::RenderWindow&   window,
                                 const std::string&  defaultFontPath,
                                 const std::string&  otherFontPath)
: d_window(window)
, d_defaultFont()
, d_otherFont()
, d_defaultOk()
, d_otherOk()
{
    d_defaultOk = d_defaultFont.loadFromFile(defaultFontPath.c_str());

    if (d_defaultOk) {
        if (!otherFontPath.empty()) {
            d_otherOk = d_otherFont.loadFromFile(otherFontPath.c_str());
        }
    }
    else {
        if (!otherFontPath.empty()) {
            d_defaultOk = d_defaultFont.loadFromFile(otherFontPath.c_str());
        }
    }
}

// PUBLIC Wawt::Adapter INTERFACE

bool
SfmlDrawAdapter::draw(const Wawt::Layout::Result&    box,
                      const Wawt::Widget::Settings&  settings) noexcept
{
    DrawOptions options;

    if (settings.d_options.has_value()) {
        try {
            options = std::any_cast<DrawOptions>(settings.d_options);
        }
        catch(const std::bad_any_cast& e) {
            assert(!"draw wanted DrawOptions.");
            return false;
        }
    }
    sf::Color lineColor{options.d_lineColor.d_red,
                        options.d_lineColor.d_green,
                        options.d_lineColor.d_blue,
                        options.d_lineColor.d_alpha};

    sf::Color fillColor{options.d_fillColor.d_red,
                        options.d_fillColor.d_green,
                        options.d_fillColor.d_blue,
                        options.d_fillColor.d_alpha};

    sf::Color selectColor{options.d_selectColor.d_red,
                          options.d_selectColor.d_green,
                          options.d_selectColor.d_blue,
                          options.d_selectColor.d_alpha};

    if (settings.d_disabled) {
        if (lineColor.a == 255u) {
            lineColor.a = options.d_greyedEffect;
        }

        if (fillColor.a == 255u) {
            fillColor.a = options.d_greyedEffect;
        }
    }

    drawBox(&d_window,
            float(std::floor(box.d_upperLeft.d_x)),
            float(std::floor(box.d_upperLeft.d_y)),
            float(std::ceil(box.d_bounds.d_width)),
            float(std::ceil(box.d_bounds.d_height)),
            lineColor,
            (settings.d_selected && !settings.d_hideSelect) ? selectColor
                                                            : fillColor,
            float(std::ceil(box.d_border)));
    return true;                                                      // RETURN
}

bool
SfmlDrawAdapter::draw(const Wawt::Text::Data&        text,
                      const Wawt::Widget::Settings&  settings) noexcept
{
    DrawOptions options;

    if (settings.d_options.has_value()) {
        try {
            options = std::any_cast<DrawOptions>(settings.d_options);
        }
        catch(const std::bad_any_cast& e) {
            assert(!"draw wanted DrawOptions.");
            return false;
        }
    }

    sf::Color fillColor{options.d_fillColor.d_red,
                        options.d_fillColor.d_green,
                        options.d_fillColor.d_blue,
                        options.d_fillColor.d_alpha};

    sf::Color textColor{options.d_textColor.d_red,
                        options.d_textColor.d_green,
                        options.d_textColor.d_blue,
                        options.d_textColor.d_alpha};

    if (settings.d_disabled) {
        if (textColor.a == 255u) {
            textColor.a = options.d_greyedEffect;
        }

        if (fillColor.a == 255u) {
            fillColor.a = options.d_greyedEffect;
        }
    }
    auto bounds = sf::FloatRect{};

    if (!text.d_string.empty()) {
        sf::Font& font = getFont(options.d_fontIndex);
        sf::Text  label{toString(text.d_string.data()), font, text.d_charSize};

        label.setFillColor(textColor);

        if (options.d_boldEffect) {
            label.setStyle(sf::Text::Bold);
        }
        auto offset  = 0;
        auto centery = text.d_upperLeft.d_y + text.d_bounds.d_height/2.0;
        bounds  = label.getLocalBounds();

        if (text.d_labelMark != Wawt::Text::BulletMark::eNONE
         && text.d_leftAlignMark) {
            offset = text.d_charSize;
        }
        label.setOrigin(bounds.left, bounds.top + bounds.height/2.0f);
        label.setPosition(text.d_upperLeft.d_x + offset, centery);
        d_window.draw(label);
    }

    if (text.d_labelMark != Wawt::Text::BulletMark::eNONE) {
        auto size       = float(text.d_charSize); // Bullet size
        auto border     = std::ceil(0.05*size);
        auto xcenter    = text.d_upperLeft.d_x + size/2.;
        auto ycenter    = text.d_upperLeft.d_y + size/2.;
        auto radius     = size/5.0;

        ycenter        += bounds.top/4.;

        if (!text.d_leftAlignMark) {
            xcenter += bounds.left + bounds.width;
        }

        if (radius-border < 3.0) {
            border = 1;
        }

        if (text.d_labelMark == Wawt::Text::BulletMark::eROUND) {
            drawCircle(&d_window,
                       xcenter,
                       ycenter,
                       radius,
                       textColor, // used as line color
                       settings.d_selected ? textColor : fillColor,
                       border);
        }
        else if (text.d_labelMark == Wawt::Text::BulletMark::eDOWNARROW) {
            drawArrow(&d_window,
                      xcenter,
                      ycenter,
                      radius,
                      textColor,
                      false);
        }
        else if (text.d_labelMark == Wawt::Text::BulletMark::eUPARROW) {
            drawArrow(&d_window,
                      xcenter,
                      ycenter,
                      radius,
                      textColor,
                      true);
        }
        else if (text.d_labelMark == Wawt::Text::BulletMark::eSQUARE){
            auto ul_x  = xcenter - radius;
            auto ul_y  = ycenter - radius;

            drawBox(&d_window,
                    ul_x,
                    ul_y,
                    2*radius,
                    2*radius,
                    textColor,
                    settings.d_selected ? textColor : fillColor,
                    border);
        }

    }
    return true;                                                      // RETURN
}

bool
SfmlDrawAdapter::setTextValues(Wawt::Bounds          *textBounds,
                               Wawt::Text::CharSize  *newSize,
                               const Wawt::Bounds&    container,
                               bool                   hasBulletMark,
                               Wawt::StringView_t     textString,
                               Wawt::Text::CharSize   upperLimit,
                               const std::any&        options)      noexcept
{
    sf::String    string(toString(textString.data()));
    DrawOptions   effects;
    sf::FloatRect bounds(0,0,0,0);
    uint16_t      charSize = uint16_t(std::round(*newSize));

    if (options.has_value()) {
        try {
            effects = std::any_cast<DrawOptions>(options);
        }
        catch(...) {
            assert(!"DrawOptions were expected.");
            return false;                                             // RETURN
        }
    }
    sf::Font&     font = getFont(effects.d_fontIndex);
    sf::Text      label{string, font, charSize}; // See: upperLimit == 0 below

    if (effects.d_boldEffect) {
        label.setStyle(sf::Text::Bold);
    }

    if (upperLimit == 0) {
        bounds = label.getLocalBounds();
    }
    else {
        auto lowerLimit = 1u;
        charSize        = upperLimit-1;

        while (upperLimit - lowerLimit > 1) {
            label.setCharacterSize(charSize);
            auto newBounds   = label.getLocalBounds();
            auto widthLimit  = container.d_width;

            if (hasBulletMark) {
                widthLimit -= charSize;
            }

            if (newBounds.height + newBounds.top   >= container.d_height
             || newBounds.width  + newBounds.left  >= widthLimit) {
                upperLimit = charSize;
            }
            else {
                lowerLimit = charSize;

                bounds.width  = newBounds.width  + newBounds.left;
                bounds.height = newBounds.height + newBounds.top;
            }
            charSize   = (upperLimit + lowerLimit)/2;
        }
        *newSize = lowerLimit;
    }

    if (hasBulletMark) {
        bounds.width += *newSize;
    }
    // There is no guarantee that there is a character size that will permit
    // the string to fit in the container.  Need to check for this case:

    if (bounds.height >= container.d_height
     || bounds.width  >= container.d_width) {
        return false;                                                 // RETURN
    }
    // Character size allows string to fit the "container".
    textBounds->d_width  = bounds.width;
    textBounds->d_height = bounds.height;
    return true;                                                      // RETURN
}

// vim: ts=4:sw=4:et:ai
