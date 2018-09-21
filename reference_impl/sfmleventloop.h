/** @file sfmleventloop.h
 *  @brief Reference implementation of windows event loop using SFML
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

#ifndef BDS_SFMLEVENTLOOP_H
#define BDS_SFMLEVENTLOOP_H

#include <SFML/Graphics/RenderWindow.hpp>

#include "wawteventrouter.h"

namespace BDS {

                            //=====================
                            // struct SfmlEventLoop
                            //=====================


struct SfmlEventLoop {
    using ShutdownCb = std::function<bool(sf::RenderWindow *window)>;

    static void run(sf::RenderWindow&                 window,
                    WawtEventRouter&                  router,
                    const ShutdownCb&                 shutdown,
                    const std::chrono::milliseconds&  pollInterval,
                    int                               minWidth,
                    int                               minHeight);
};

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
