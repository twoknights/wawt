/** @file sfmladapter.cpp
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

#include "sfmladapter.h"
#include "drawoptions.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Utf.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowStyle.hpp>

#include <cmath>
#include <chrono>
#include <thread>

#include <iostream>

namespace BDS {

namespace {

void drawBox(sf::RenderWindow  *window,
             float              x,
             float              y,
             float              width,
             float              height,
             sf::Color          lineColor,
             sf::Color          fillColor,
             int                borderThickness) {
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
                int                borderThickness) {
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

} // end unnamed namespace

                                //------------------
                                // class SfmlAdapter
                                //------------------

SfmlAdapter::SfmlAdapter(sf::RenderWindow&   window,
                         const std::string&  path,
                         bool                noArrow)
: d_window(window)
, d_font()
{
    d_font.loadFromFile(path.c_str());

    if (!noArrow) {
        Wawt::s_downArrow = L'\u25BC';
        Wawt::s_upArrow   = L'\u25B2';
    }
}

// PUBLIC Wawt::Adapter INTERFACE

void
SfmlAdapter::draw(const Wawt::DrawDirective&  widget,
                  const std::wstring&        text)
{
    DrawOptions options;

    if (widget.d_options.has_value()) {
        try {
            options = std::any_cast<DrawOptions>(widget.d_options);
        }
        catch(const std::bad_any_cast& e) {
            auto [type, wid, idx] = widget.d_tracking;
            std::string msg = "Bad options (any_cast). Widget="
                            + std::to_string(wid);
            if (idx >= 0) {
                msg += " row=" + std::to_string(idx);
            }
            else {
                msg += " index=" + std::to_string(type);
            }
            throw Wawt::Exception(msg);                                 // THROW
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

    if (widget.d_greyEffect) {
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
    drawBox(&d_window,
            float(widget.d_upperLeft.d_x),
            float(widget.d_upperLeft.d_y),
            float(widget.width()+1),
            float(widget.height()+1),
            lineColor,
            (widget.d_selected && widget.d_bulletType==Wawt::BulletType::eNONE)
                ? selectColor : fillColor,
            widget.d_borderThickness);

    if (widget.d_bulletType == Wawt::BulletType::eRADIO) {
        // Bullet size:
        auto height = widget.height();
        auto radius = float(widget.interiorHeight())/5.0;

        drawCircle(&d_window,
                   float(widget.d_upperLeft.d_x + height/2),
                   float(widget.d_upperLeft.d_y + height/2),
                   radius,
                   textColor, // line color
                   widget.d_selected ? textColor : fillColor,
                   2);
    }
    else if (widget.d_bulletType == Wawt::BulletType::eCHECK) {
        auto size   = float(widget.interiorHeight());
        auto center = float(widget.d_borderThickness + size/2.0);
        auto offset = float(center - 0.2*size);
        auto ul_x   = float(widget.d_upperLeft.d_x + offset);
        auto ul_y   = float(widget.d_upperLeft.d_y + offset);

        drawBox(&d_window,
                ul_x,
                ul_y,
                float(0.4*size),
                float(0.4*size),
                textColor,
                widget.d_selected ? textColor : fillColor,
                2);
    }

    if (!text.empty()) {
        sf::Text  label{text, d_font, widget.d_charSize};

        label.setFillColor(textColor);

        if (options.d_boldEffect) {
            label.setStyle(sf::Text::Bold);
        }
        auto centery = (widget.d_upperLeft.d_y + widget.d_lowerRight.d_y)/2.0f;
        auto bounds  = label.getLocalBounds();

        label.setOrigin(bounds.left, bounds.top + bounds.height/2.0f);
        label.setPosition(widget.d_startx, centery);
        d_window.draw(label);
    }
    return;                                                           // RETURN
}

void
SfmlAdapter::getTextMetrics(Wawt::DrawDirective   *parameters,
                            Wawt::TextMetrics     *metrics,
                            const std::wstring&   text,
                            double                startLimit)
{
    assert(metrics->d_textHeight > 0);    // these are upper limits
    assert(metrics->d_textWidth > 0);     // bullet size excluded

    DrawOptions   effects;
    sf::String    string(text);
    sf::FloatRect bounds(0,0,0,0);
    uint16_t      charSize = uint16_t(std::round(parameters->d_charSize));
    sf::Text      label{string, d_font, charSize};

    if (parameters->d_options.has_value()) {
        effects = std::any_cast<DrawOptions>(parameters->d_options);
    }

    if (effects.d_boldEffect) {
        label.setStyle(sf::Text::Bold);
    }
    uint16_t upperLimit = uint16_t(std::round(startLimit));

    if (upperLimit == 0) {
        bounds = label.getLocalBounds();
    }
    else {
        auto lowerLimit = 1u;
        charSize        = upperLimit;

        while (upperLimit - lowerLimit > 1) {
            label.setCharacterSize(charSize);
            auto newBounds = label.getLocalBounds();
            auto lineSpacing = d_font.getLineSpacing(charSize);

            if (lineSpacing      >= metrics->d_textHeight
             || newBounds.width  >= metrics->d_textWidth) {
                upperLimit = charSize;
            }
            else {
                lowerLimit = charSize;

                bounds.width  = newBounds.width;
                bounds.height = lineSpacing;
            }
            charSize   = (upperLimit + lowerLimit)/2;
        }
        parameters->d_charSize = lowerLimit;
    }
    metrics->d_textWidth  = bounds.width;
    metrics->d_textHeight = bounds.height;
    return;                                                           // RETURN
}

void
SfmlWindow::eventLoop(sf::RenderWindow&                 window,
                      WawtConnector&                    connector,
                      const std::chrono::milliseconds&  pollInterval,
                      int                               minWidth,
                      int                               minHeight)
{
    Wawt::FocusCb   onKey;
    Wawt::EventUpCb mouseUp;

    while (window.isOpen()) {
        sf::Event event;

        if (window.waitEvent(event)) {
            try {
                if (event.type == sf::Event::Closed) {
                    connector.shutdownRequested([&window]() {
                                                    window.close();
                                                });
                }
                else if (event.type == sf::Event::Resized) {
                    float width  = static_cast<float>(event.size.width); 
                    float height = static_cast<float>(event.size.height); 

                    if (width < minWidth) {
                        width = float(minWidth);
                    }

                    if (height < minHeight) {
                        height = float(minHeight);
                    }

                    sf::View view(sf::FloatRect(0, 0, width, height));
                    connector.resize(width, height);

                    window.clear();
                    window.setView(view);
                    connector.draw();
                    window.display();
                }
                else if (event.type == sf::Event::MouseButtonPressed
                      && event.mouseButton.button == sf::Mouse::Button::Left) {
                    window.clear();
                    mouseUp = connector.downEvent(event.mouseButton.x,
                                                   event.mouseButton.y);
                    connector.draw();
                    window.display();
                }
                else if (event.type == sf::Event::MouseButtonReleased
                      && event.mouseButton.button == sf::Mouse::Button::Left) {
                    window.clear();

                    if (mouseUp) {

                        if (onKey) {
                            onKey(L'\0'); // erase cursor
                        }
                        onKey = mouseUp(event.mouseButton.x,
                                        event.mouseButton.y,
                                        true);

                        if (onKey) {
                            onKey(L'\0'); // show cursor
                        }
                    }
                    connector.draw();
                    window.display();
                }
                else if (event.type == sf::Event::TextEntered) {
                    window.clear();
                    if (onKey) {
                        wchar_t pressed = L'\0';
                        sf::Utf<32>::encodeWide(event.text.unicode, &pressed);

                        if (pressed) {
                            if (onKey(pressed)) { // focus lost?
                                onKey = Wawt::FocusCb();
                            }
                        }
                    }
                    connector.draw();
                    window.display();
                }
            }
            catch (Wawt::Exception& ex) {
                std::cerr << ex.what() << std::endl;
                window.close();
            }
        }
        else {
            std::this_thread::sleep_for(pollInterval); // FIX THIS
            window.clear();
            connector.draw();
            window.display();
        }
    }
    return;                                                           // RETURN
}

}  // namespace BDS

// vim: ts=4:sw=4:et:ai
