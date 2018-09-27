/** @file wawtipcprotocol.h
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

#ifndef BDS_WAWTIPCPROTOCOL_H
#define BDS_WAWTIPCPROTOCOL_H

#include <any>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "wawt.h"

namespace BDS {

                            //======================
                            // class WawtIpcProtocol
                            //======================

/**
 * @brief An interface whose providers support communication between tasks.
 */

class WawtIpcProtocol {
  public:
    // PUBLIC TYPES
    enum class AddressStatus    { eOK, eMALFORMED, eINVALID, eUNKNOWN };
    enum class ConnectionStatus { eOK, eDISCONNECT, eCLOSED, eERROR };

    using ConnectionId     = uint32_t;

    using Message          = std::pair<std::unique_ptr<char[]>,uint16_t>;

    using ConnectCb        = std::function<void(ConnectionId,
                                                ConnectionStatus)>;

    using MessageCb        = std::function<void(ConnectionId, Message&&)>;

    constexpr static ConnectionId kINVALID_ID = UINT32_MAX;

    //! Asynchronous close of connection.
    virtual void            closeConnection(ConnectionId    id)             =0;

    //! Synchronous call to associate a "connection" to an "address".
    virtual AddressStatus   prepareConnection(Wawt::String_t *diagnostic,
                                              ConnectionId   *id,
                                              ConnectCb       connectionUpdate,
                                              MessageCb       receivedMessage,
                                              std::any        address)      =0;

    //! Asynchronous call to send a message on a connection
    virtual bool            sendMessage(ConnectionId        id,
                                        Message&&           message)        =0;

    // Asynchronous close of all connections. No new ones permitted.
    virtual void            closeAll()                                      =0;

    //! Asynchronous establishment of a connection.
    virtual bool            startConnection(Wawt::String_t *diagnostic,
                                            ConnectionId    connectionId)   =0;

};

#if 0
class ThreadAdapter : public WawtIpcProtocol {
  public:
    // PUBLIC TYPES

    // DELETED
    ThreadAdapter(const ThreadAdapter&)            = delete;
    ThreadAdapter& operator=(const ThreadAdapter&) = delete;

    // PUBLIC WawtIpcProtocol INTERFACE
    bool closeConnection(ConnectionId)                                override;

    AddressStatus  makeAddress(Address          *address,
                               std::string      *errorMessage,
                               std::any)                              override;

    bool sendMessage(ConnectionId,                         
                     Message&&                   message)             override;

    ConnectionId establishConnection(std::string           *diagnostic,
                                     ConnectCallback        connectionUpdate,
                                     MessageCallback        receivedMessage,
                                     const Address&         address)  override;

  protected:
    virtual Message messageProcessor(const Message& message)                =0;

  private:
    std::mutex              d_lock{};
    std::thread             d_thread{};
    ConnectCallback         d_connectUpdate{};
    MessageCallback         d_messageCallback{};
    std::deque<Message>     d_processorFifo{};
};
#endif
                            
} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
