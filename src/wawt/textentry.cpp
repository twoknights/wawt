/** @file textentry.cpp
 *  @brief Factory for text entry widgets and associated support.
 *
 * Copyright 2018 Bruce Szablak

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

#include "wawt/drawprotocol.h"
#include "wawt/textentry.h"
#include "wawt/wawtenv.h"

#include <algorithm>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#ifdef WAWT_WIDECHAR
#define S(str) (L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) (u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace Wawt {

namespace {

} // unnamed namespace

                            //----------------
                            // class TextEntry
                            //----------------


// PRIVATE METHODS
void
TextEntry::update(Widget *widget, Trackee *label) noexcept
{
    if (d_label == nullptr && widget) {
        widget->optionName(WawtEnv::sEntry);

        widget->downEventMethod(
                    [](double, double, Widget *me, Widget*) {
                        auto cb = EventUpCb();
                        if (me->tracker()) {
                            cb= [me](double x, double y, bool up) -> void {
                                    if (up && me->inside(x, y)
                                           && me->tracker()) {
                                        me->focus(me);
                                    }
                                };
                        }
                        return cb;
                    })
               .drawMethod(
                    [](Widget *me, DrawProtocol *adapter) {
                        auto *entry = static_cast<TextEntry*>(me->tracker());
                        if (entry) {
                            entry->draw(me, adapter);
                        }
                    })
               .inputMethod(
                    [](Widget *me, Char_t input) {
                        auto *entry = static_cast<TextEntry*>(me->tracker());
                        if (entry) {
                            return entry->input(me, input);
                        }
                        return false;
                    })
               .serializeMethod(
                    [](std::ostream& os, std::string *closeTag,
                       const Widget& me, unsigned int indent) {
                        auto *entry = static_cast<TextEntry*>(me.tracker());
                        if (entry) {
                            entry->serialize(os, closeTag, me, indent);
                        }
                    });
    }
    Tracker::update(widget, label);
    return;                                                           // RETURN
}

void
TextEntry::draw(Widget *widget, DrawProtocol *adapter) noexcept
{
    auto  label  = entry();
    auto& box    = widget->layoutData();
    auto  text   = widget->text().d_data;
    auto& layout = widget->text().d_layout;

    adapter->draw(box, widget->settings());

    if (d_focus && d_bufferLng < d_maxInputCharacters) {
        label.append(d_cursor);
    }
    text.d_view   = label;

    if (!label.empty()) {
        if (!text.resolveSizes(box,
                               layout.upperLimit(box),
                               adapter,
                               widget->settings().d_options)) {
            return;                                               // RETURN
        }
    }
    text.d_upperLeft = layout.position(text.d_bounds, box);

    adapter->draw(text, widget->settings());
}

bool
TextEntry::input(Widget *widget, Char_t input) noexcept
{
    if (input == WawtEnv::kFocusChg) {
        d_focus = !d_focus;
        widget->selected(d_focus);

        if (!d_focus && d_endCb) {
            d_endCb(this, '\0');
        }
    }
    else if (input == d_backspace) {
        if (d_bufferLng > 0) {
            d_bufferLng -= 1;
        }
    }
    else if (d_endChars.end()
                   != std::find(d_endChars.begin(), d_endChars.end(), input)) {
        if (!d_endCb || !d_endCb(this, input)) {
            widget->selected(d_focus = false);
        }
    }
    else if (!d_verifierCb || d_verifierCb(this, input)) {
        if (d_maxInputCharacters > d_bufferLng) {
            d_buffer[d_bufferLng++] = input;

            if (d_maxInputCharacters == d_bufferLng
             && d_autoEnter && (d_endCb && !d_endCb(this, d_enter))) {
                widget->selected(d_focus = false);
            }
        }
    }
    return d_focus;                                                   // RETURN
}

void
TextEntry::serialize(std::ostream&  os,
                     std::string   *closeTag,
                     const Widget&  entry,
                     unsigned int   indent) noexcept
{
    Widget::defaultSerialize(os, closeTag, entry, indent);
    std::string spaces(indent+2, ' ');
    os << spaces
       << "<maxInputCharacters count='" << d_maxInputCharacters << "'/>\n" 
       << spaces
       << "<chars cursor='";
    outputXMLescapedString(os, d_cursor)    << "' backspace='";
    outputXMLescapedChar(os, d_backspace) << "' enter='";
    outputXMLescapedChar(os, d_backspace) << "'>";

    for (auto ch : d_endChars) {
        outputXMLescapedChar(os, ch);
    }
    os << "</chars>\n";
    return;                                                           // RETURN
}

// PUBLIC METHODS
TextEntry::TextEntry(uint16_t           maxInputCharacters,
                     const EndCb&       endCb,
                     Char_t             cursor,
                     Char_t             backspace,
                     Char_t             enter) noexcept
: d_maxInputCharacters(maxInputCharacters)
, d_endCb(endCb)
, d_cursor(toString(&cursor,1))
, d_backspace(backspace)
, d_enter(enter)
, d_endChars(1, enter)
, d_layoutString(String_t(maxInputCharacters-2, C('X')) + S("g"))
{
    d_buffer = std::make_unique<Char_t[]>(maxInputCharacters);
}

TextEntry::TextEntry(uint16_t           maxInputCharacters,
                     const EndCb&       endCb,
                     EndCharList        endList,
                     Char_t             cursor,
                     Char_t             backspace,
                     Char_t             enter) noexcept
: d_maxInputCharacters(maxInputCharacters)
, d_endCb(endCb)
, d_cursor(toString(&cursor,1))
, d_backspace(backspace)
, d_enter(enter)
, d_endChars(endList.begin(), endList.end())
, d_layoutString(String_t(maxInputCharacters-1, C('X')) + S("g"))
{
    d_endChars.push_back(enter);
    d_buffer = std::make_unique<Char_t[]>(maxInputCharacters);
}

bool
TextEntry::entry(StringView_t text) noexcept
{
    auto work = std::make_unique<Char_t[]>(d_maxInputCharacters);
    auto lng  = 0u;

    while (lng < d_maxInputCharacters && !text.empty()) {
        auto next = popFrontChar(text);

        if (next == '\0' || (d_verifierCb && !d_verifierCb(this, next))) {
            return false;                                             // RETURN
        }
        work[lng++] = next;
    }

    if (!text.empty()) {
        return false;                                                 // RETURN
    }
    d_bufferLng = lng;
    d_buffer    = std::move(work);
    return true;                                                      // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
