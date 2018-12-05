/** @file ipcprotocol.h
 *  @brief Abstract interface for inter-process communication.
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

#ifndef WAWT_IPCPROTOCOL_H
#define WAWT_IPCPROTOCOL_H

#include <any>
#include <cstdint>
#include <forward_list>
#include <functional>
#include <memory>
#include <string>

#include "wawt/wawt.h"
#include "wawt/ipcmessage.h"

namespace Wawt {

                                //==================
                                // class IpcProtocol
                                //==================

/**
 * @brief An interface whose providers support communication between tasks.
 */

class IpcProtocol
{
  public:
    // PUBLIC TYPES
    enum class ChannelStatus { ePENDING, eMALFORMED, eINVALID, eUNKNOWN  };
    enum class ChannelChange { eREADY, eDROP, eCLOSE, eERROR, ePROTO };
    enum class ChannelMode   { eCREATOR, eACCEPTOR };

    struct ChannelId {
        uint32_t        d_adapterId  : 8,
                        d_internalId : 24;

        // PUBLIC CONSTRUCTORS
        constexpr ChannelId() : d_adapterId(0xFF), d_internalId(0xFFFFFF) { }

        constexpr ChannelId(int adapter, int internal)
            : d_adapterId(adapter), d_internalId(internal) { }

        // PUBLIC ACCESSORS
        constexpr operator uint32_t()                          const noexcept {
            return (d_adapterId << 8)|d_internalId;
        }

        constexpr bool operator==(const ChannelId& rhs)        const noexcept {
            return uint32_t(*this) == uint32_t(rhs);
        }

        constexpr bool operator!=(const ChannelId& rhs)        const noexcept {
            return uint32_t(*this) != uint32_t(rhs);
        }

        constexpr bool operator<(const ChannelId& rhs)         const noexcept {
            return uint32_t(*this) < uint32_t(rhs);
        }
    };

    struct ChannelIdHash {
        std::size_t operator()(const ChannelId& id)            const noexcept {
            return std::hash<unsigned int>{}(uint32_t(id));
        };
    };

    // PUBLIC CLASS MEMBERS
    static const ChannelId kINVALID_ID;

    using MessageChain      = std::forward_list<IpcMessage>;

    // Calling thread must not hold locks.
    using ChannelCb         = std::function<void(ChannelId,
                                                 ChannelChange,
                                                 ChannelMode)>;

    // Calling thread must not hold locks.
    using MessageCb         = std::function<void(ChannelId, IpcMessage&&)>;

    //! Configure the adapter to accept channels created by peers.
    virtual ChannelStatus   acceptChannels(String_t       *diagnostic,
                                           std::any        configuration)
                                                                    noexcept=0;

    // Asynchronous close of all channels. No new ones permitted.
    virtual void            closeAdapter()                          noexcept=0;

    //! Asynchronous close of a channel.
    virtual void            closeChannel(ChannelId    id)           noexcept=0;

    //! Configure a channel to a peer that is accepting channels.
    virtual ChannelStatus   createNewChannel(String_t       *diagnostic,
                                             std::any        configuration)
                                                                    noexcept=0;

    virtual void            installCallbacks(ChannelCb       channelUpdate,
                                             MessageCb       receivedMessage)
                                                                    noexcept=0;

    //! Asynchronous call to send a message on a channel
    virtual bool            sendMessage(ChannelId           id,
                                        MessageChain&&      chain)  noexcept=0;
};

constexpr IpcProtocol::ChannelId IpcProtocol::kINVALID_ID;

} // end Wawt namespace

#endif
// vim: ts=4:sw=4:et:ai
