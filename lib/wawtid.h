/** @file wawtid.h
 *  @brief Provides definition of WAWT related types.
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

#ifndef BDS_WAWTID_H
#define BDS_WAWTID_H

#include <any>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace BDS {

                                    //==============
                                    // class WawtId
                                    //==============

/**
 * @brief Utilities for creating classes to hold IDs and attributes.
 *
 * This class allows composition of a "value" of integer type ('IntId') with
 * any number of "attributes" (also integer types, but typically boolean
 * flags) such that the resulting ID provides equivalence opreations, and 
 * and ordering based on the "value".
 *
 * Values used to initialize IDs do not need to be of the same type as the
 * ID's value underlying type. Setting a value results in a cast to the
 * underlying type.
 */
class WawtId {
  public:

    template<typename IntType>
    class IntId {
      protected:
        IntType d_id;

      public:
        using value_type = IntType;

        constexpr IntId() : d_id() { }

        template<typename OtherInt>
        constexpr IntId(OtherInt value) : d_id(static_cast<IntType>(value)) { }

        constexpr bool operator==(const IntId& rhs) const {
            return d_id == rhs.d_id;
        }

        constexpr bool operator!=(const IntId& rhs) const {
            return !(*this == rhs);
        }

        constexpr bool operator<(const IntId& rhs) const {
            return d_id < rhs.d_id;
        }

        constexpr bool operator>(const IntId& rhs) const {
            return d_id > rhs.d_id;
        }
        
        constexpr IntType value() const {
            return d_id;
        }
    };

    template<typename IntType, typename Name>
    class Mixin {
      protected:
        IntType d_value;

      public:
        constexpr Mixin() : d_value() { }

        constexpr Mixin(bool value) : d_value(value) { }

        constexpr bool operator==(const Mixin& rhs) const {
            return d_value == rhs.d_value;
        }

        constexpr bool operator!=(const Mixin& rhs) const {
            return !(*this == rhs);
        }
    };

    template<typename IdType, typename... Mixins>
    class Id : public Mixins... { // parameter pack
        IdType d_id;
      public:
        constexpr Id() : Mixins()..., d_id() { }

        template<typename OtherType>
        constexpr Id(OtherType id, Mixins... args)
            : Mixins(args)..., d_id(id) { }

        constexpr Id(const Id& copy) : Mixins(copy)..., d_id(copy.d_id) { }

        constexpr Id(Id&& copy) noexcept
            : Mixins(std::move(copy))..., d_id(copy.d_id) { }

        constexpr Id& operator=(const Id& rhs) {
            (Mixins::operator=(rhs), ...);
            d_id = rhs.d_id;
            return *this;
        }

        constexpr Id& operator=(Id&& rhs) noexcept {
            (Mixins::operator=(std::move(rhs)), ...);
            d_id = rhs.d_id;
            return *this;
        }

        constexpr bool operator==(const Id& rhs) const { // fold expression
            return d_id == rhs.d_id && (... && Mixins::operator==(rhs));
        }

        constexpr bool operator!=(const Id& rhs) const {
            return !(*this == rhs);
        }

        constexpr bool operator<(const Id& rhs) const {
            return d_id < rhs.d_id;
        }

        constexpr bool operator>(const Id& rhs) const {
            return d_id > rhs.d_id;
        }
        
        constexpr typename IdType::value_type value() const {
            return d_id.value();
        }
        
        constexpr operator int() const {
            return d_id.value();
        }
    };

    class IsSet : public Mixin<bool, IsSet> {
      public:
        constexpr IsSet()       : Mixin(false) { }
        constexpr IsSet(bool f) : Mixin(f) { }

        constexpr bool isSet() const {
            return d_value;
        }
    };

    class IsRelative : public Mixin<bool, IsRelative> {
      public:
        constexpr IsRelative()          : Mixin(false) { }
        constexpr IsRelative(bool flag) : Mixin(flag) { }

        constexpr bool isRelative() const {
            return d_value;
        }
    };
};

} // end BDS namespace

#endif

// vim: ts=4:sw=4:et:ai
