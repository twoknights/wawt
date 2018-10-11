/** @file text.h
 *  @brief Class for widgets dealing with text strings.
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

#include <optional>
#include <utility>

namespace Wawt {

class  DrawProtocol;
struct WidgetData;
                                //===========
                                // class Text
                                //===========

struct Text {
    enum class Align { eINVALID, eLEFT, eCENTER, eRIGHT };
    using CharSizeGroup = std::optional<uint16_t>;
    using CharSizeMap   = std::map<uint16_t, uint16_t>;

    StringView_t        d_stringView;
    Align               d_alignment;
    CharSizeGroup       d_charSizeGroup;

    Text(StringView_t   string,
         CharSizeGroup  group     = CharSizeGroup(),
         Align          alignment = Align::eCENTER)
        : d_stringView(WawtEnv::instance()->translate(std::move(string)))
        , d_alignment(alignment)
        , d_charSizeGroup(group) { }

    Text(StringView_t string, Align alignment)
        : d_stringView(WawtEnv::instance()->translate(std::move(string)))
        , d_alignment(alignment)
        , d_charSizeGroup() { }

    Text setView(StringView_t string) && {
        d_stringView = string;
        return std::move(*this);
    }
};


using TextMethod = std::function<Rectangle(Text::CharSizeMap   *map,
                                           DrawProtocol        *adapter,
                                           const WidgetData&    widgetData,
                                           Text::CharSizeGroup  charSizeGroup,
                                           Text::Align          textAlign)>;

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
