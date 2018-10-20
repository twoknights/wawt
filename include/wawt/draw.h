/** @file draw.h
 *  @brief Provides definition of draw behaviors.
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

#ifndef WAWT_DRAW_H
#define WAWT_DRAW_H

#include "wawt/wawt.h"

#include <any>
#include <functional>
#include <ostream>
#include <string>
#include <tuple>
#include <utility>

namespace Wawt {

                            //=================
                            // struct DrawData
                            //=================


struct DrawData {
    using CharSize          = uint16_t;
    enum BulletMark {
        eNONE
      , eSQUARE
      , eROUND
      , eUPARROW
      , eDOWNARROW
    };

    uint16_t            d_widgetId          = 0;
    uint16_t            d_leftMark:1,
                        d_selected:1,
                        d_disableEffect:1,
                        d_hidden:1,
                        d_labelMark:3,
                        d_relativeId:9;
    Rectangle           d_rectangle{};
    Dimensions          d_borders{};
    Rectangle           d_labelBounds{};
    StringView_t        d_label{};
    std::any            d_options{};
    const char         *d_className         = nullptr;
    CharSize            d_charSize          = 0;

    DrawData()                          = default;

    DrawData(char const * const className) noexcept
        : d_leftMark(true)
        , d_selected(false)
        , d_disableEffect(false)
        , d_hidden(false)
        , d_labelMark(eNONE)
        , d_relativeId{}
        , d_className(className) { }
};

                            //===================
                            // class DrawProtocol
                            //===================

class  DrawProtocol {
  public:
    virtual ~DrawProtocol() { }

    virtual bool  draw(const DrawData&      drawData)               noexcept=0;

    virtual bool  getTextMetrics(Dimensions          *textBounds,
                                 DrawData::CharSize  *charSize,
                                 const DrawData&      drawData,
                                 DrawData::CharSize   upperLimit)   noexcept=0;
};

                                //===========
                                // class Draw
                                //===========

class  Draw : public DrawProtocol {
    std::ostream&   d_os;
  public:
    Draw();

    Draw(std::ostream& os) : d_os(os) { }

    ~Draw() { }

    bool  draw(const DrawData&      drawData)                noexcept override;

    bool  getTextMetrics(Dimensions          *textBounds,
                         DrawData::CharSize  *charSize,
                         const DrawData&      drawData,
                         DrawData::CharSize   upperLimit)    noexcept override;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
