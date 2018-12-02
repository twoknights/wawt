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
  public:
    // Return 'true' if focus is to be retained.
    // PUBLIC TYPES
    using Initializer   = ScrolledList::Initializer;
    using Items         = ScrolledList::Items;
    using ItemIter      = ScrolledList::ItemIter;
    using OnItemClick   = std::function<void(DropDownList*, ItemIter)>;
    using OptionalRow   = ScrolledList::OptionalRow;

    // PUBLIC CONSTRUCTORS
    //! 'height' uses the base screen's "y-axis"
    DropDownList(double              maxHeight,
                 uint16_t            minCharactersToShow,
                 bool                scrollbarsOnLeft = false)        noexcept;

    DropDownList(double              maxHeight,
                 Initializer         items,
                 bool                scrollbarsOnLeft = false)        noexcept;

    // PUBLIC MANIPULATORS
    void            clear()                                           noexcept{
        d_selectedRow.reset();
    }

    DropDownList&   onItemClick(const OnItemClick& callback)          noexcept{
        d_clickCb = callback;
        return *this;
    }

    Items&          rows()                                            noexcept{
        return d_list.rows();
    }

    Widget          widget()                                          noexcept;

    // PUBLIC ACCESSORS
    const Items&    rows()                                      const noexcept{
        return d_list.rows();
    }

    OptionalRow     selectedRow()                               const noexcept{
        return d_selectedRow;
    }

  private:
    // PRIVATE MANIPULATORS
    void            draw(Widget *widget, DrawProtocol *adapter)       noexcept;

    void            popUpDropDown(Widget *dropDown);

    void            serialize(std::ostream&  os,
                              std::string   *closeTag,
                              const Widget&  entry,
                              unsigned int   indent)                  noexcept;

    // PRIVATE DATA MEMBERS
    OnItemClick         d_clickCb{};

    ScrolledList        d_list;
    double              d_maxHeight;
    OptionalRow         d_selectedRow{};
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
