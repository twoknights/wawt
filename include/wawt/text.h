/** @file text.h
 *  @brief Text layout and draw settings.
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

#ifndef WAWT_TEXT_H
#define WAWT_TEXT_H

#include "wawt/wawt.h"
#include "wawt/layout.h"

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <utility>

namespace Wawt {

                                //============
                                // struct Text
                                //============

enum class  TextAlign { eINVALID, eLEFT, eCENTER, eRIGHT };
struct      CharSizeGroup : std::optional<uint16_t> { };

inline constexpr CharSizeGroup operator ""_Sz(unsigned long long int n) {
    return CharSizeGroup{n};
}

constexpr CharSizeGroup kNOGROUP{};

class DrawProtocol;

struct Text {
  public:
    // PUBLIC TYPES

                            //==================
                            // struct Text::Data
                            //==================

    using CharSize          = uint16_t;

    enum BulletMark {
        eNONE
      , eSQUARE
      , eROUND
      , eUPARROW
      , eDOWNARROW
      , eOPTIONMARK
    };

    struct Data {
        uint16_t            d_leftAlignMark:1, // left or right
                            d_useTextBounds:1,
                            d_successfulLayout:1,
                            d_labelMark:4;
        Coordinates         d_upperLeft{};
        Bounds              d_bounds{};
        CharSize            d_charSize = 0;
        String_t            d_string{};

        Data()
        : d_leftAlignMark(false)
        , d_useTextBounds(false)
        , d_successfulLayout(false)
        , d_labelMark(BulletMark::eNONE) { }

        bool inside(double x, double y) const noexcept {
            auto dx = float(x) - d_upperLeft.d_x;
            auto dy = float(y) - d_upperLeft.d_y;
            return dx >= 0               && dy >= 0
                && dx < d_bounds.d_width && dy < d_bounds.d_height;
        }
    };

                            //====================
                            // struct Text::Layout
                            //====================

    using CharSizeMap       = std::map<uint16_t, uint16_t>;
    using CharSizeMapPtr    = std::shared_ptr<CharSizeMap>;

    struct Layout {
        TextAlign           d_textAlign     = TextAlign::eCENTER;
        CharSizeGroup       d_charSizeGroup{};
        CharSizeMapPtr      d_charSizeMap{};
        bool                d_refreshBounds = false;

        CharSize    upperLimit(const Wawt::Layout::Result& container) noexcept;

        Coordinates position(const Bounds&                bounds,
                             const Wawt::Layout::Result&  container) noexcept;
    };

    // PUBLIC CLASS METHODS
    static bool resolveSizes(CharSize                   *size,
                             Bounds                     *bounds,
                             const String_t&             string,
                             bool                        hasBulletMark,
                             const Wawt::Layout::Result& container,
                             CharSize                    upperLimit,
                             DrawProtocol               *adapter,
                             const std::any&             options) noexcept;

    // PUBLIC METHODS
    void resolveLayout(const Wawt::Layout::Result&  container,
                       DrawProtocol                *adapter,
                       const std::any&              options) noexcept;

    Data        d_data;
    Layout      d_layout;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
