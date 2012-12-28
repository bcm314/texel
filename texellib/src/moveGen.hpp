/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * moveGen.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef MOVEGEN_HPP_
#define MOVEGEN_HPP_

#include "move.hpp"
#include "position.hpp"
#include "util.hpp"

#include <assert.h>

//#define MOVELIST_DEBUG

#ifdef MOVELIST_DEBUG
# include <set>
# include "textio.hpp"
#endif

/**
 * Generates move lists (pseudo-legal, legal, check evasions, captures).
 */
class MoveGen {
private:
    static const int MAX_MOVES = 256;

public:

    /** A stack-allocated move list object. */
    class MoveList {
    private:
        int buf[sizeof(Move[MAX_MOVES])/sizeof(int)];
    public:
        int size;

        MoveList() : size(0) { }
        void filter(const std::vector<Move>& searchMoves);
        void clear() { size = 0; }

              Move& operator[](int i)        { return ((Move*)&buf[0])[i]; }
        const Move& operator[](int i) const  { return ((Move*)&buf[0])[i]; }

        void addMove(int from, int to, int promoteTo) {
            Move& m = (*this)[size++];
            new (&m) Move(from, to, promoteTo, 0);
        }
    };

    /**
     * Generate and return a list of pseudo-legal moves.
     * Pseudo-legal means that the moves don't necessarily defend from check threats.
     */
    static void pseudoLegalMoves(const Position& pos, MoveList& moveList);

    /**
     * Generate and return a list of pseudo-legal check evasion moves.
     * Pseudo-legal means that the moves don't necessarily defend from check threats.
     */
    static void checkEvasions(const Position& pos, MoveList& moveList);

    /** Generate captures, checks, and possibly some other moves that are too hard to filter out. */
    static void pseudoLegalCapturesAndChecks(const Position& pos, MoveList& moveList);

    static void pseudoLegalCaptures(const Position& pos, MoveList& moveList);

    /**
     * Return true if the side to move is in check.
     */
    static bool inCheck(const Position& pos) {
        int kingSq = pos.getKingSq(pos.whiteMove);
        return sqAttacked(pos, kingSq);
    }

    /**
     * Return true if making a move delivers check to the opponent
     */
    static bool givesCheck(const Position& pos, const Move& m);

    /**
     * Return true if the side to move can take the opponents king.
     */
    static bool canTakeKing(Position& pos) {
        pos.setWhiteMove(!pos.whiteMove);
        bool ret = inCheck(pos);
        pos.setWhiteMove(!pos.whiteMove);
        return ret;
    }

    /**
     * Return true if a square is attacked by the opposite side.
     */
    static bool sqAttacked(const Position& pos, int sq) {
        U64 occupied = pos.whiteBB | pos.blackBB;
        return sqAttacked(pos, sq, occupied);
    }
    static bool sqAttacked(const Position& pos, int sq, U64 occupied) {
        return pos.whiteMove ? sqAttacked<true>(pos, sq, occupied)
                             : sqAttacked<false>(pos, sq, occupied);
    }
    template <bool wtm>
    static bool sqAttacked(const Position& pos, int sq, U64 occupied) {
        if (wtm) {
            if ((BitBoard::knightAttacks[sq] & pos.pieceTypeBB[Piece::BKNIGHT]) != 0)
                return true;
            if ((BitBoard::kingAttacks[sq] & pos.pieceTypeBB[Piece::BKING]) != 0)
                return true;
            if ((BitBoard::wPawnAttacks[sq] & pos.pieceTypeBB[Piece::BPAWN]) != 0)
                return true;
            U64 bbQueen = pos.pieceTypeBB[Piece::BQUEEN];
            if ((BitBoard::bishopAttacks(sq, occupied) & (pos.pieceTypeBB[Piece::BBISHOP] | bbQueen)) != 0)
                return true;
            if ((BitBoard::rookAttacks(sq, occupied) & (pos.pieceTypeBB[Piece::BROOK] | bbQueen)) != 0)
                return true;
        } else {
            if ((BitBoard::knightAttacks[sq] & pos.pieceTypeBB[Piece::WKNIGHT]) != 0)
                return true;
            if ((BitBoard::kingAttacks[sq] & pos.pieceTypeBB[Piece::WKING]) != 0)
                return true;
            if ((BitBoard::bPawnAttacks[sq] & pos.pieceTypeBB[Piece::WPAWN]) != 0)
                return true;
            U64 bbQueen = pos.pieceTypeBB[Piece::WQUEEN];
            if ((BitBoard::bishopAttacks(sq, occupied) & (pos.pieceTypeBB[Piece::WBISHOP] | bbQueen)) != 0)
                return true;
            if ((BitBoard::rookAttacks(sq, occupied) & (pos.pieceTypeBB[Piece::WROOK] | bbQueen)) != 0)
                return true;
        }
        return false;
    }

