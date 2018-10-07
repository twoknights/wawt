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
#include <tuple>
#include <utility>

namespace Wawt {

class  DrawProtocol;

                            //===================
                            // class DrawSettings  
                            //===================

enum class BulletType { eNONE, eRADIO, eCHECK };
using PaintFn     = std::function<void(int ux, int uy, int lx, int ly)>;

struct  DrawPosition  {
    double              d_x               = 0.0;
    double              d_y               = 0.0;
};

struct DrawDirective {
    using Tracking = std::tuple<int,int,int>;
    Tracking            d_tracking; // handy debug info

    DrawPosition        d_upperLeft       {};
    DrawPosition        d_lowerRight      {};
    double              d_borderThickness = 0.0;
    BulletType          d_bulletType      = BulletType::eNONE;
    bool                d_greyEffect      = false;
    bool                d_selected        = false;
    double              d_startx          = 0.0; // for text placement
    unsigned int        d_charSize        = 0u; // in pixels
    std::any            d_options; // default: transparent box, black text

    DrawDirective() : d_tracking{-1,-1,-1} { }

    DrawDirective(std::any&& options) : d_options(std::move(options)) { }

    double height() const {
        return d_lowerRight.d_y - d_upperLeft.d_y + 1;
    }

    double interiorHeight() const {
        return height() - 2*d_borderThickness;
    }

    double interiorWidth() const {
        return width() - 2*d_borderThickness;
    }

    bool verify() const {
        return d_upperLeft.d_x < d_lowerRight.d_x
            && d_upperLeft.d_y < d_lowerRight.d_y;
    }

    double width() const {
        return d_lowerRight.d_x - d_upperLeft.d_x + 1;
    }
};

class DrawSettings : DrawDirective {
    friend class Widget;
    friend class Wawt;
    friend class List;

  public:

    DrawSettings() : DrawDirective() , d_hidden(false) , d_paintFn() { }

    template<class OptionClass>
    DrawSettings(OptionClass&& options)
        : DrawDirective(std::any(std::move(options)))
        , d_hidden(false)
        , d_paintFn() { }

    DrawSettings&& paintFn(const PaintFn& paintFn) && {
        d_paintFn = paintFn;
        return std::move(*this);
    }

    DrawSettings&& bulletType(BulletType value) && {
        d_bulletType = value;
        return std::move(*this);
    }

    BulletType& bulletType() {
        return d_bulletType;
    }

    std::any& options() {
        return d_options;
    }

    bool& selected() {
        return d_selected;
    }

    const DrawDirective& adapterView() const {
        return static_cast<const DrawDirective&>(*this);
    }

    BulletType bulletType() const {
        return d_bulletType;
    }

    double height() const {
        return DrawDirective::height();
    }

    bool hidden() const {
        return d_hidden;
    }

    const std::any& options() const {
        return d_options;
    }

    bool selected() const {
        return d_selected;
    }

    double width() const {
        return DrawDirective::width();
    }

  private:
    bool                d_hidden;
    PaintFn             d_paintFn;

    bool draw(DrawProtocol *adapter, const String_t& txt) const;
};

                            //===================
                            // class DrawProtocol
                            //===================

class  DrawProtocol {
  public:
    virtual ~DrawProtocol() { }

    virtual bool  draw(const DrawDirective&       parameters,
                       const String_t&            text)             noexcept=0;

    virtual bool  getTextMetrics(DrawDirective   *parameters,
                                 Dimensions      *metrics,
                                 const String_t&  text,
                                 double           upperLimit = 0)   noexcept=0;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
