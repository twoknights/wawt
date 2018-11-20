/** @file scrolledlist.h
 *  @brief Implements a list that handles variable number of elements.
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

#ifndef WAWT_SCROLLEDLIST_H
#define WAWT_SCROLLEDLIST_H

#include "wawt/widget.h"

#include <any>
#include <cstdint>
#include <list>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Wawt {

                            //===================
                            // class ScrolledList
                            //===================


class ScrolledList : public Tracker {
  public:
    // Return 'true' if focus is to be retained.
    // PUBLIC TYPES
    using Item          = std::pair<String_t, bool>;
    using Items         = std::list<Item>;
    using ItemIter      = Items::iterator;
    using OnItemClick   = std::function<void(ItemIter)>;
    using OptionalRow   = std::optional<ItemIter>;

    // PUBLIC CONSTRUCTORS
    ScrolledList(uint16_t       minCharactersToShow,
                 TextAlign      alignment            = TextAlign::eCENTER,
                 bool           scrollbarsOnLeft     = false,
                 bool           alwaysShowScrollbars = false)         noexcept;

    ScrolledList(std::initializer_list<Item>
                                items,
                 TextAlign      alignment            = TextAlign::eCENTER,
                 bool           scrollbarsOnLeft     = false,
                 bool           alwaysShowScrollbars = false)         noexcept;

    // PUBLIC MANIPULATORS
    void            clear()                                           noexcept;

    void            clearSelection()                                  noexcept;

    ScrolledList&   itemOptions(std::any&& options)                   noexcept{
        d_itemOptions = std::move(options);
        return *this;
    }

    // Return an 'WawtEnv::sList' container of the list widgets.
    Widget          widget()                                          noexcept;

    ScrolledList&   onItemClick(const OnItemClick& callback)          noexcept{
        d_clickCb = callback;
        return *this;
    }

    Items&          rows()                                            noexcept{
        return d_rows;
    }

    ScrolledList&   singleSelectList(bool value)                      noexcept;

    void            synchronizeView(DrawProtocol *adapter)            noexcept;

    //! Row at top of viewed area.
    void            top(ItemIter topItem)                             noexcept;

    // PUBLIC ACCESSORS
    const std::any& itemOptions()                               const noexcept{
        return d_itemOptions;
    }

    OptionalRow     lastRowClicked()                            const noexcept{
        return d_lastRowClicked; 
    }

    const Items&    rows()                                      const noexcept{
        return d_rows;
    }

    bool            singleSelectList()                          const noexcept{
        return d_singleSelect;
    }

    uint16_t        selectCount()                               const noexcept{
        return d_selectCount;
    }

    //! Row at top of viewed area.
    ItemIter        top()                                       const noexcept;

    //! Number of rows being shown in viewed area.
    std::size_t     viewedRows()                                const noexcept{
        return d_windowView.size();
    }

    //! Number of rows that can be shown in viewed area.
    std::size_t     viewSize()                                  const noexcept{
        return d_windowSize;
    }

  private:
    // PRIVATE TYPES
    using RowInfo = std::tuple<StringView_t, bool, float>;

    // PRIVATE MANIPULATORS
    void            draw(Widget *widget, DrawProtocol *adapter)       noexcept;

    void            scroll(int delta)                                 noexcept;

    void            upEvent(double x, double y, Widget *widget)       noexcept;

    Widget::DownEventMethod makeScroll(bool down)                     noexcept;

    Widget::DownEventMethod makePageScroll(bool down)                 noexcept;

    // PRIVATE DATA
    Items                   d_rows{};
    std::set<std::size_t>   d_selectedSet{};
    std::vector<RowInfo>    d_windowView{};
    OnItemClick             d_clickCb{};
    OptionalRow             d_lastRowClicked{};
    ItemIter                d_top               = d_rows.begin();
    int                     d_topPos            = 0;
    std::size_t             d_selectCount       = 0;
    float                   d_rowSize           = 0;
    bool                    d_singleSelect      = false;
    std::size_t             d_windowSize        = 0;

    std::string             d_layoutString;
    TextAlign               d_alignment;
    bool                    d_scrollbarsOnLeft;
    bool                    d_alwaysShowScrollbars;
    std::any                d_itemOptions;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
