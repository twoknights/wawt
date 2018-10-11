///////////////////////////////////////////////////////////////////////////////
//
// Tic-Tac-DOH!
//
// Copyright 2018 Bruce Szablak
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////////////

#include "stringid.h"
#include "controller.h"

#include <SFML/System/String.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <SFML/System/Utf.hpp>

#include <drawoptions.h>
#include <sfmldrawadapter.h>
#include <sfmleventloop.h>
#include <sfmlipcadapter.h>

#include <fontconfig/fontconfig.h>

#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

std::string fontPath(const char *name)
{
    FcConfig  *config = FcInitLoadConfigAndFonts();
    FcPattern *pat    = FcNameParse((const FcChar8*)name);
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult result;
    FcPattern *font = FcFontMatch(config, pat, &result);

    std::string path;

    if (font) {
        FcChar8 *file = NULL;

        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
            path = (char*)file;
        }
    }
    FcPatternDestroy(pat);
    return path;
}

constexpr int WIDTH  = 1280;
constexpr int HEIGHT = 720;

int main()
{
    bool arial = false;
    std::string path = fontPath("Verdana");

    if (path.empty()) {
        path  = fontPath("Arial");

        if (path.empty()) {
            std::cerr << "Failed to find Verdana or Arial fonts." << std::endl;
            return 0;                                                 // RETURN
        }
        arial = true;
    }
    Wawt::WawtEnv wawtEnv(DrawOptions::classDefaults());
#if 0
    sf::RenderWindow window(sf::VideoMode(float(WIDTH), float(HEIGHT)),
                            "Tic-Tac-DOH!",
                            sf::Style::Default,
                            sf::ContextSettings());

    SfmlDrawAdapter   drawAdapter(window, path, arial);
#endif
    std::cout << sizeof(Wawt::Widget{}) << std::endl;
    Wawt::Draw draw;
    GameScreen screen(nullptr);
    screen.setup();
    screen.wawtScreenSetup("GameScreen",
                           Wawt::Screen::SetTimerCb(),
                           &draw); // &drawAdapter);
    screen.activate(double(WIDTH), double(HEIGHT), "X");
    screen.draw();
#if 0
    window.display();
    while (window.isOpen()) {
        sf::Event event;

        if (window.waitEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
    }
#endif
#if 0
    SfmlIpcAdapter    ipcAdapter(0);

    Wawt::EventRouter router(&drawAdapter);

    Controller        controller(router, &ipcAdapter);
    auto shutdown = [&controller]() {
                        return controller.shutdown();
                    };

    try {
        controller.startup();
        SfmlEventLoop::run(window, router, shutdown, 5ms, WIDTH/4, HEIGHT/4);
    }
    catch (Wawt::WawtException& e) {
        std::cout << e.what() << std::endl;

        return 1;                                                     // RETURN
    }
#endif
    return 0;
}

// vim: ts=4:sw=4:et:ai
