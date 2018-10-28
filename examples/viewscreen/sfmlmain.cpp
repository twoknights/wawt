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

#include <drawoptions.h>
#include <sfmldrawadapter.h>

#include <fontconfig/fontconfig.h>

#include <iostream>
#include <chrono>

#ifdef WAWT_WIDECHAR
#define S(str) String_t(L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

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
    //auto screen = 
    auto lineColor = defaultOptions(WawtEnv::sPanel)
                        .lineColor(defaultOptions(WawtEnv::sScreen)
                                        .d_fillColor);
    auto screen =
        panel().addChild(
                    label({{-1.0, -1.0}, {1.0, -0.9}, 0.1}, S("Labels"))
                    .options(defaultOptions(WawtEnv::sLabel)
                             .fillColor(DrawOptions::Color(235,235,255))))
               .addChild(
                    pushButtonGrid({{-1.0, 0.9}, {1.0, 1.0}}, -1.0, 2_Sz,
                                   {{OnClickCb(), S("Next")}})
                                    .border(5).options(lineColor))
               .addChild(
                    panelLayout({{-1.0, 1.0, 0_wr}, {1.0, -1.0, 1_wr}}, 0, 1,

// Start Samples:
label({}, S("The default label has no border,")),
label({}, S("no fill color,")),
label({}, S("centered; with font size selected so the label fits.")),
label({}, S("Labels can be 'left' aligned,"), 1_Sz, TextAlign::eLEFT),
label({}, S("or 'right' aligned,"), 1_Sz, TextAlign::eRIGHT),
label({},
      S("and assigned to a font size group where all share the same size."),
      1_Sz),
label({}, S("С поддержкой UTF-8 или широким символом."))));

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
    std::cout << "\nSerialized screen definition:\n";
    screen.serializeScreen(std::cout);
    std::cout.flush();
    screen.activate(WIDTH, HEIGHT);
    std::cout << "\nAdapter view:\n";
    Wawt::DrawStream draw;
    screen.draw(&draw);

    screen.draw();
    window.display();

    while (window.isOpen()) {
        sf::Event event;

        if (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        else if (event.type == sf::Event::Resized) {
            float width  = static_cast<float>(event.size.width); 
            float height = static_cast<float>(event.size.height); 
            sf::View view(sf::FloatRect(0, 0, width, height));
            screen.resize(width, height);

            window.clear();
            window.setView(view);
            screen.draw();
            window.display();
        }
    }
    return 0;
}

// vim: ts=4:sw=4:et:ai
