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

    // PRIVATE MANIPULATORS
    void            update(Widget *widget, Trackee *label) override;

    //! Return 'true' if "focus" is retained; 'false' if it is lost.
    bool            input(Widget *widget, Char_t input);

    void            draw(Widget *widget, DrawProtocol *adapter);

  public:
    // Return 'true' if focus is to be retained.
    // PUBLIC TYPES
    using EndCb         = std::function<bool(TextEntry *, Char_t)>;
    using EndCharList   = std::initializer_list<Char_t>;
    using VerifierCb    = std::function<bool(TextEntry*,Char_t)>;

    // PUBLIC CONSTRUCTORS
    TextEntry(uint16_t          maxInputCharacters,
              const EndCb&      endCb       = EndCb(),
              Char_t            cursor      = '|',
              Char_t            backspace   = '\b',
              Char_t            enter       = '\r');

    TextEntry(uint16_t          maxInputCharacters,
              const EndCb&      endCb,
              EndCharList       endList,
              Char_t            cursor      = '|',
              Char_t            backspace   = '\b',
              Char_t            enter       = '\r');

    // PUBLIC MANIPULATORS
    //! Return 'true' if 'text' fits within maximum character requirements.
    bool                entry(StringView_t text);

    void                inputVerifier(const VerifierCb& verify) {
        d_verifierCb = verify;
    }

    // PUBLIC ACCESSORS
    String_t            entry() const {
        return toString(d_buffer.get(), d_bufferLng);
    }

    Char_t              enterChar() const {
        return d_enter;
    }

    bool                focus() const {
        return d_focus;
    }

    const VerifierCb&   inputVerifier() const {
        return d_verifierCb;
    }

    const String_t&     layoutString() const {
        return d_string;
    }

  private:
    using Buffer     = std::unique_ptr<Char_t[]>;
    using EndChars   = std::vector<Char_t>;

    VerifierCb  d_verifierCb{};
    Buffer      d_buffer{};
    uint16_t    d_bufferLng     = 0u;
    bool        d_focus         = false;
    uint16_t    d_maxInputCharacters;
    EndCb       d_endCb;
    String_t    d_cursor;
    Char_t      d_backspace;
    Char_t      d_enter;
    EndChars    d_endChars;
    String_t    d_string;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
