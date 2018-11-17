/** @file gamemaster.h
 *  @brief Implement the Tic-Tac-DOH! game's rules.
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

#ifndef TICTACDOH_GAMEMASTER_H
#define TICTACDOH_GAMEMASTER_H

                            //=================
                            // class GameMaster
                            //=================

/**
 * Implement the game of Tic-Tac-DOH!
 *
 * The rules of the game:
 * At the start of the game, each player gets 50 points. The two
 * players face off and play a game of Tic-Tac-Toe with 10 points
 * at stake.  The winner of that game gets the points. Players play
 * games until one side concedes, or no longer has sufficient
 * points for the ante.
 *
 * During the Tic-Tac-Toe game each player can, on any turn, challenge
 * their opponent to "double the ante" to 20 points.  The opponent can
 * either concede the game and 10 points, or agree to the doubling.
 *
 * Each turn of the Tic-Tac-Toe game consists of each player choosing
 * an unoccupied board square.  When both players have made their choice
 * they simultaneously reveal those choices. If each player chooses a
 * different square, then both squares become occupied with their player's
 * marker.
 *
 * If both player's choose the same square, then the "rock-siccsors-paper"
 * game is played until one side side wins.
 *
 * The Tic-Tac-Toe game ends with any row, column or diagonal are
 * occupied by one side's marker. Or when all the squares of the board
 * are occupied.  If both sides occupy a row, column or diagonal at
 * the end of the turn, then "rock-siccsors-paper" game determines the
 * winner.  If all the squares are occupied the winner is the side
 * with the most squares containing their marker.
 */

class GameMaster {
};

#endif

// vim: ts=4:sw=4:et:ai
