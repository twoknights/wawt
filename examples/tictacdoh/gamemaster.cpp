/** gamemaster.cpp
 *
 *  Play the game of Tic-Tac-DOH!
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

#include "gamemaster.h"

#include <chrono>
#include <string>

using namespace std::literals::chrono_literals;
using namespace Wawt::literals;
using namespace Wawt;

namespace {

const char disconnectFmt[]      = "Disconnected";
const char gameSettingsFmt[]    = "Settings: %d %d";
const char timeoutFmt[]         = "Timeup: %d";

const char boardOccupyFmt[]     = "Occupy: %d";

const char setupCancelFmt[]     = "Cancel-Open";
const char setupParametersFmt[] = "Start: %d %d %63s";

void startTimer(std::thread&                        handle,
                IpcQueue                           *queue,
                std::shared_ptr<std::atomic_int>&   moveCount
                int                                 moveTimeout)
{
    if (handle.joinable()) {
        handle.detach();
    }
    std::duration::seconds timeout(moveTimeout);
    handle
        = std::thread(  [queue,moveCount,timeout]() {
                            auto count = moveCount->load();
                            std::this_thread::sleep_for(timeout);
                            
                            if (count == moveCount->load()) {
                                auto timeup =
                                    IpcUtilities::formatMessage(timeoutFmt,
                                                                count);
                                queue->localEnqueue(std::move(timeup));
                            }
                        });
}

} // unnamed namespace

                            //-----------------
                            // class GameMaster
                            //-----------------


void
GameMaster::gameLoop(IpcQueue&            queue,
                     EventRouter&         router,
                     EventRouter::Handle  gameScreen)
{
    auto moveCounter = std::make_shared<std::atomic_int>;
    std::thread timer;
    
    auto disconnect = IpcUtilities::formatMessage(disconnectFmt);

    try while (true) { // until shutdown (caught) or other exception
        IpcQueue::ReplyQueue    opponent, replyQ;
        IpcQueue::Envelope      envelope;
        IpcQueue::Message       message;
        bool                    moveFirst;
        int                     moveTimeout; // seconds
        char                    configBuf[64];
        std::string             diagnostics;
        std::any                address;

        //
        //  Show Setup screen
        //

        router.activate<SetupScreen>(setupScreen, &queue);
        bool opened     = false;
        bool cancelDone = false;

        try while (!opened) {

            queue.reset(); // closes adapter

            if (cancelDone) {
                router.call(setupScreen,
                            &SetupScreen::startupFeedback,
                            false,
                            "Connection canceled.") || abort();
                cancelDone = false;
            }

            // Wait for user to supply startup information.
            [replyQ, envelope] = queue.waitForIndication();
            assert(replyQ.isLocal());

            message = std::get<IpcQueue::Message>(std::move(envelope));

            if (!IpcUtilities::parseMessage(message,
                                            setupParametersFmt,
                                            &moveFirst,
                                            &moveTimeout,
                                            configBuf)) {
                std::cerr << "Local did not set game settings: '"
                          << IpcUtilities::stringView(message).data()
                          << '\'' << std::endl;
                continue; // start over
            }
            auto status = queue.adapter()
                               ->configureAdapter(&diagnostics,
                                                  std::string(configBuf));

            if (status != IpcConnectProtocol::eOK) {
                router.call(setupScreen,
                            &SetupScreen::startupFeedback,
                            false,
                            diagnostics) || abort();
                continue; // start over
            }

            message = IpcUtilities::formatMessage(gameSettingsFmt,
                                                  moveFirst,
                                                  moveTimeout);

            opened = queue.openAdapter(&diagnostics, disconnect, message);

            router.call(setupScreen,
                        &SetupScreen::startupFeedback,
                        opened,
                        diagnostics) || abort();

            if (!opened) {
                continue; // start over
            }

            [opponent, envelope] = queue.waitForIndication();
            message = std::get<IpcQueue::Message>(std::move(envelope));

            if (opponent.isLocal()) {
                // Only reason for GUI to send a message is to cancel
                // the startup, and try again.
                assert(IpcUtilities::parseMessage(message, setupCancelFmt));
                cancelDone  = true;
                opened      = false;
                continue; // start over
            }

            // 'message' should be the opponent's startup settings.
            // The 'opponent' reply queue will supply the result
            // of a "random" flip of a coin where both local and
            // remote endpoints agree on which side won.
            //
            // The winner of the toss, gets their choice of who moves
            // first, while the loser of the toss gets their prefered
            // move timeout accepted.
            int  timeout;
            bool firstMove; // true if opponent wants to move first.

            if (!IpcUtilities::parseMessage(message,
                                            gameStart,
                                            &firstMove,
                                            &timeout)) {
                std::cerr << "Bad initial message: "
                          << IpcUtilities::stringView(message).data()
                          << std::endl;
                continue; // start over
            }
            // The reply queue gives the "toss" result for a given remote
            // endpoint.  A 'true' result means the remote endpoint won
            // the coin flip.

            if (opponent->tossResult()) { // local endpoint lost the toss
                moveFirst   = !firstMove;
            }
            else {
                moveTimeout = timeout; // adopt opponent's timeout
            }
            // Show the player a pop-up with the startup conditions
            // sleep 5s before activating the game screen.

            router.call(setupScreen, &SetupScreen::gameStarting) || abort();

            std::this_thread::sleep_for(5s);
        }

        //
        //  Show Game screen.
        //

        router.activate(gameScreen, &queue, moveTimout, moveFirst);
        *moveCounter = 1;
        startTimer(timer, &queue, &moveCount, moveTimeout);

        try while (!opponent.isClosed()) {
            const char square[] = "Square: %d";  // square is argument
            IpcQueue::ReplyQueue replyTo;
            int                  sq, timerCount;

            [replyTo, envelope] = queue.waitForIndication();
            auto arrivalTime = std::chrono::system_clock::now();

            message = std::get<IpcQueue::Message>(std::move(envelope));

            if (replyTo.isLocal()) { // from GUI or perhaps "timer"
                if (IpcUtilities::parseMessage(message, "Quit")) {
                    opponent->closeQueue(message);
                }
                else if (IpcUtilities::parseMessage(message, square, sq)) {
                    ++*moveCounter;
                    opponent->enqueue(message);
                    auto result = occupy(0, sq); // 0=continue, 1=win, 2=draw
                    
                    if (result > 0) {
                        router.call(gameScreen, &GameScreen::gameOver, result);
                        opponent->closeQueue();
                    }
                }
                else if (IpcUtilities::parseMessage(message,
                                                    timeoutFmt,
                                                    &timerCount)) {
                    if (timerCount == moveCounter->load()) { // someone forfeit
                        auto result = timerCount % 2 == (moveFirst ? 1 : 0)
                                        ? kGUI_FORFEIT : kOPPONENT_FORFEIT;
                        router.call(gameScreen, &GameScreen::gameOver, result);
                        opponent->closeQueue();
                    }
                }
                else {
                    std::cerr << "Unknown message from GUI: '"
                              << IpcUtilities::stringView(message).data()
                              << '\'' << std::endl;
                    opponent->closeQueue();
                }
            }
            else {
                assert(replyTo == opponent);

                if (IpcUtilities::parseMessage(message, disconnectFmt)) {
                }
                else if (IpcUtilities::parseMessage(message, square, &sq)) {
                    ++*moveCounter;
                    auto result = occupy(1, sq); // 0=continue, 2=draw, 3=loss
                
                    if (result > 0) {
                        router.call(gameScreen, &GameScreen::gameOver, result);
                        opponent->closeQueue();
                    }
                }
                else if (IpcUtilities::parseMessage(message,
                                                    gameOver,
                                                    &forfeit)) {
                    router.call(gameScreen, &GameScreen::gameOver, kFORFEIT);
                    opponent->closeQueue();
                }
                else {
                    std::cerr << "Unknown message from opponent: '"
                              << IpcUtilities::stringView(message).data()
                              << '\'' << std::endl;
                    opponent->closeQueue();
                }
            }
        } catch (IpcQueue::SessionDrop& session) {
            assert(session.sessionId == opponent.sessionId);
            // Cause a pop-up dialog to be shown with the bad news. That
            // dialog will show SetupScreen before closing.
            router.call(gameScreen, &GameScreen::opponentDisconnect);
        }
    } catch (IpcQueue::ShutdownException&) { }
      catch (...) {
          moveCounter = -1;
          if (timer.joinable()) {
              timer.detach();
          }
          throw;
      }

    moveCounter = -1;
    if (timer.joinable()) {
        timer.detach();
    }
    return;                                                           // RETURN
}

    //      Show gameboard, each player points, ante amount
    //      (board shows countdown until forfeit).

    //      Get Square. Player/Opponent: Forfeit, or Double Ante 

    //      if (Opponent Double Ante)
    //          Show challenge, countdown.
    //          Get Accept (adjust ante) or Forfeit.
    //          Send result to opponent.
    //          On accept, go back to begining of loop.
            
    //      if (Opponent Forfeit)
}

void
GameMaster::getMessage(Message *type)
{
    sf::Packet packet;
    packet << static_cast<uint16>(packet);

    if (sf::Socket::Done != d_connection.send(packet)) {
        abortGame();
    }
}

void
GameMaster::sendMessage(Message type)
{
    sf::Packet packet;
    packet << static_cast<uint16>(packet);

    if (sf::Socket::Done != d_connection.send(packet)) {
        abortGame();
    }
}

// vim: ts=4:sw=4:et:ai
