/** @file dropdownlist.h
 *  @brief Implements drop-down list widget factory
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

#ifndef WAWT_DROPDOWNLIST_H
#define WAWT_DROPDOWNLIST_H

#include "wawt/widget.h"
#include "wawt/scrolledlist.h"

#include <ostream>
#include <string>

namespace Wawt {

                            //===================
                            // class DropDownList
                            //===================


class DropDownList : public Tracker {

    // PRIVATE MANIPULATORS
    void            update(Widget *widget, Trackee *label)   noexcept override;

    //! Return 'true' if "focus" is retained; 'false' if it is lost.
    void            draw(Widget *widget, DrawProtocol *adapter)       noexcept;

    void            serialize(std::ostream&  os,
                              std::string   *closeTag,
                              const Widget&  entry,
                              unsigned int   indent)                  noexcept;

  public:
    // Return 'true' if focus is to be retained.
    // PUBLIC TYPES
    using Items = std::vector<String_t>;

    // PUBLIC CONSTRUCTORS
    DropDownList(uint16_t       maxRowCharacters,
                 double         maxDropDownHeight,
                 bool           scrollbarsOnLeft     = false,
                 bool           alwaysShowScrollbars = false)         noexcept;

    // PUBLIC MANIPULATORS
    void            clear()                                           noexcept;

    DropDownList&   itemOptions(std::any&& options)                   noexcept{
        d_options = std::move(options);
        return *this;
    }

    Items&          items()                                           noexcept{
        return d_items;
    }

    bool            setSelection(const String_t& selection)           noexcept;

    // PUBLIC ACCESSORS
    const std::any& itemOptions()                               const noexcept{
        return d_scrolledList.itemOptions();
    }

    const String_t& layoutString()                              const noexcept{
        return d_layoutString;
    }

    const Items&    rows()                                      const noexcept{
        return d_scrolledList.rows();
    }

  private:
    String_t    d_layoutString;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
