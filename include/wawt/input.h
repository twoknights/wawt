/** @file input.h
 *  @brief Handle user interface generated input events.
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

#ifndef WAWT_EVENT_H
#define WAWT_EVENT_H

#include "wawt/wawt.h"

#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace Wawt {

class Widget;
class List;
class Text;

                                //===================
                                // class InputHandler  
                                //===================

    using EnterFn     = std::function<bool(String_t*, bool)>;

    using OnClickCb   = std::function<FocusCb(bool, int x, int y, Widget*)>;

    using GroupCb     = std::function<FocusCb(List*, uint16_t)>;

    using SelectFn    = std::function<FocusCb(Text*)>;

    enum class ActionType { eINVALID, eCLICK, eTOGGLE, eBULLET, eENTRY };

    class InputHandler {
        friend class Widget;
        friend class Wawt;
        friend class List;

        EventUpCb callOnClickCb(int               x,
                                int               y,
                                Widget             *base,
                                const OnClickCb&  cb,
                                bool              callOnDown);

        EventUpCb callSelectFn(Text            *text,
                               const SelectFn&  cb,
                               bool             callOnDown);

        EventUpCb callSelectFn(Widget            *base,
                               const SelectFn&  cb,
                               bool             callOnDown);

        bool                d_disabled;

      public:
        using Callback = std::variant<std::monostate,
                                      OnClickCb,
                                      SelectFn,
                                      std::pair<EnterFn,uint16_t>,
                                      std::pair<SelectFn,bool>,
                                      std::pair<OnClickCb,bool>>;

        ActionType          d_action;
        Callback            d_callback;

        template<class OnClick = Callback,
                 typename = std::enable_if_t<std::is_convertible_v<OnClick,
                                                                  Callback>>>
        InputHandler(OnClick    cb     = Callback(),
                     ActionType action = ActionType::eINVALID)
            : d_disabled(action == ActionType::eINVALID)
            , d_action(action)
            , d_callback(std::move(cb)) { }

        InputHandler&& defaultAction(ActionType type) && {
            if (d_action == ActionType::eINVALID) {
                d_action   = type;
                d_disabled = d_action == ActionType::eINVALID;
            }
            return std::move(*this);
        }

        void defaultAction(ActionType type) & {
            if (d_callback.index() > 0 && d_action == ActionType::eINVALID) {
                d_action   = type;
                d_disabled = d_action == ActionType::eINVALID;
            }
        }

        Callback& callback() {
            return d_callback;
        }

        ActionType& action() {
            return d_action;
        }

        FocusCb callSelectFn(Text* text);

        EventUpCb downEvent(int x, int y, Widget*  base);

        EventUpCb downEvent(int x, int y, Text* text);

        bool contains(int x, int y, const Widget* base) const;

        bool disabled() const {
            return d_disabled;
        }

        bool textcontains(int x, int y, const Text *text) const;
    };

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