    /**
     * Remove all illegal moves from moveList.
     * "moveList" is assumed to be a list of pseudo-legal moves.
     * This function removes the moves that don't defend from check threats.
     */
    static void removeIllegal(Position& pos, MoveList& moveList);

    /** Return true if the pseudo-legal move "move" is legal is position "pos". */
    static bool isLegal(Position& pos, const Move& move);

private:
    /**
     * Return the next piece in a given direction, starting from sq.
     */
    static int nextPiece(const Position& pos, int sq, int delta) {
        while (true) {
            sq += delta;
            int p = pos.getPiece(sq);
            if (p != Piece::EMPTY)
                return p;
        }
        assert(false);
        return -1;
    }

    /** Like nextPiece(), but handles board edges. */
    static int nextPieceSafe(const Position& pos, int sq, int delta) {
        int dx = 0, dy = 0;
        switch (delta) {
        case 1: dx=1; dy=0; break;
        case 9: dx=1; dy=1; break;
        case 8: dx=0; dy=1; break;
        case 7: dx=-1; dy=1; break;
        case -1: dx=-1; dy=0; break;
        case -9: dx=-1; dy=-1; break;
        case -8: dx=0; dy=-1; break;
        case -7: dx=1; dy=-1; break;
        }
        int x = Position::getX(sq);
        int y = Position::getY(sq);
        while (true) {
            x += dx;
            y += dy;
            if ((x < 0) || (x > 7) || (y < 0) || (y > 7)) {
                return Piece::EMPTY;
            }
            int p = pos.getPiece(Position::getSquare(x, y));
            if (p != Piece::EMPTY)
                return p;
        }
        assert(false);
        return -1;
    }

    static void addPawnMovesByMask(MoveList& moveList, const Position& pos, U64 mask,
                                   int delta, bool allPromotions) {
        if (mask == 0)
            return;
        U64 promMask = mask & BitBoard::maskRow1Row8;
        mask &= ~promMask;
        while (promMask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(promMask);
            int sq0 = sq + delta;
            if (sq >= 56) { // White promotion
                moveList.addMove(sq0, sq, Piece::WQUEEN);
                moveList.addMove(sq0, sq, Piece::WKNIGHT);
                if (allPromotions) {
                    moveList.addMove(sq0, sq, Piece::WROOK);
                    moveList.addMove(sq0, sq, Piece::WBISHOP);
                }
            } else { // Black promotion
                moveList.addMove(sq0, sq, Piece::BQUEEN);
                moveList.addMove(sq0, sq, Piece::BKNIGHT);
                if (allPromotions) {
                    moveList.addMove(sq0, sq, Piece::BROOK);
                    moveList.addMove(sq0, sq, Piece::BBISHOP);
                }
            }
            promMask &= (promMask - 1);
        }
        while (mask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask);
            moveList.addMove(sq + delta, sq, Piece::EMPTY);
            mask &= (mask - 1);
        }
    }

    static void addPawnDoubleMovesByMask(MoveList& moveList, const Position& pos,
                                         U64 mask, int delta) {
        while (mask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask);
            moveList.addMove(sq + delta, sq, Piece::EMPTY);
            mask &= (mask - 1);
        }
    }

    static void addMovesByMask(MoveList& moveList, const Position& pos, int sq0, U64 mask) {
        while (mask != 0) {
            int sq = BitBoard::numberOfTrailingZeros(mask);
            moveList.addMove(sq0, sq, Piece::EMPTY);
            mask &= (mask - 1);
        }
    }

    /** Not implemented. */
    MoveGen();
};


#endif /* MOVEGEN_HPP_ */
