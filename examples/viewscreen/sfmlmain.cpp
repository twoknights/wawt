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

#include <wawt/screen.h>
#include <wawt/wawtenv.h>
#include <wawt/widget.h>
#include <wawt/widgetfactory.h>
#include <wawt/drawprotocol.h>

#include <SFML/System/String.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <SFML/System/Utf.hpp>

#include <wawt/dropdownlist.h>
#include <drawoptions.h>
#include <sfmldrawadapter.h>

#include <fontconfig/fontconfig.h>

#include <iostream>
#include <chrono>

#ifdef WAWT_WIDECHAR
#define S(str) (L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) (u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

using namespace std::chrono_literals;

class ViewScreen : public Wawt::ScreenImpl<ViewScreen, DrawOptions> {
  public:
    ViewScreen() : ScreenImpl() { }

    Wawt::Widget createScreenPanel();

    void resetWidgets() { }

    Wawt::DropDownList d_moveClock{0.3, { { S("5") } , { S("10") } , { S("15") } }};
};

Wawt::Widget
ViewScreen::createScreenPanel()
{
    using namespace Wawt;
    using namespace Wawt::literals;

    //*********************************************************************
    // START SCREEN DEFINITION
    //*********************************************************************

    auto clockSetting = 
        concatenateTextWidgets({{-1.0,-1.0},{1.0,-0.8}},
                               2_Sz, TextAlign::eLEFT,
                               label(Layout(),
                                     S("Preferred move clock setting:"),
                                     2_Sz, TextAlign::eLEFT),
                               d_moveClock.widget());
    auto screen = panel().addChild(std::move(clockSetting));
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
    std::string path = fontPath("Verdana");

    if (path.empty()) {
        path  = fontPath("Arial");

        if (path.empty()) {
            std::cerr << "Failed to find Verdana or Arial fonts." << std::endl;
            return 0;                                                 // RETURN
        }
    }
    sf::RenderWindow window(sf::VideoMode(float(WIDTH), float(HEIGHT)),
                            "ViewScreen",
                            sf::Style::Default,
                            sf::ContextSettings());

    SfmlDrawAdapter drawAdapter(window, path);
    Wawt::WawtEnv   wawtEnv(DrawOptions::optionDefaults(), &drawAdapter);

    ViewScreen screen;
    screen.setup();

    std::cout << "Minimum widget size: " << sizeof(Wawt::panel()) << std::endl;
    screen.activate(WIDTH, HEIGHT);

    screen.draw();
    window.display();

    auto cb = screen.downEvent(780.0,10.0);
    cb(780.0,10.0,true);

    screen.draw();
    window.display();

    std::cout << "\nSerialized screen definition:\n";
    screen.serializeScreen(std::cout);
    std::cout.flush();
    std::cout << "\nAdapter view:\n";
    Wawt::DrawStream draw;
    screen.draw(&draw);

    auto width  = float{WIDTH};
    auto height = float{HEIGHT};

    while (window.isOpen()) {
        sf::Event event;

        if (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        else if (event.type == sf::Event::Resized) {
            float nwidth  = static_cast<float>(event.size.width); 
            float nheight = static_cast<float>(event.size.height); 
            if (width != nwidth || height != nheight) {
                width = nwidth; height = nheight;
                sf::View view(sf::FloatRect(0, 0, width, height));
                screen.resize(width, height);

                window.clear();
                window.setView(view);
                screen.draw();
                window.display();
            }
        }
    }
    return 0;
}

// vim: ts=4:sw=4:et:ai
