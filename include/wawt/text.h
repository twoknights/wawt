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
#include "wawt/wawtenv.h"

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace Wawt {

                                //============
                                // struct Text
                                //============

enum class  TextAlign { eINVALID, eLEFT, eCENTER, eRIGHT };

class DrawProtocol;

struct Text {
  public:
    // PUBLIC TYPES
    using ViewFn    = std::function<StringView_t()>;

                            //====================
                            // struct Text::View_t
                            //====================

    struct View_t {
        using Char = String_t::value_type;

        View_t()                = default;

        template<typename ENUM,
                 typename = std::enable_if_t<std::is_enum_v<ENUM>
                                            || std::is_same_v<ENUM,int>>>
        View_t(ENUM stringId)
            : d_viewFn([stringId] {
                            return WawtEnv::translate(int(stringId));
                       }) { }

        template<std::size_t N>
        View_t(const Char (&str)[N])
            : d_viewFn([str] {
                            return WawtEnv::translate(StringView_t(str,N-1));
                       }) { }

        template<typename STR>
        View_t(STR&& str,
               std::enable_if_t<std::is_convertible_v<STR, StringView_t>>*
                                                                    = nullptr)
            : d_viewFn([s=std::forward<STR>(str)] { return StringView_t(s); })
            { }

        template<typename FN>
        View_t(FN&& f,
               std::enable_if_t<std::is_invocable_r<StringView_t, FN>::value>*
                                                                    = nullptr)
             : d_viewFn(std::forward<FN>(f)) { }

        ViewFn d_viewFn;
    };

                            //==================
                            // struct Text::Data
                            //==================

    enum BulletMark {
        eNONE
      , eSQUARE
      , eROUND
      , eUPARROW
      , eLEFTARROW
      , eDOWNARROW
      , eRIGHTARROW
      , eOPTIONMARK
    };

    struct Data {
        uint32_t            d_leftAlignMark:1, // left or right
                            d_useTextBounds:1,
                            d_labelMark:4,
                            d_charSize:13;
        Coordinates         d_upperLeft{};
        Bounds              d_bounds{};
        StringView_t        d_view{};

        Data() noexcept
        : d_leftAlignMark(false)
        , d_useTextBounds(false)
        , d_labelMark(BulletMark::eNONE)
        , d_charSize(0) { }

        bool resolveSizes(const Wawt::Layout::Result& container,
                          uint16_t                    upperLimit,
                          DrawProtocol               *adapter,
                          const std::any&             options)        noexcept;

        bool inside(double x, double y)                         const noexcept{
            auto dx = float(x) - d_upperLeft.d_x;
            auto dy = float(y) - d_upperLeft.d_y;
            return dx >= 0               && dy >= 0
                && dx < d_bounds.d_width && dy < d_bounds.d_height;
        }

        StringView_t view()                                     const noexcept{
            return d_view;
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
        ViewFn              d_viewFn{[] { return StringView_t(); }};

        uint16_t    upperLimit(const Wawt::Layout::Result& container) noexcept;

        Coordinates position(const Bounds&                bounds,
                             const Wawt::Layout::Result&  container) noexcept;
    };

    // PUBLIC METHODS
    bool resolveLayout(const Wawt::Layout::Result&  container,
                       DrawProtocol                *adapter,
                       const std::any&              options) noexcept;

    Data        d_data;
    Layout      d_layout;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
