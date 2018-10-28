/** @file textentry.h
 *  @brief Implements text entry widget factory
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

#ifndef WAWT_TEXTENTRY_H
#define WAWT_TEXTENTRY_H

#include "wawt/widget.h"

#include <ostream>
#include <string>

namespace Wawt {

                            //================
                            // class TextEntry
                            //================


class TextEntry : public Tracker {
  public:
    // Return 'true' if focus is to be retained.
    using EnterCb       = std::function<bool(TextEntry *)>;
    using VerifierCb    = std::function<bool(TextEntry*,Char_t)>;

    TextEntry(uint16_t          maxInputCharacters,
              const EnterCb&    enterCb,
              Char_t            cursor      = '|',
              Char_t            backspace   = '\b',
              Char_t            enter       = '\r');

    // PUBLIC MANIPULATORS
    void            addInputVerifier(const VerifierCb& verify);

    Widget          widget(const Layout&      layout,
                           StringView_t       prompt,
                           CharSizeGroup      group     = CharSizeGroup(),
                           TextAlign          alignment = TextAlign::eLEFT);

    void            draw(Widget *widget, DrawProtocol *adapter);

    //! Return 'true' if 'text' fits within maximum character requirements.
    bool            entry(StringView_t text);

    //! Return 'true' if "focus" is retained; 'false' if it is lost.
    bool            input(Widget *widget, Char_t input);

    void            layout(Widget                  *widget,
                           const Widget&            parent,
                           const Widget&            root,
                           bool                     firstPass,
                           DrawProtocol            *adapter);

    void            serialize(std::ostream&      os,
                              std::string       *closeTag,
                              const Widget&      widget,
                              unsigned int       indent);

    // PUBLIC ACCESSORS
    const String_t& entry() const {
        return d_entry;
    }

  private:
    using Buffer = std::unique_ptr<Char_t[]>;

    DrawData    d_drawData{WawtEnv::sEntry};
    VerifierCb  d_verifier{};
    Buffer      d_buffer{};
    String_t    d_entry{};
    bool        d_focus         = false;
    uint16_t    d_bufferLng     = 0u;
    uint16_t    d_maxInputCharacters;
    EnterCb     d_enter;
    Char_t      d_cursor;
    Char_t      d_backspace;
    Char_t      d_enter;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
