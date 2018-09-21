/** @file sfmleventloop.cpp
 *  @brief Reference implementation of windows event loop using SFML
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

#include "sfmleventloop.h"

#include <wawt.h>
#include <wawteventrouter.h>

#include <SFML/System/String.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowStyle.hpp>

#include <chrono>
#include <iostream>
#include <thread>

using namespace std::literals::chrono_literals;

namespace BDS {

                            //---------------------
                            // struct SfmlEventLoop
                            //---------------------

void
SfmlEventLoop::run(sf::RenderWindow&                 window,
                   WawtEventRouter&                  router,
                   const ShutdownCb&                 shutdown,
                   const std::chrono::milliseconds&  loopInterval,
                   int                               minWidth,
                   int                               minHeight)
{
    Wawt::FocusCb   onKey;
    Wawt::EventUpCb mouseUp;

    while (window.isOpen()) {
        sf::Event event;

        try {
            if (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    if (shutdown(&window)) {
                        window.close();
                    }
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
                    router.resize(width, height);

                    window.clear();
                    window.setView(view);
                    router.draw();
                    window.display();
                }
                else if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Button::Left) {
                        mouseUp = router.downEvent(event.mouseButton.x,
                                                   event.mouseButton.y);
                        window.clear();
                        router.draw();
                        window.display();
                    }
                }
                else if (event.type == sf::Event::MouseButtonReleased) {
                    if (event.mouseButton.button == sf::Mouse::Button::Left
                     && mouseUp) {
                        if (onKey) {
                            onKey(Wawt::Char_t(0)); // erase cursor
                        }
                        onKey = mouseUp(event.mouseButton.x,
                                        event.mouseButton.y,
                                        true);

                        if (onKey) {
                            onKey(Wawt::Char_t(0)); // show cursor
                        }
                        window.clear();
                        router.draw();
                        window.display();
                    }
                }
                else if (event.type == sf::Event::TextEntered) {
                    if (onKey) {
                        window.clear();

                        auto key = static_cast<Wawt::Char_t>(event.text
                                                                  .unicode);

                        if (key) {
                            if (onKey(key)) { // focus lost?
                                onKey = Wawt::FocusCb();
                            }
                        }
                        router.draw();
                        window.display();
                    }
                }
            }
            else {
                if (router.tick(loopInterval)) {
                    window.clear();
                    router.draw();
                    window.display();
                }
            }
        }
        catch (Wawt::Exception& ex) {
            std::cerr << ex.what() << std::endl;
            window.close();
        }
    }
    return;                                                           // RETURN
}

}  // namespace BDS

// vim: ts=4:sw=4:et:ai
