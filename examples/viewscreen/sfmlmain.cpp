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
    //auto screen = panel(); // Replace with screen definition
#if 0
    auto screen = pushButtonGrid({{},{}, 10}, 1_F, 3, {
            { ClickCb(), "1" }, { ClickCb(), "2" }, { ClickCb(), "3" },
            { ClickCb(), "4" }, { ClickCb(), "5" }, { ClickCb(), "6" },
            { ClickCb(), "7" }, { ClickCb(), "8" }, { ClickCb(), "9" },
                                { ClickCb(), "0" }
        }, true);
    auto screen = widgetGrid({{},{}, 20}, 1,
        pushButtonGrid(Layout().border(5), 1_F, 3, {
            { ClickCb(), "1" }, { ClickCb(), "2" }, { ClickCb(), "3" }
        }),
        pushButtonGrid(Layout().border(5), 1_F, 3, {
            { ClickCb(), "4" }, { ClickCb(), "5" }, { ClickCb(), "6" }
        }),
        pushButtonGrid(Layout().border(5), 1_F, 3, {
            { ClickCb(), "7" }, { ClickCb(), "8" }, { ClickCb(), "9" }
        }),
        pushButtonGrid(Layout().border(5), 1_F, 1, {
                                { ClickCb(), "0" }
        }));
   // auto screen = panelGrid({}, 3, 3, label({}, "X", 1_F));
    const char *s = "a123456789b123456789c123456789d123456789";
    auto screen = bulletButtonGrid(Layout().border(10),
                                   true,
                                   GridFocusCb(),
                                   1_F,
                                   {s, s, s },
                                   TextAlign::eRIGHT);
#endif
    auto screen =
        panel().addChild(
                label({{-1.0,-1.0},{1.0,-.9},0.1}, S("Labels"))
                 .options(defaultOptions(WawtEnv::sLabel)
                            .fillColor(DrawOptions::Color(235,235,255))))
               .addChild(widgetGrid({{-1.0,1.0,0_wr},{1.0,1.0}}, 1,
label({}, S("The default label has no border,")),
label({}, S("no fill color,")),
label({}, S("centered; with font size selected so the label fits.")),
label({}, S("Labels can be 'left' aligned,"), TextAlign::eLEFT, 1_F),
label({}, S("or 'right' aligned,"), TextAlign::eRIGHT, 1_F),
label({}, S("and assigned to a font size group so they have matching sizes."),
    1_F),
label({}, S("С полной поддержкой utf-8 и широким характером."))));

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
