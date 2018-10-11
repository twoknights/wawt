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

namespace BDS {

namespace {

void drawBox(sf::RenderWindow  *window,
             float              x,
             float              y,
             float              width,
             float              height,
             sf::Color          lineColor,
             sf::Color          fillColor,
             float              borderThickness) {
    sf::RectangleShape rectangle({width, height});

    if (lineColor.a > 0 && borderThickness > 0) {
        rectangle.setOutlineColor(lineColor);
        rectangle.setOutlineThickness(-borderThickness);
    }
    rectangle.setFillColor(fillColor);
    rectangle.setPosition(x, y);
    window->draw(rectangle);
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

inline
void setArrows(wchar_t& up, wchar_t& down) {
    down = L'\u25BC';
    up   = L'\u25B2';
}

inline
void setArrows(char32_t& up, char32_t& down) {
    down = U'\u25BC';
    up   = U'\u25B2';
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
                                 bool                noArrow,
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

    if (!noArrow) {
        Wawt::WawtEnv::kDownArrow  = C('\u25BC');
        Wawt::WawtEnv::kUpArrow    = C('\u25B2');
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
    auto& box = drawData.d_rectangle;
    float borderThickness = float(std::ceil(box.d_borderThickness));
    drawBox(&d_window,
            float(box.d_ux),
            float(box.d_uy),
            float(box.d_width),
            float(box.d_height),
            lineColor,
            (drawData.d_selected
          && drawData.d_labelMark==Wawt::DrawData::BulletMark::eNONE)
                ? selectColor : fillColor,
            borderThickness);

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

        label.setOrigin(bounds.left, bounds.top + bounds.height/2.0f);
        label.setPosition(drawData.d_labelBounds.d_ux, centery);
        d_window.draw(label);

        if (drawData.d_labelMark == Wawt::DrawData::BulletMark::eROUND) {
            auto lineSpacing = font.getLineSpacing(drawData.d_charSize);
            auto size        = float(drawData.d_charSize); // Bullet size
            auto height      = float(box.d_height) - (lineSpacing - size);
            auto radius      = size/4.0;

            drawCircle(&d_window,
                       float(box.d_ux + size/2. - 1.),
                       float(box.d_uy + height/2.),
                       radius,
                       textColor, // line color
                       drawData.d_selected ? textColor : fillColor,
                       2);
        }
        else if (drawData.d_labelMark == Wawt::DrawData::BulletMark::eSQUARE){
            auto lineSpacing= font.getLineSpacing(drawData.d_charSize);
            auto size       = float(drawData.d_charSize);
            auto height     = float(box.d_height) - (lineSpacing - size);
            auto xcenter    = float(borderThickness + size/2.0);
            auto ycenter    = float(height/2.0);
            auto radius     = 0.2*size;
            auto ul_x       = float(box.d_ux + xcenter - radius);
            auto ul_y       = float(box.d_uy + ycenter - radius);

            drawBox(&d_window,
                    ul_x,
                    ul_y,
                    2*radius,
                    2*radius,
                    textColor,
                    drawData.d_selected ? textColor : fillColor,
                    2);
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
            auto widthLimit  = textBounds->d_width;

            if (drawData.d_labelMark != Wawt::DrawData::BulletMark::eNONE) {
                widthLimit -= charSize;
            }

            if (lineSpacing      >= textBounds->d_height
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
    textBounds->d_width  = bounds.width;
    textBounds->d_height = bounds.height;
    return true;                                                      // RETURN
}

}  // namespace BDS

// vim: ts=4:sw=4:et:ai
