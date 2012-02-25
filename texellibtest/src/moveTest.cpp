#include "moveTest.hpp"

#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"

#include "position.hpp"
#include "move.hpp"
#include "piece.hpp"

/**
 * Test of move constructor, of class Move.
 */
void testMoveConstructor() {
    int f = Position::getSquare(4, 1);
    int t = Position::getSquare(4, 3);
    int p = Piece::WROOK;
    Move move(f, t, p);
    ASSERT_EQUAL(move.from(), f);
    ASSERT_EQUAL(move.to(), t);
    ASSERT_EQUAL(move.promoteTo(), p);
}

/**
 * Test of equals, of class Move.
 */
void testEquals() {
    Move m1(Position::getSquare(0, 6), Position::getSquare(1, 7), Piece::WROOK);
    Move m2(Position::getSquare(0, 6), Position::getSquare(0, 7), Piece::WROOK);
    Move m3(Position::getSquare(1, 6), Position::getSquare(1, 7), Piece::WROOK);
    Move m4(Position::getSquare(0, 6), Position::getSquare(1, 7), Piece::WKNIGHT);
    Move m5(Position::getSquare(0, 6), Position::getSquare(1, 7), Piece::WROOK);
    ASSERT(!m1.equals(m2));
    ASSERT(!m1.equals(m3));
    ASSERT(!m1.equals(m4));
    ASSERT(m1.equals(m5));
}

cute::suite
MoveTest::getSuite() const {
	cute::suite s;
    s.push_back(CUTE(testMoveConstructor));
    s.push_back(CUTE(testEquals));
	return s;
}
