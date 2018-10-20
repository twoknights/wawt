/** @file drawoptions.h
 *  @brief Adapter options used in the user interface.
 *
 * Copyright 2018 Bruce Szablak
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DRAWOPTIONS_H
#define DRAWOPTIONS_H

#include "wawt/wawtenv.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

                                //===================
                                // struct DrawOptions
                                //===================

struct DrawOptions {
    // PUBLIC TYPES

    //! Color definition
    class  Color  {
      public:
        uint8_t         d_red;
        uint8_t         d_green;
        uint8_t         d_blue;
        uint8_t         d_alpha;

        constexpr Color(uint8_t alpha = 0)
        : d_red(0u), d_green(0u), d_blue(0u), d_alpha(alpha) { }

        constexpr Color(uint8_t red,
                        uint8_t green,
                        uint8_t blue,
                        uint8_t alpha = 255)
        : d_red(red), d_green(green), d_blue(blue), d_alpha(alpha) { }
    };
    // PUBLIC CLASS MEMBERS
    static const Color kBLACK;
    static const Color kGREY;
    static const Color kWHITE;
    static const Color kCLEAR;

    // PUBLIC DATA MEMBERS
    Color           d_fillColor;
    Color           d_lineColor;
    Color           d_textColor;
    Color           d_selectColor;
    uint8_t         d_greyedEffect;
    bool            d_boldEffect;
    uint8_t         d_fontIndex;

    // PUBLIC CONSTRUCTORS
    constexpr DrawOptions(Color   fillColor    = kCLEAR,
                          Color   lineColor    = kCLEAR,
                          Color   textColor    = kBLACK,
                          Color   selectColor  = kWHITE,
                          bool    boldEffect   = false,
                          uint8_t greyedEffect = 128u,
                          uint8_t fontIndex    = 0u)
    : d_fillColor(fillColor)
    , d_lineColor(lineColor)
    , d_textColor(textColor)
    , d_selectColor(selectColor)
    , d_greyedEffect(greyedEffect)
    , d_boldEffect(boldEffect)
    , d_fontIndex(fontIndex) { }

    constexpr DrawOptions(const DrawOptions& copy)  = default;

    constexpr DrawOptions clone() const & {
        return DrawOptions(*this);
    }

    // PUBLIC R-Value MANIPULATORS
    constexpr DrawOptions&& bold(bool enable = true) && {
        d_boldEffect = enable;
        return std::move(*this);
    }

    constexpr DrawOptions&& fillColor(const Color& color) && {
        d_fillColor = color;
        return std::move(*this);
    }

    constexpr DrawOptions&& fillAlpha(uint8_t alpha) && {
        d_fillColor.d_alpha = alpha;
        return std::move(*this);
    }

    constexpr DrawOptions&& font(uint8_t index) && {
        d_fontIndex = index;
        return std::move(*this);
    }

    constexpr DrawOptions&& greyed(uint8_t alpha = 128u) && {
        d_greyedEffect = alpha;
        return std::move(*this);
    }

    constexpr DrawOptions&& lineColor(const Color& color) && {
        d_lineColor = color;
        return std::move(*this);
    }

    constexpr DrawOptions&& lineAlpha(uint8_t alpha) && {
        d_lineColor.d_alpha = alpha;
        return std::move(*this);
    }

    constexpr DrawOptions&& select(const Color& color) && {
        d_selectColor = color;
        return std::move(*this);
    }

    constexpr DrawOptions&& selectAlpha(uint8_t alpha) && {
        d_selectColor.d_alpha = alpha;
        return std::move(*this);
    }

    constexpr DrawOptions&& textColor(const Color& color) && {
        d_textColor = color;
        return std::move(*this);
    }

    constexpr DrawOptions&& textAlpha(uint8_t alpha) && {
        d_textColor.d_alpha = alpha;
        return std::move(*this);
    }

    static auto classDefaults() {
        using Tuple = Wawt::WawtEnv::OptionTuple<DrawOptions>;

        static std::vector defaults = {
            Tuple{ Wawt::WawtEnv::sScreen,
                   0.0, 
                   DrawOptions(Color(160u, 160u, 255u, 255u), kBLACK) },
            Tuple{ Wawt::WawtEnv::sPanel,
                   0.0, 
                   DrawOptions(kCLEAR, kBLACK) },// fill, border
            Tuple{ Wawt::WawtEnv::sLabel,
                   0.0, 
                   DrawOptions(kCLEAR, kBLACK) },// fill, border
            Tuple{ Wawt::WawtEnv::sPush,
                   5.0, 
                   DrawOptions(Color(192u, 192u, 255u, 255u), kBLACK) },
            Tuple{ Wawt::WawtEnv::sList,
                   5.0, 
                   DrawOptions(Color(192u, 192u, 255u, 255u), kBLACK) },
            Tuple{ Wawt::WawtEnv::sItem,
                   0.0, 
                   DrawOptions(kCLEAR, kBLACK) },// fill, border
            Tuple{ Wawt::WawtEnv::sDropDown,
                   5.0, 
                   DrawOptions(Color(192u, 192u, 255u, 255u), kBLACK) }
        };
        return defaults;
    }
};

constexpr const DrawOptions::Color DrawOptions::kBLACK{0u,0u,0u,255u};
constexpr const DrawOptions::Color DrawOptions::kGREY{159u,159u,159u,255u};
constexpr const DrawOptions::Color DrawOptions::kWHITE{255u,255u,255u,255u};
constexpr const DrawOptions::Color DrawOptions::kCLEAR{0u,0u,0u,0u};

#endif
// vim: ts=4:sw=4:et:ai
