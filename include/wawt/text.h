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

struct DrawDirective;
class DrawProtocol;

                                //================
                                // class TextBlock
                                //================

    using FontSizeGrp = std::optional<uint16_t>;

    enum class Align { eINVALID, eLEFT, eCENTER, eRIGHT };

    // PUBLIC CONSTANTS

    struct TextString {
        TextId              d_id;
        String_t            d_string;
        Align               d_alignment;
        FontSizeGrp         d_fontSizeGrp;

        TextString(TextId       id        = kNOID,
                   FontSizeGrp  group     = FontSizeGrp(),
                   Align        alignment = Align::eINVALID)
            : d_id(id)
            , d_string()
            , d_alignment(alignment)
            , d_fontSizeGrp(group) { }

        TextString(TextId id, Align alignment)
            : d_id(id)
            , d_string()
            , d_alignment(alignment)
            , d_fontSizeGrp() { }

        TextString(String_t     string,
                   FontSizeGrp  group     = FontSizeGrp(),
                   Align        alignment = Align::eINVALID)
            : d_id()
            , d_string(std::move(string))
            , d_alignment(alignment)
            , d_fontSizeGrp(group) { }

        TextString(String_t string, Align alignment)
            : d_id()
            , d_string(std::move(string))
            , d_alignment(alignment)
            , d_fontSizeGrp() { }

        TextString(FontSizeGrp group, Align alignment = Align::eINVALID)
            : d_id()
            , d_string()
            , d_alignment(alignment)
            , d_fontSizeGrp(group) { }

        TextString&& defaultAlignment(Align alignment) && {
            if (d_alignment == Align::eINVALID) {
                d_alignment = alignment;
            }
            return std::move(*this);
        }
    };

    class TextBlock {
        friend class Widget;
        friend class Wawt;
        friend class List;

        Dimensions          d_metrics       {};
        TextString          d_block         {};
        bool                d_needRefresh   = false;

      public:
        TextBlock()                         = default; 

        TextBlock(const TextString& value) : d_metrics(), d_block(value) { }

        Align& alignment() {
            return d_block.d_alignment;
        }

        FontSizeGrp& fontSizeGrp() {
            return d_block.d_fontSizeGrp;
        }

        void initTextMetricValues(DrawDirective      *args,
                                  DrawProtocol       *adapter,
                                  uint16_t            upperLimit = 0);

        void setText(TextId id);

        void setText(String_t string);

        void setText(const Wawt::StringMapper& mappingFn);

        void setText(const TextString& value) {
            d_block       = value;
            d_needRefresh = true;
        }

        Align alignment() const {
            return d_block.d_alignment;
        }

        FontSizeGrp fontSizeGrp() const {
            return d_block.d_fontSizeGrp;
        }

        const String_t& getText() const {
            return d_block.d_string;
        }

        const Dimensions& metrics() const {
            return d_metrics;
        }

        bool needRefresh() const {
            return d_needRefresh;
        }
    };


inline
constexpr FontSizeGrp operator "" _F(unsigned long long int n) {
    return FontSizeGrp(n);
}


} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
