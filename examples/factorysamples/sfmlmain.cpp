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
#define S(str) String_t(L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

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
    auto path = fontPath("Verdana");

    if (path.empty()) {
        path  = fontPath("Arial");

        if (path.empty()) {
            std::cerr << "Failed to find Verdana or Arial fonts." << std::endl;
            return 0;                                                 // RETURN
        }
    }
    auto window = sf::RenderWindow(sf::VideoMode(float(WIDTH), float(HEIGHT)),
                                   "ViewScreen",
                                   sf::Style::Default,
                                   sf::ContextSettings());

    auto drawAdapter = SfmlDrawAdapter(window, path);
    auto wawtEnv     = Wawt::WawtEnv(DrawOptions::optionDefaults(),
                                     &drawAdapter);

    Wawt::EventRouter::Handle panels, labels, bullets, lists, buttons;
    auto router  = Wawt::EventRouter();

    labels  = router.create<Labels>("Label Samples",
                                    [&router,&panels](auto) {
                                       router.activate<Panels>(panels);
                                    });
    panels  = router.create<Panels>("Panel Samples",
                                    [&router,&labels](auto) {
                                       router.activate<Labels>(labels);
                                    },
                                    [&router,&bullets](auto) {
                                       router.activate<Bullets>(bullets);
                                    });
    bullets = router.create<Bullets>("Bullet Button Samples",
                                    [&router,&panels](auto) {
                                       router.activate<Panels>(panels);
                                    },
                                    [&router,&lists](auto) {
                                       router.activate<Lists>(lists);
                                    });
    lists   = router.create<Lists>("Fixed Size Lists",
                                    [&router,&bullets](auto) {
                                       router.activate<Bullets>(bullets);
                                    },
                                    [&router,&buttons](auto) {
                                       router.activate<Buttons>(buttons);
                                    });
    buttons = router.create<Buttons>("Push Buttons & Grids",
                                    [&router,&lists](auto) {
                                       router.activate<Lists>(lists);
                                    },
                                    [](auto) {
                                    });

    router.activate<Labels>(labels);

    auto shutdown = []() { return true; };

    try {
        SfmlEventLoop::run(window, router, shutdown, 5ms, WIDTH/10, HEIGHT/10);
    }
    catch (Wawt::WawtException& e) {
        std::cout << e.what() << std::endl;

        return 1;                                                     // RETURN
    }
    return 0;
}

// vim: ts=4:sw=4:et:ai
