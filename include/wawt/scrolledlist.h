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
#include <deque>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace Wawt {

                            //===================
                            // class ScrolledList
                            //===================


class ScrolledList : public Tracker {
  public:
    // Return 'true' if focus is to be retained.
    // PUBLIC TYPES
    using Item          = std::pair<String_t, bool>;
    using Items         = std::deque<Item>;
    using OnItemClick   = std::function<void(uint16_t, const Item&)>;
    using OptionalRow   = std::optional<uint16_t>;

    // PUBLIC CONSTRUCTORS
    ScrolledList(uint16_t       visibleRowCount,
                 TextAlign      alignment            = TextAlign::eCENTER,
                 bool           scrollbarsOnLeft     = false,
                 bool           alwaysShowScrollbars = false)         noexcept;

    // PUBLIC MANIPULATORS
    void            clear()                                           noexcept{
        d_rows.clear();
        d_top = 0;
        synchronizeView();
    }

    void            clearSelection()                                  noexcept;

    ScrolledList&   itemOptions(std::any&& options)                   noexcept{
        d_itemOptions = std::move(options);
        return *this;
    }

    // Return an 'WawtEnv::sList' container of the list widgets.
    Widget          list(const Layout& layout)                        noexcept;

    ScrolledList&   onItemClick(const OnItemClick& callback)          noexcept{
        d_clickCb = callback;
        return *this;
    }

    Items&          rows()                                            noexcept{
        return d_rows;
    }

    ScrolledList&   singleSelectList(bool value)                      noexcept;

    void            synchronizeView(DrawProtocol *adapter = nullptr)  noexcept;

    //! Row at top of viewed area.
    void            top(uint16_t row)                                 noexcept{
        if (row < d_rows.size()) {
            d_top = row;
        }
    }

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
    uint16_t        top()                                       const noexcept{
        return d_top < d_rows.size() ? d_top : 0;
    }

    //! Number of rows being shown in viewed area.
    uint16_t        viewedRows()                                const noexcept{
        if (top() + d_windowSize < d_rows.size()) {
            return d_windowSize;
        }
        return d_rows.size() - top();
    }

    //! Number of rows that can be shown in viewed area.
    unsigned int    viewSize()                                  const noexcept{
        return d_windowSize;
    }

  private:
    // PRIVATE TYPES
    using RowInfo = std::tuple<StringView_t, uint16_t, bool, float>;

    // PRIVATE MANIPULATORS
    void            draw(Widget *widget, DrawProtocol *adapter)       noexcept;

    void            upEvent(double x, double y, Widget *widget)       noexcept;

    // PRIVATE DATA
    Items                   d_rows{};
    OnItemClick             d_clickCb{};
    OptionalRow             d_lastRowClicked{};
    std::set<uint16_t>      d_selectedSet{};
    std::vector<RowInfo>    d_windowView{};
    uint16_t                d_selectCount       = 0;
    float                   d_charSize          = 0;
    uint16_t                d_top               = 0;
    bool                    d_singleSelect      = false;
    uint16_t                d_windowSize;
    TextAlign               d_alignment;
    bool                    d_scrollbarsOnLeft;
    bool                    d_alwaysShowScrollbars;
    std::any                d_itemOptions;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
