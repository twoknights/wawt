///////////////////////////////////////////////////////////////////////////////
//
// ViewScreen
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

#include <wawt/draw.h>
#include <wawt/screen.h>
#include <wawt/wawtenv.h>
#include <wawt/widget.h>
#include <wawt/widgetfactory.h>

#include <SFML/System/String.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <SFML/System/Utf.hpp>

#include <drawoptions.h>
#include <sfmldrawadapter.h>

#include <fontconfig/fontconfig.h>

#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

class ViewScreen : public Wawt::ScreenImpl<ViewScreen, DrawOptions> {
  public:
    ViewScreen() : ScreenImpl() { }

    Wawt::Widget createScreenPanel();

    void resetWidgets() { }
};

Wawt::Widget
ViewScreen::createScreenPanel()
{
    using namespace Wawt;

    //*********************************************************************
    // START SCREEN DEFINITION
    //*********************************************************************
    auto screen = panel(); // Replace with screen definition

    //*********************************************************************
    // END SCREEN DEFINITION
    //*********************************************************************
    return screen;
}

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

constexpr double WIDTH  = 1280.;
constexpr double HEIGHT = 720.;

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
    sf::RenderWindow window(sf::VideoMode(float(WIDTH), float(HEIGHT)),
                            "ViewScreen",
                            sf::Style::Default,
                            sf::ContextSettings());

    SfmlDrawAdapter drawAdapter(window, path, arial);
    Wawt::WawtEnv   wawtEnv(DrawOptions::classDefaults(), &drawAdapter);

    ViewScreen screen;
    screen.setup();
    screen.activate(WIDTH, HEIGHT);

    std::cout << "Minimum widget size: " << sizeof(Wawt::panel()) << std::endl;
    std::cout << "\nSerialized screen definition:\n";
    screen.serializeScreen(std::cout);
    std::cout << "\nAdapter view:\n";
    Wawt::Draw draw;
    screen.draw(&draw);

    screen.draw();
    window.display();

    while (window.isOpen()) {
        sf::Event event;

        if (window.waitEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
    }
    return 0;
}

// vim: ts=4:sw=4:et:ai
