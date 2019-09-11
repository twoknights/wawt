/** @file ipcmessage.h
 *  @brief IPC message utilities
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

#ifndef WAWT_IPCMESSAGE_H
#define WAWT_IPCMESSAGE_H

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace Wawt {

                                //=================
                                // class IpcMessage
                                //=================

struct IpcMessage {
    // PUBLIC TYPES
    using MessageNumber = uint32_t;

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
    IpcMessage(uint16_t                     size,
               uint16_t                     offset = 0u);
    IpcMessage(const std::string_view& data);
    IpcMessage(const char* data, uint16_t length);
    IpcMessage(const IpcMessage& copy);

    // PUBLIC MANIPULATORS
    IpcMessage& operator=(IpcMessage&&) = default;
    IpcMessage& operator=(const IpcMessage& rhs);

    IpcMessage& operator+=(std::size_t bytes)     noexcept {
        d_offset += bytes;
        return *this;
    }

    void        reset()                           noexcept {
        d_data.reset();
        d_size = d_offset = 0;
    }

    char  *data()                                 noexcept {
        return d_data.get() + d_offset;
    }

    // PUBLIC ACCESSORS
    uint16_t        capacity()              const noexcept {
        return d_size;
    }

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
IpcMessage::IpcMessage(uint16_t size, uint16_t offset)
: d_data(std::make_unique<char[]>(size))
, d_size(size)
, d_offset(offset)
{
}

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

                           //======================
                           // struct IpcMessageUtil
                           //======================

struct IpcMessageUtil
{
    // PUBLIC TYPES
    using MessageNumber = IpcMessage::MessageNumber;

    // PUBLIC CONSTANTS
    static constexpr char kSALT    = '\005';

    static constexpr char kSTARTUP = '\146';
    static constexpr char kDIGEST  = '\012';
    static constexpr char kDATA    = '\055';
    static constexpr char kDIGDATA = '\201';
    static constexpr char kCLOSE   = '\303';

    static constexpr int  hdrsz    = int(sizeof(char) + sizeof(uint16_t));
    static constexpr int  numsz    = int(sizeof(uint32_t));
    static constexpr int  longsz   = int(sizeof(uint64_t));
    static constexpr int  saltsz   = hdrsz    + numsz;
    static constexpr int  prefixsz = saltsz   + hdrsz;

    // PUBLIC CLASS METHODS
    static bool extractHdr(char        *type,
                           uint16_t    *size,
                           const char **p,
                           const char  *end) {
        if (end - *p >= hdrsz) {
            auto a = reinterpret_cast<const uint8_t*>(*p);
            *type = a[0];
            *size = (uint16_t(a[1]) << 8) | uint16_t(a[2]);
            *p += hdrsz;
            return true;
        }
        return false;
    }

    static bool extractValue(uint32_t *value, const char **p, const char *end){
        if (end - *p >= numsz) {
            auto a = reinterpret_cast<const uint8_t*>(*p);
            *value = (uint32_t(a[0]) << 24) | (uint32_t(a[1]) << 16)
                   | (uint32_t(a[2]) <<  8) |  uint32_t(a[3]);
            *p += numsz;
            return true;
        }
        return false;
    }

    static bool extractValue(uint64_t *value, const char **p, const char *end){
        if (end - *p >= longsz) {
            auto a = reinterpret_cast<const uint8_t*>(*p);
            *value = (uint64_t(a[0]) << 56) | (uint64_t(a[1]) << 48)
                   | (uint64_t(a[2]) << 40) | (uint64_t(a[3]) << 32)
                   | (uint64_t(a[4]) << 24) | (uint64_t(a[5]) << 16)
                   | (uint64_t(a[6]) <<  8) |  uint64_t(a[7]);
            *p += longsz;
            return true;
        }
        return false;
    }

    static bool extractSalt(uint32_t *salt, const char **p, const char *end);

    //! 'size' is the length of the data following header + 'hdrsz'
    static char *initHeader(char *p, uint16_t size, char type) {
        *p++    =  type;
        *p++    =  char(size >>  8);
        *p++    =  char(size);
        return p;
    }

    static char *initValue(char *p, uint32_t value) {
        *p++    = char(value  >> 24);
        *p++    = char(value  >> 16);
        *p++    = char(value  >>  8);
        *p++    = char(value);
        return p;
    }
 
    static char *initValue(char *p, uint64_t value) {
        p    = initValue(p, uint32_t(value >> 32));
        return initValue(p, uint32_t(value));
    }

    //! 'size' is the length of the data that follows the prefix.
    static char *initPrefix(char *p, uint32_t salt, uint16_t size, char type);

    template<typename... Args>
    static
    IpcMessage      formatMessage(const char *format, Args&&... args);

    template<typename... Args>
    static
    bool            parseMessage(const IpcMessage&  message,
                                 const char        *format,
                                 Args&&...          args);

    static 
    MessageNumber   messageNumber(const IpcMessage& message);

    static
    bool            verifyDigestPair(const IpcMessage&    digest,
                                     const IpcMessage&    digestMessage);
};

template<typename... Args>
IpcMessage
IpcMessageUtil::formatMessage(const char *format, Args&&... args)
{
    constexpr int NUMARGS = sizeof...(Args);

    if constexpr(NUMARGS == 0) {
        auto size   = std::strlen(format) + 1;
        auto buffer = std::make_unique<char[]>(size);
        std::memcpy(buffer.get(), format, size);
        return {std::move(buffer), size, 0};
    }
    else {
        auto size   = std::snprintf(nullptr, 0, format,
                                             std::forward<Args>(args)...) + 1;
        auto buffer = std::make_unique<char[]>(size);
        std::snprintf(buffer.get(), size, format, std::forward<Args>(args)...);
        return {std::move(buffer), size, 0};
    }
}

template<typename... Args>
inline bool
IpcMessageUtil::parseMessage(const IpcMessage&        message,
                             const char              *format,
                             Args&&...                args)
{
    // A received message always consists of one message fragment.
    // It is expected to also have a EOS at the end (see 'formatMessage').
    constexpr int NUMARGS = sizeof...(Args);

    if (!message.empty()) {
        if constexpr(NUMARGS == 0) {
            return !std::strncmp(message.cbegin(), format, message.length());
        }
        else {
            return NUMARGS == std::sscanf(message.cbegin(),
                                          format,
                                          std::forward<Args>(args)...);
        }
    }
    return false;
}

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
