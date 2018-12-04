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
#include <cstring>
#include <deque>
#include <forward_list>
#include <functional>
#include <memory>
#include <string>

#include "wawt.h"

namespace Wawt {

struct IpcMessage {
    // PUBLIC DATA MEMBERS
    std::unique_ptr<char[]>     d_data{};
    uint16_t                    d_size      = 0;
    uint16_t                    d_offset    = 0;

    // PUBLIC CONSTRUCTORS
    IpcMessage()                            = default;
    IpcMessage(IpcMessage&&)                = default;

    IpcMessage(std::unique_ptr<char[]>&&    data,
               uint16_t                     size,
               uint16_t                     offset);
    IpcMessage(const std::string_view& data);
    IpcMessage(const char* data, uint16_t length);
    IpcMessage(const IpcMessage& copy);

    // PUBLIC MANIPULATORS
    IpcMessage& operator=(IpcMessage&&) = default;
    IpcMessage& operator=(const IpcMessage& rhs);

    void        reset()                           noexcept {
        d_data.reset();
        d_size = d_offset = 0;
    }

    char  *data()                                 noexcept {
        return d_data.get() + d_offset;
    }

    // PUBLIC ACCESSORS
    const char  *cbegin()                   const noexcept {
        return d_data.get() + d_offset;
    }

    bool         empty()                    const noexcept {
        return length() == 0;
    }

    const char  *cend()                     const noexcept {
        return d_data.get() + d_size;
    }

    uint16_t        length()                const noexcept {
        return d_size - d_offset;
    }

    explicit operator std::string_view()    const noexcept {
        return std::string_view(cbegin(), length());
    }
};

inline
IpcMessage::IpcMessage(std::unique_ptr<char[]>&&    data,
                       uint16_t                     size,
                       uint16_t                     offset)
: d_data(std::move(data))
, d_size(size)
, d_offset(offset)
{
}

inline
IpcMessage::IpcMessage(const std::string_view& data)
: d_data(std::make_unique<char[]>(data.size()))
, d_size(data.size())
{
    data.copy(d_data.get(), d_size);
}

inline
IpcMessage::IpcMessage(const char* data, uint16_t length)
: d_data(std::make_unique<char[]>(length))
, d_size(length)
{
    std::memcpy(d_data.get(), data, length);
}

inline
IpcMessage::IpcMessage(const IpcMessage& copy)
: d_data(std::make_unique<char[]>(copy.d_size))
, d_size(copy.d_size)
, d_offset(copy.d_offset)
{
    std::memcpy(d_data.get(), copy.d_data.get(), d_size);
}

inline
IpcMessage& IpcMessage::operator=(const IpcMessage& rhs)
{
    if (this != &rhs) {
        d_data      = std::make_unique<char[]>(rhs.d_size);
        d_size      = rhs.d_size;
        d_offset    = rhs.d_offset;
        std::memcpy(d_data.get(), rhs.d_data.get(), d_size);
    }
    return *this;                                                     // RETURN
}

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
