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
             float              xborder,
             float              yborder) {
    sf::RectangleShape rectangle({width, height});

    if (lineColor.a > 0 && (xborder > 0 || yborder > 0)) {
        rectangle.setOutlineColor(lineColor);
        rectangle.setOutlineThickness(1);
    }
    rectangle.setFillColor(fillColor);
    rectangle.setPosition(x, y);
    window->draw(rectangle);

    if (lineColor.a > 0) {
        rectangle.setFillColor(lineColor);

        if (xborder >= 2) {
            rectangle.setPosition(x, y);
            rectangle.setSize({xborder, height});
            window->draw(rectangle);
            rectangle.setPosition(x+width-xborder, y);
            window->draw(rectangle);
        }

        if (yborder >= 2) {
            rectangle.setPosition(x, y);
            rectangle.setSize({width, yborder});
            window->draw(rectangle);
            rectangle.setPosition(x, y+height-yborder);
            window->draw(rectangle);
        }
    }
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

    if (up) {
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
SfmlDrawAdapter::draw(const Wawt::DrawData& drawData) noexcept
{
    DrawOptions options;

    if (drawData.d_options.has_value()) {
        try {
            options = std::any_cast<DrawOptions>(drawData.d_options);
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

    sf::Color textColor{options.d_textColor.d_red,
                        options.d_textColor.d_green,
                        options.d_textColor.d_blue,
                        options.d_textColor.d_alpha};

    sf::Color selectColor{options.d_selectColor.d_red,
                          options.d_selectColor.d_green,
                          options.d_selectColor.d_blue,
                          options.d_selectColor.d_alpha};

    if (drawData.d_disableEffect) {
        if (lineColor.a == 255u) {
            lineColor.a = options.d_greyedEffect;
        }

        if (fillColor.a == 255u) {
            fillColor.a = options.d_greyedEffect;
        }

        if (textColor.a == 255u) {
            textColor.a = options.d_greyedEffect;
        }
    }
    using namespace Wawt;

    auto& box = drawData.d_rectangle;
    drawBox(&d_window,
            float(box.d_ux),
            float(box.d_uy),
            float(box.d_width),
            float(box.d_height),
            lineColor,
            (drawData.d_selected
          && drawData.d_labelMark==DrawData::BulletMark::eNONE)
                ? selectColor : fillColor,
            drawData.d_borders.d_x,
            drawData.d_borders.d_y);

    if (!drawData.d_label.empty()) {
        sf::Font&  font = getFont(options.d_fontIndex);
        sf::Text   label{toString(drawData.d_label.data()),
                         font,
                         drawData.d_charSize};

        label.setFillColor(textColor);

        if (options.d_boldEffect) {
            label.setStyle(sf::Text::Bold);
        }
        auto centery = box.d_uy + box.d_height/2.0;
        auto bounds  = label.getLocalBounds();
        auto offset  = 0;

        if (drawData.d_labelMark != DrawData::BulletMark::eNONE
         && drawData.d_leftMark) {
            offset = drawData.d_charSize;
        }
        label.setOrigin(bounds.left, bounds.top + bounds.height/2.0f);
        label.setPosition(drawData.d_labelBounds.d_ux + offset, centery);
        d_window.draw(label);

        auto size       = float(drawData.d_charSize); // Bullet size
        auto border     = std::ceil(0.05*size);
        auto xcenter    = drawData.d_labelBounds.d_ux + size/2.;
        auto ycenter    = drawData.d_labelBounds.d_uy + size/2.;
        auto radius     = size/5.0;

        ycenter        += bounds.top/2.;

        if (!drawData.d_leftMark) {
            xcenter += bounds.left + bounds.width;
        }

        if (radius-border < 3.0) {
            border = 1;
        }

        if (drawData.d_labelMark == DrawData::BulletMark::eROUND) {
            drawCircle(&d_window,
                       xcenter,
                       ycenter,
                       radius,
                       textColor, // line color
                       drawData.d_selected ? textColor : fillColor,
                       border);
        }
        else if (drawData.d_labelMark == DrawData::BulletMark::eDOWNARROW) {
            drawArrow(&d_window,
                      xcenter,
                      ycenter,
                      radius,
                      textColor,
                      false);
        }
        else if (drawData.d_labelMark == DrawData::BulletMark::eUPARROW) {
            drawArrow(&d_window,
                      xcenter,
                      ycenter,
                      radius,
                      textColor,
                      true);
        }
        else if (drawData.d_labelMark == DrawData::BulletMark::eSQUARE){
            auto ul_x  = xcenter - radius;
            auto ul_y  = ycenter - radius;

            drawBox(&d_window,
                    ul_x,
                    ul_y,
                    2*radius,
                    2*radius,
                    textColor,
                    drawData.d_selected ? textColor : fillColor,
                    border,
                    border);
        }

    }
    return true;                                                      // RETURN
}

bool
SfmlDrawAdapter::getTextMetrics(Wawt::Dimensions          *textBounds,
                                Wawt::DrawData::CharSize  *charHeight,
                                const Wawt::DrawData&      drawData,
                                Wawt::DrawData::CharSize   upperLimit) noexcept
{
    sf::String    string(toString(drawData.d_label.data()));
    DrawOptions   effects;
    sf::FloatRect bounds(0,0,0,0);
    uint16_t      charSize = uint16_t(std::round(drawData.d_charSize));

    if (drawData.d_options.has_value()) {
        try {
            effects = std::any_cast<DrawOptions>(drawData.d_options);
        }
        catch(...) {
            assert(!"DrawOptions were expected.");
            return false;                                             // RETURN
        }
    }
    sf::Font&     font = getFont(effects.d_fontIndex);
    sf::Text      label{string, font, charSize};

    if (effects.d_boldEffect) {
        label.setStyle(sf::Text::Bold);
    }

    if (upperLimit == 0) {
        bounds = label.getLocalBounds();
    }
    else {
        auto lowerLimit = 1u;
        charSize        = upperLimit;

        while (upperLimit - lowerLimit > 1) {
            label.setCharacterSize(charSize);
            auto newBounds   = label.getLocalBounds();
            auto lineSpacing = font.getLineSpacing(charSize);
            auto widthLimit  = textBounds->d_x;

            if (drawData.d_labelMark != Wawt::DrawData::BulletMark::eNONE) {
                widthLimit -= charSize;
            }

            if (lineSpacing      >= textBounds->d_y
             || newBounds.width  >= widthLimit) {
                upperLimit = charSize;
            }
            else {
                lowerLimit = charSize;

                bounds.width  = newBounds.width;
                bounds.height = lineSpacing;
            }
            charSize   = (upperLimit + lowerLimit)/2;
        }
        *charHeight = lowerLimit;
    }
    textBounds->d_x  = bounds.width;
    textBounds->d_y = bounds.height;
    return true;                                                      // RETURN
}

// vim: ts=4:sw=4:et:ai
