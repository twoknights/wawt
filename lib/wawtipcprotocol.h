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
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

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
    enum class ConnectionStatus { eOK, eDISCONNECT, eCLOSED, eCANCEL, eERROR };

    using ConnectionId     = int;

    using Address          = std::any;

    using Message          = std::pair<std::unique_ptr<char[]>,uint16_t>;

    using ConnectCallback  = std::function<void(ConnectionId,
                                                ConnectionStatus)>;

    using MessageCallback  = std::function<void(ConnectionId, Message&&)>;

    //! Asynchronous close of connection.
    virtual bool closeConnection(ConnectionId id)                           =0;

    //! Synchronous call to form a connection address from "instructions".
    virtual AddressStatus  makeAddress(Address             *address,
                                       std::string         *errorMessage,
                                       std::any             directions)     =0;

    //! Asynchronous call for "mutual" message exchange.
    virtual bool sendMessage(ConnectionId               id,
                             Message&&                  message)            =0;

    //! Asynchronous establishment of a connection.
    virtual ConnectionId establishConnection(
                                     std::string           *diagnostic,
                                     ConnectCallback        connectionUpdate,
                                     MessageCallback        receivedMessage,
                                     const Address&         address)        =0;

};

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
                            
} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
