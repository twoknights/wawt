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

#include <wawt/wawt.h>
#include <wawt/eventrouter.h>
#include <wawt/ipcqueue.h>
#include <wawt/wawtenv.h>

#include <sfmldrawadapter.h>
#include <sfmleventloop.h>
#include <sfmlipcadapter.h>

#include <fontconfig/fontconfig.h>

#include <chrono>
#include <string>
#include <iostream>

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
    auto primary = fontPath("Verdana");

    if (primary.empty()) {
        primary  = fontPath("Arial");

        if (primary.empty()) {
            std::cerr << "Failed to find Verdana or Arial fonts." << std::endl;
            return 0;                                                 // RETURN
        }
    }
    auto window = sf::RenderWindow(sf::VideoMode(float(WIDTH), float(HEIGHT)),
                                   "Tic-Tac-DOH!",
                                   sf::Style::Default,
                                   sf::ContextSettings());

    auto secondary   = fontPath("Times");
    auto idMapper    = StringIdLookup();
    auto drawAdapter = SfmlDrawAdapter(window, primary, secondary);
    auto wawtEnv     = Wawt::WawtEnv(DrawOptions::optionDefaults(),
                                     &drawAdapter,
                                     &idMapper);
    auto router      = Wawt::EventRouter();
    auto tcp         = SfmlIpV4Provider();
    auto queue       = Wawt::IpcQueue(&tcp);
    auto listenPort  =
        [&tcp](const Wawt::IpcProtocol::Provider::SetupTicket& ticket) {
            return tcp.listenPort(ticket);
        };

    auto controller  = Controller(router, &idMapper, &queue, listenPort);
    auto shutdown    = [&controller]() {
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
    return 0;
}

// vim: ts=4:sw=4:et:ai
