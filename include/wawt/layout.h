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


                                //=============
                                // class Layout
                                //=============

    enum class Normalize {
              eOUTER         ///< Normalize to widget's width/2
            , eMIDDLE        ///< Normalize to middle of border.
            , eINNER         ///< Normalize to 1 pixel before inner edge
            , eDEFAULT       ///< eINNER for parent, otherwise eOUTER
    };

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

    class WidgetRef {
        WidgetId    d_widgetId{};
        Widget    **d_base  = nullptr;
      public:
        constexpr WidgetRef() { }

        constexpr WidgetRef(WidgetId id) : d_widgetId(id) { }

        template<class T>
        constexpr WidgetRef(T **ptr)
            : d_base(reinterpret_cast<Widget**>(ptr)) { }

        const Widget* getBasePointer(const Panel&      parent,
                                   const Panel&      root) const;

        WidgetId getWidgetId() const;
    };

    class Position {
      public:
        double                  d_sX            = -1.0;
        double                  d_sY            = -1.0;
        WidgetRef               d_widgetRef     = WidgetId(0, true);
        Normalize               d_normalizeX    = Normalize::eDEFAULT;
        Normalize               d_normalizeY    = Normalize::eDEFAULT;

        constexpr Position() { }

        constexpr Position(double x, double y) : d_sX(x) , d_sY(y) { }

        constexpr Position(double x, double y, WidgetRef&& widgetRef)
            : d_sX(x), d_sY(y), d_widgetRef(std::move(widgetRef)) { }

        constexpr Position(double      x,
                           double      y,
                           WidgetRef&& widgetRef,
                           Normalize   normalizeX,
                           Normalize   normalizeY)
            : d_sX(x)
            , d_sY(y)
            , d_widgetRef(std::move(widgetRef))
            , d_normalizeX(normalizeX)
            , d_normalizeY(normalizeY) { }
    };

    struct Layout {
        Position            d_upperLeft{};
        Position            d_lowerRight{};
        Vertex              d_pin{Vertex::eNONE};
        double              d_thickness = -1.0;

        constexpr Layout() { }

        constexpr Layout(const Position&  upperLeft,
                         const Position&  lowerRight,
                         double           thickness = -1.0)
            : d_upperLeft(upperLeft)
            , d_lowerRight(lowerRight)
            , d_thickness(thickness) { }

        constexpr Layout(const Position&   upperLeft,
                         const Position&   lowerRight,
                         Vertex            pin,
                         double            thickness = -1.0)
            : d_upperLeft(upperLeft)
            , d_lowerRight(lowerRight)
            , d_pin(pin)
            , d_thickness(thickness) { }

        constexpr Layout(Position&& upperLeft,
                         Position&& lowerRight,
                         double     thickness = -1.0)
            : d_upperLeft(std::move(upperLeft))
            , d_lowerRight(std::move(lowerRight))
            , d_thickness(thickness) { }

        constexpr Layout(Position&& upperLeft,
                         Position&& lowerRight,
                         Vertex     pin,
                         double     thickness = -1.0)
            : d_upperLeft(std::move(upperLeft))
            , d_lowerRight(std::move(lowerRight))
            , d_pin(pin)
            , d_thickness(thickness) { }

        static Layout slice(bool vertical, double begin, double end);

        constexpr static Layout centered(double width, double height) {
            auto w = std::abs(width);
            auto h = std::abs(height);
            return Layout({-w, -h}, {w, h});
        }

        constexpr static Layout duplicate(WidgetId id,
                                          double   thickness = -1.0) {
            return Layout({-1.0, -1.0, id}, {1.0, 1.0, id}, thickness);
        }

        constexpr Layout&& pin(Vertex vertex) && {
            d_pin = vertex;
            return std::move(*this);
        }

        constexpr Layout&& translate(double x, double y) && {
            d_upperLeft.d_sX  += x;
            d_upperLeft.d_sY  += y;
            d_lowerRight.d_sX += x;
            d_lowerRight.d_sY += y;
            return std::move(*this);
        }

        Layout&& border(double thickness) && {
            d_thickness = thickness;
            return std::move(*this);
        }
    };


} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
