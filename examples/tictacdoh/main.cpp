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

#include "controller.h"
#include "drawoptions.h"
#include "sfmldrawadapter.h"
#include "wawtconnector.h"

#include "stringid.h"
#include "setupscreen.h"

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

using namespace BDS;

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
                            "Tic-Tac-DOH!",
                            sf::Style::Default,
                            sf::ContextSettings());

    StringIdLookup   idMapper;
    SetupScreen      setup(idMapper);
    SfmlDrawAdapter  drawAdapter(window, path, arial);

    WawtConnector    connector(&drawAdapter,
                               idMapper,
                               WIDTH,
                               HEIGHT,
                               DrawOptions::defaults());

    Controller       controller(connector);

    try {
        controller.installScreens(&setup);
    }
    catch (Wawt::Exception& e) {
        std::cout << e.what() << std::endl;

        return 1;                                                     // RETURN
    }

    SfmlWindow::eventLoop(window, connector, 50ms, WIDTH/2, HEIGHT/2);

    return 0;
}

// vim: ts=4:sw=4:et:ai
