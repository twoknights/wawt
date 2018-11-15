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

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>

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

constexpr double       COS30        = 0.86602540378443865;
constexpr double       BOARDWIDTH   = 23.;
constexpr double       BOARDHEIGHT  = 44.*COS30;
constexpr unsigned int kCOLUMNS     = 15u;
constexpr unsigned int kROWS        = 22u;

void
drawHexBoard(Wawt::Widget *widget, Wawt::DrawProtocol *adapter, double mx, double my) {
    auto  window      = dynamic_cast<SfmlDrawAdapter*>(adapter)->window();
    auto& layout      = widget->layoutData();
    auto  upperLeft_x = layout.d_upperLeft.d_x;
    auto  upperLeft_y = layout.d_upperLeft.d_y;
    auto  boxwidth    = layout.d_bounds.d_width;
    auto  boxheight   = layout.d_bounds.d_height;

    auto  radius      = std::min(boxwidth/BOARDWIDTH, boxheight/BOARDHEIGHT);
    auto  corner_x    = std::ceil(upperLeft_x
                                  + std::round((boxwidth  - radius*BOARDWIDTH)/2.));
    auto  corner_y    = std::ceil(upperLeft_y
                                  + std::round((boxheight - radius*BOARDHEIGHT)/2.));

    // Draw the hex board on a black background
    auto  x           = 0.f;
    auto  y           = float(COS30*radius);

    auto  dx1         = radius/2.f; // sin(30)*s
    auto  dx2         = 3.f*radius/2.f;
    auto  dx3         = 2.f*radius;
    auto  dx4         = 3.f*radius;

    auto  hexCount    = (kCOLUMNS+1)/2;
    auto  width       = float(std::floor(3*radius*hexCount-radius));
    auto  height      = std::floor(2*kROWS*y);

    sf::RectangleShape board({width, height});
    board.setPosition(corner_x, corner_y);
    board.setFillColor(sf::Color::Black);
    window->draw(board);

    sf::Color               grey(128,128,128);
    std::vector<sf::Vertex> lineStrip;
    std::vector<sf::Vertex> lines;
    lineStrip.reserve(4*hexCount);
    lines.reserve(4*hexCount);

    for (auto i = 0u; i < hexCount; ++i) {
        auto p1 = sf::Vector2f(x,     0.);
        auto p2 = sf::Vector2f(x+dx1,  y);
        auto p3 = sf::Vector2f(x+dx2,  y);
        auto p4 = sf::Vector2f(x+dx3, 0.);

        lineStrip.emplace_back(p1, grey);
        lineStrip.emplace_back(p2, grey);
        lineStrip.emplace_back(p3, grey);
        lineStrip.emplace_back(p4, grey);

        p1.y = p4.y = 2.*y;

        lines.emplace_back(p1, grey);
        lines.emplace_back(p2, grey);
        lines.emplace_back(p3, grey);
        lines.emplace_back(p4, grey);

        x += dx4;
    }
    sf::RenderStates state;
    state.transform.translate(corner_x, corner_y);

    for (auto i = 0u; i < kROWS; ++i) {
        window->draw(lineStrip.data(), lineStrip.size(), sf::LineStrip, state);
        window->draw(lines.data(), lines.size(), sf::Lines, state);
        state.transform.translate(0., 2.*y);
    }

    if (mx > 0 && my > 0) {
        // Hilite the hex the mouse is in while the mouse button is down.

        // First convert the point clicked to hex coordinates: r, g, b.
        auto hex_x  = mx - corner_x - radius;
        auto hex_y  = my - corner_y;
        auto r      = - hex_x/3./radius - hex_y/2./COS30/radius;
        auto g      = - hex_x/3./radius + hex_y/2./COS30/radius;

        // The two sector coordinates (R,G) are fractional, and permit just 4
        // combination of values depending on how they are rounded (up/down).
        // However a point on a hex grid has 6 possible hexes to which it could
        // be "rounded". We use R+G+B=0 to introduce "B" which can also be
        // rounded. Then the coordinate that introduced the greatest rounding
        // error is "dropped".  The remaining two can then determine the R and
        // G values.
        auto b = -g - r;

        auto rr = std::round(r);
        auto rg = std::round(g);
        auto rb = std::round(b);

        auto dr = std::abs(rr-r);
        auto dg = std::abs(rg-g);
        auto db = std::abs(rb-b);

        if (dr > db && dr > dg) {
            rr = -rg - rb;
        }
        else if (dg > db) { // also implies dg >= dr given the previous if
            rg = -rr - rb;
        }

        // Before going any further, see if these coordinates are on the game
        // board:
        rb = -rr - rg;
        
        if (int(rb) < 0 || int(rb) >= int(kCOLUMNS)) {
            return;                                                   // RETURN
        }
        auto rorigin = 1 + int(rb)/2;
        auto deltar  = -int(rr) - rorigin;
        auto upper   = int(kROWS) - (int(rb) % 2 == 0 ? 1 : 0);

        if (deltar < 0 || deltar >= upper) {
            return;                                                   // RETURN
        }
        // Now convert the hex coordinates r,g to the x,y coordinates of the
        // hex's center, and draw.
        hex_x = corner_x + radius * (1. - 1.5 * (rr+rg));
        hex_y = corner_y + radius *     COS30 * (rg-rr);

        sf::CircleShape hex(radius, 6);
        hex.setOrigin(radius, radius);
        hex.setRotation(30);
        hex.setFillColor(sf::Color::White);
        hex.setPosition(hex_x, hex_y);
        window->draw(hex);
    }
    return;                                                           // RETURN
}

#include "addon.h"
#include "label.h"
#include "panel.h"
#include "bullet.h"
#include "list.h"
#include "button.h"
#include "canvas.h"

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

    Wawt::EventRouter::Handle panels, labels, bullets, lists, buttons, addons,
                              canvas;
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
                                    [&router,&canvas](auto) {
                                       router.activate<Canvas>(canvas);
                                    });
    canvas  = router.create<Canvas>("Canvas",
                                    [&router,&addons](auto) {
                                       router.activate<Addons>(addons);
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
