/** @file drawprotocol.h
 *  @brief Provides definition of draw protocol and stream implementation.
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

#ifndef WAWT_DRAWPROTOCOL_H
#define WAWT_DRAWPROTOCOL_H

#include "wawt/wawt.h"
#include "wawt/widget.h"
#include "wawt/layout.h"

#include <any>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <utility>

namespace Wawt {


                            //===================
                            // class DrawProtocol
                            //===================

class  DrawProtocol {
  public:
    virtual ~DrawProtocol() { }

    virtual bool  draw(const Layout::Result&    box,
                       const Widget::Settings&  settings)           noexcept=0;

    virtual bool  draw(const Text::Data&        text,
                       const Widget::Settings&  settings)           noexcept=0;

    virtual bool  setTextValues(Bounds          *textBounds,
                                Text::CharSize  *charSize,
                                const Bounds&    container,
                                bool             hasBulletMark,
                                StringView_t     string,
                                Text::CharSize   upperLimit,
                                const std::any&  options)           noexcept=0;
};

                                //=================
                                // class DrawStream
                                //=================

class  DrawStream : public DrawProtocol {
    std::ostream&   d_os;
  public:
    DrawStream();

    DrawStream(std::ostream& os) : d_os(os) { }

    ~DrawStream() { }

    bool  draw(const Layout::Result&     box,
               const Widget::Settings&   settings)       noexcept override;

    bool  draw(const Text::Data&         text,
               const Widget::Settings&   settings)       noexcept override;

    bool  setTextValues(Bounds          *textBounds,
                        Text::CharSize  *charSize,
                        const Bounds&    container,
                        bool             hasBulletMark,
                        StringView_t     string,
                        Text::CharSize   upperLimit,
                        const std::any&  options)        noexcept override;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
