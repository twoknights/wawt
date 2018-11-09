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

#include <wawt/eventrouter.h>
#include <wawt/wawtenv.h>

#include <drawoptions.h>
#include <sfmldrawadapter.h>
#include <sfmleventloop.h>

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

Wawt::EventUpCb
dumpScreen(double, double, Wawt::Widget *widget, Wawt::Widget*) {
    auto out = Wawt::DrawStream();
    widget->screen()->draw(&out);
    return Wawt::EventUpCb();
}

#include "addon.h"
#include "label.h"
#include "panel.h"
#include "bullet.h"
#include "list.h"
#include "button.h"

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

constexpr double WIDTH  = 1280.;
constexpr double HEIGHT = 720.;

int main()
{
    auto primary = fontPath("Verdana");

    if (primary.empty()) {
        primary  = fontPath("Arial");

        if (primary.empty()) {
            std::cerr << "Failed to find Verdana or Arial fonts." << std::endl;
            return 0;                                                 // RETURN
        }
    }
    auto secondary = fontPath("Times");
    auto window = sf::RenderWindow(sf::VideoMode(float(WIDTH), float(HEIGHT)),
                                   "ViewScreen",
                                   sf::Style::Default,
                                   sf::ContextSettings());

    auto drawAdapter = SfmlDrawAdapter(window, primary, secondary);
    auto wawtEnv     = Wawt::WawtEnv(DrawOptions::optionDefaults(),
                                     &drawAdapter);

    Wawt::EventRouter::Handle panels, labels, bullets, lists, buttons, addons;
    auto router  = Wawt::EventRouter();

    labels  = router.create<Labels>("Labels",
                                    [&router,&panels](auto) {
                                       router.activate<Panels>(panels);
                                    });
    panels  = router.create<Panels>("Panels",
                                    [&router,&labels](auto) {
                                       router.activate<Labels>(labels);
                                    },
                                    [&router,&bullets](auto) {
                                       router.activate<Bullets>(bullets);
                                    });
    bullets = router.create<Bullets>("Bullets",
                                    [&router,&panels](auto) {
                                       router.activate<Panels>(panels);
                                    },
                                    [&router,&lists](auto) {
                                       router.activate<Lists>(lists);
                                    });
    lists   = router.create<Lists>("Lists",
                                    [&router,&bullets](auto) {
                                       router.activate<Bullets>(bullets);
                                    },
                                    [&router,&buttons](auto) {
                                       router.activate<Buttons>(buttons);
                                    });
    buttons = router.create<Buttons>("Buttons",
                                    [&router,&lists](auto) {
                                       router.activate<Lists>(lists);
                                    },
                                    [&router,&addons](auto) {
                                       router.activate<Addons>(addons);
                                    });
    addons  = router.create<Addons>("Addons",
                                    [&router,&buttons](auto) {
                                       router.activate<Buttons>(buttons);
                                    },
                                    [](auto) {
                                    });

    router.activate<Labels>(labels);

    auto shutdown = []() { return true; };

    try {
        SfmlEventLoop::run(window, router, shutdown, 5ms, 0, 0);
    }
    catch (Wawt::WawtException& e) {
        std::cout << e.what() << std::endl;

        return 1;                                                     // RETURN
    }
    return 0;
}

// vim: ts=4:sw=4:et:ai
