/** @file layout.h
 *  @brief Support for screen layouts.
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

#ifndef WAWT_LAYOUT_H
#define WAWT_LAYOUT_H

#include "wawt/wawt.h"

#include <cmath>
#include <utility>

namespace Wawt {

class Widget;

                            //=============
                            // class Layout
                            //=============

struct Layout {
    // PUBLIC TYPES
    // Layout attributes:
    enum class Vertex  {
              eUPPER_LEFT
            , eUPPER_CENTER
            , eUPPER_RIGHT
            , eCENTER_LEFT
            , eCENTER_CENTER
            , eCENTER_RIGHT
            , eLOWER_LEFT
            , eLOWER_CENTER
            , eLOWER_RIGHT
            , eNONE
    };

    using Coordinates = std::pair<float,float>;

    class Position {
      public:
        float                   d_sX            = -1.0;
        float                   d_sY            = -1.0;
        WidgetRef               d_widgetRef     = WidgetId::kPARENT;

        // PUBLIC CONSTRUCTORS
        constexpr Position()                = default;

        constexpr Position(double x, double y)                        noexcept
            : d_sX(float(x)), d_sY(float(y)) { }

        constexpr Position(double            x,
                           double            y,
                           const WidgetRef&  widgetRef)               noexcept
            : d_sX(float(x))
            , d_sY(float(y))
            , d_widgetRef(widgetRef) { }

        // PUBLIC ACCESSORS
        Coordinates resolvePosition(const Widget& parent) const;
    };

    // PUBLIC CLASS MEMBERS
    constexpr static Layout centered(double width, double height)   noexcept {
        auto w = std::abs(width);
        auto h = std::abs(height);
        return Layout({-w, -h}, {w, h});
    }

    constexpr static Layout duplicate(WidgetId  id,
                                      double    thickness = -1.0)   noexcept {
        return Layout({-1.0, -1.0, id}, {1.0, 1.0, id}, thickness);
    }

    // PUBLIC DATA MEMBERS
    Position            d_upperLeft{};
    Position            d_lowerRight{1.0,1.0};
    Vertex              d_pin{Vertex::eNONE};
    float               d_thickness = -1.0;

    // PUBLIC CONSTRUCTORS
    constexpr Layout()                  = default;

    constexpr Layout(const Position&  upperLeft,
                     const Position&  lowerRight,
                     double           thickness = -1.0)             noexcept
        : d_upperLeft(upperLeft)
        , d_lowerRight(lowerRight)
        , d_thickness(float(thickness)) { }

    constexpr Layout(const Position&   upperLeft,
                     const Position&   lowerRight,
                     Vertex            pin,
                     double            thickness = -1.0)            noexcept
        : d_upperLeft(upperLeft)
        , d_lowerRight(lowerRight)
        , d_pin(pin)
        , d_thickness(float(thickness)) { }

    // PUBLIC Reference Qualified MANIPULATORS
    constexpr Layout&& border(double thickness) &&                  noexcept {
        d_thickness = float(thickness);
        return std::move(*this);
    }

    constexpr Layout&  border(double thickness) &                   noexcept {
        d_thickness = float(thickness);
        return *this;
    }

    constexpr Layout&& pin(Vertex vertex) &&                        noexcept {
        d_pin = vertex;
        return std::move(*this);
    }

    constexpr Layout&& translate(double x, double y) &&             noexcept {
        this->translate(x, y);
        return std::move(*this);
    }

    constexpr Layout& translate(double x, double y) &               noexcept {
        d_upperLeft.d_sX  += float(x);
        d_upperLeft.d_sY  += float(y);
        d_lowerRight.d_sX += float(x);
        d_lowerRight.d_sY += float(y);
        return *this;
    }

    // PUBLIC MANIPULATORS
    // Following requires that both Positions refer to the same widget
    Layout& scale(double sx, double sy);

    // PUBLIC ACCESSORS
    // Following requires widgets that have been "resolved".
    float       resolveBorder(const Widget& reference) const;

    Rectangle   resolveOutline(const Widget& parent) const;
};

using LayoutGenerator = std::function<Layout()>;

LayoutGenerator gridLayoutGenerator(double       borderThickness,
                                    std::size_t  widgetCount,
                                    std::size_t  columns    = 0,
                                    std::size_t *rows       = nullptr)noexcept;

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
