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

#include "wawt/textentry.h"
#include "wawt/wawtenv.h"

#ifdef WAWT_WIDECHAR
#define S(str) String_t(L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace Wawt {

namespace {

} // unnamed namespace

                            //----------------
                            // class TextEntry
                            //----------------


TextEntry::TextEntry(int                maxInputCharacters,
                     const EnterCb&     enterCb,
                     Char_t             cursor,
                     Char_t             backspace,
                     Char_t             enter)
: d_maxInputCharacters(maxInputCharacters)
, d_enterCb(enterCb)
, d_cursor(cursor)
, d_backspace(backspace)
, d_enter(enter)
{
    d_buffer = std::make_unique<Char_t[]>(maxInputCharacters);
}

Widget
TextEntry::widget(const Layout&      layout,
                  StringView_t       string,
                  CharSizeGroup      group,
                  TextAlign          alignment);
{
    return Widget(WawtEnv::sEntry, Trackee(*this), layout)
            .text(string, group, alignment)
            .method(
                [](double x, double y, Widget *widget, Widget *parent) {
                    if (widget->tracker()) {
                        return
                            [widget](double x, double y, bool up) -> void {
                                if (up
                                 && widget->inside(x, y)
                                 && widget->tracker()) {
                                    widget->setFocus(widget);
                                }
                            };
                    }
                    return EventUpCb();
                })
            .method(
                [](Widget *widget, DrawProtocol *adapter) {
                    auto *entry = static_cast<TextEntry*>(widget->tracker());
                    if (entry) {
                        entry->draw(widget, adapter);
                    }
                })
            .method(
                [](Widget *widget, Char_t input) {
                    auto *entry = static_cast<TextEntry*>(widget->tracker());
                    if (entry) {
                        return entry->input(widget, input);
                    }
                    return false;
                })
            .method(
                [](Widget *widget, const Widget& parent, const Widget& root,
                   bool    firstPass, DrawProtocol *adapter) {
                    auto *entry = static_cast<TextEntry*>(widget->tracker());
                    if (entry) {
                       entry->layout(widget, parent, root, firstPass, adapter);
                    }
                })
            .method(
                [](std::ostream& os, std::string *closeTag,
                   const Widget& widget, unsigned int indent) {
                    auto *entry = static_cast<TextEntry*>(widget->tracker());
                    if (entry) {
                        entry->serialize(os, closeTag, widget, indent);
                    }
                });                                                   // RETURN
}

void
TextEntry::draw(Widget *widget, DrawProtocol *adapter)
{
}

bool
TextEntry::entry(StringView_t text)
{
    auto work = std::make_unique<Char_t[]>(d_maxInputCharacters);
    auto lng  = 0u;

    while (lng < d_maxInputCharacters && !text.empty()) {
        auto next = popFrontChar(text);

        if (next == '\0' || (d_verifyCb && !d_verifyCb(next))) {
            return false;                                             // RETURN
        }
        work[lng++] = next;
    }

    if (!text.empty()) {
        return false;                                                 // RETURN
    }
    d_bufferLng = lng;
    d_buffer    = std::move(work);
    d_entry     = toString(d_buffer.get(), lng);
    return true;                                                      // RETURN
}

bool
TextEntry::input(Widget *widget, Char_t input)
{
    if (input == kFocusChg) {
        d_focus = !d_focus;
    }
    else if (!d_verifierCb || d_verifierCb(this, input)) {
        if (!d_focus) {
            assert(!"Stray input character seen.");
            return false;                                             // RETURN
        }

        if (input == d_backspace) {
            if (d_bufferLng > 1) {
                d_bufferLng -= 1;
            }
        }
        else if (input == d_enter) {
            if (d_enterCb) {
                d_focus = d_entryCb(this);
            }
            else {
                d_focus = false;
            }
        }
        else if (d_maxInputCharacters > d_bufferLng) {
            d_buffer[d_bufferLng++] = input;
        }
        d_entry = toString(d_buffer.get(), d_bufferLng);
    }
    return d_focus;                                                   // RETURN
}

void
TextEntry::layout(Widget                  *widget,
                  const Widget&            parent,
                  const Widget&            root,
                  bool                     firstPass,
                  DrawProtocol            *adapter)
{
    Widget::defaultLayout(widget, parent, root, firstPass, adapter);
}

void
TextEntry::serialize(std::ostream&      os,
                     std::string       *closeTag,
                     const Widget&      widget,
                     unsigned int       indent)
{
}

};
}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
