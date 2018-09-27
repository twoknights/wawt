/** @file wawtipcprotocol.cpp
 *  @brief Implementation of IPC utilities.
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

#include "wawtipcprotocol.h"

namespace BDS {

namespace {

}  // unnamed namespace

                        //--------------------
                        // class ThreadAdapter
                        //--------------------

bool
ThreadAdapter::closeConnection(ConnectionId)
{
    std::lock_guard<std::mutex>  guard(d_lock);

    if (d_connectUpdate) {
        d_messageCallback = MessageCallback();
        d_processorFifo.clear();

        if (d_thread.joinable()) {
            d_thread.detach();
        }
    }
    return true;                                                      // RETURN
}

WawtIpcProtocol::AddressStatus
ThreadAdapter::makeAddress(Address*, std::string*, std::any)
{
    return AddressStatus::eOK;
}

bool
ThreadAdapter::sendMessage(ConnectionId, Message&& message)
{
    std::lock_guard<std::mutex>  guard(d_lock);

    if (!d_connectUpdate) {
        return false;                                                 // RETURN
    }
    d_processorFifo.emplace_back(std::move(message));

    if (!d_thread.joinable()) {
        d_thread = std::thread( [this]() {
                                    d_lock.lock();
                                    while (!d_processorFifo.empty()) {
                                        auto& ft  = d_processorFifo.front();
                                        auto  msg = std::move(ft);
                                        d_processorFifo.pop_front();
                                        d_lock.unlock();
                                        auto  rsp = messageProcessor(msg);
                                        d_lock.lock();
                                        auto cb = d_messageCallback;

                                        if (!cb) {
                                            break;
                                        }
                                        
                                        if (rsp.second > 0) {
                                            d_lock.unlock();
                                            cb(1, std::move(rsp));
                                            d_lock.lock();
                                        }
                                    }
                                    auto cb = d_connectUpdate;

                                    if (!d_messageCallback && cb) {
                                        d_connectUpdate = ConnectCallback();
                                        d_lock.unlock();
                                        cb(1, ConnectionStatus::eDISCONNECT);
                                    }
                                    else {
                                        d_lock.unlock();
                                    }
                                });
    }
    return d_thread.joinable();                                       // RETURN
}

WawtIpcProtocol::ConnectionId
ThreadAdapter::establishConnection(std::string           *diagnostic,
                                   ConnectCallback        connectionUpdate,
                                   MessageCallback        receivedMessage,
                                   const Address&)
{
    std::unique_lock<std::mutex>  guard(d_lock);
    *diagnostic = "";

    if (!d_connectUpdate) {
        return false;                                                 // RETURN
    }
    d_connectUpdate    = connectionUpdate;
    d_messageCallback  = std::move(receivedMessage);
    guard.release();
    connectionUpdate(1, ConnectionStatus::eOK);
    return 1;
}

}  // namespace BDS
// vim: ts=4:sw=4:et:ai
