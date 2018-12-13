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
#include <atomic>
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
    class Channel {
      public:
        // PUBLIC TYPES
        enum State  { eREADY, eDROP, eCLOSE, eERROR, ePROTO };

        using MessageChain      = std::forward_list<IpcMessage>;
        using MessageCb         = std::function<void(IpcMessage&&)>;
        using StateCb           = std::function<void(State)>;

        virtual                ~Channel() { }

        //! Asynchronous close of a channel.
        virtual void            closeChannel()                      noexcept=0;

        virtual void            completeSetup(MessageCb&&    receivedMessage,
                                              StateCb&&      channelClose)
                                                                    noexcept=0;

        //! Asynchronous call to send a message on a channel
        virtual bool            sendMessage(MessageChain&&  chain)  noexcept=0;

        virtual State           state()                       const noexcept=0;
    };

    using ChannelPtr  = std::weak_ptr<Channel>;

    class Provider {
      public:
        // PUBLIC TYPES
        struct SetupBase {
            enum Status { eINPROGRESS
                        , eCANCELED
                        , eMALFORMED
                        , eINVALID
                        , eERROR
                        , eFINISH };
            std::atomic<Status>                 d_setupStatus{};
            const std::any                      d_configuration;

            SetupBase(std::any&& configuration)
                : d_configuration(std::move(configuration)) {
                    d_setupStatus = eINPROGRESS;
                }
        };

        using SetupTicket = std::shared_ptr<SetupBase>;
        using SetupCb     = std::function<void(const ChannelPtr&,
                                               const SetupTicket&)>;

        virtual bool            acceptChannels(String_t    *diagnostic,
                                               SetupTicket  ticket,
                                               SetupCb&&    channelSetupDone)
                                                                    noexcept=0;

        // On return, setup is not "in-progress", but may have "finished".
        virtual bool            cancelSetup(const SetupTicket& ticket)
                                                                    noexcept=0;

        //! Create a channel to a peer that is accepting channels.
        virtual bool            createNewChannel(String_t    *diagnostic,
                                                 SetupTicket  ticket,
                                                 SetupCb&&    channelSetupDone)
                                                                    noexcept=0;

        // Asynchronous cancel of all pending channels. No new ones permitted.
        virtual void            shutdown()                          noexcept=0;
    };
};

} // end Wawt namespace

#endif
// vim: ts=4:sw=4:et:ai
