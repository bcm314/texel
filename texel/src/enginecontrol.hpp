/*
 * enginecontrol.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef ENGINECONTROL_HPP_
#define ENGINECONTROL_HPP_

#include "search.hpp"
#include "transpositionTable.hpp"
#include "position.hpp"
#include "move.hpp"

#include <vector>
#include <iosfwd>
#include <thread>
#include <mutex>
#include <memory>

class SearchParams;


/**
 * Control the search thread.
 */
class EngineControl {
private:
    std::ostream& os;

    std::shared_ptr<std::thread> engineThread;
    std::mutex threadMutex;
    volatile bool shouldDetach;
    std::shared_ptr<Search> sc;
    TranspositionTable tt;

    Position pos;
    std::vector<U64> posHashList;
    int posHashListSize;
    volatile bool ponder;     // True if currently doing pondering
    bool onePossibleMove;
    volatile bool infinite;

    int minTimeLimit;
    int maxTimeLimit;
    int maxDepth;
    int maxNodes;
    std::vector<Move> searchMoves;

    // Options
    int hashSizeMB;
    bool ownBook;
    bool analyseMode;
    bool ponderMode;

    // Reduced strength variables
    int strength;
    U64 randomSeed;

    /**
     * This class is responsible for sending "info" strings during search.
     */
    class SearchListener : public Search::Listener {
        std::ostream& os;

    public:
        SearchListener(std::ostream& os0);

        void notifyDepth(int depth);

        void notifyCurrMove(const Move& m, int moveNr);

        void notifyPV(int depth, int score, int time, U64 nodes, int nps, bool isMate,
                      bool upperBound, bool lowerBound, const std::vector<Move>& pv);

        void notifyStats(U64 nodes, int nps, int time);
    };

public:
    EngineControl(std::ostream& o);

    void startSearch(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar);

    void startPonder(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar);

    void ponderHit();

    void stopSearch();

    void newGame();

    /**
     * Compute thinking time for current search.
     */
    void computeTimeLimit(const SearchParams& sPar);

    static void printOptions(std::ostream& os);

    void setOption(const std::string& optionName, const std::string& optionValue);

private:
    static int clamp(int val, int min, int max);

    void startThread(int minTimeLimit, int maxTimeLimit, int maxDepth, int maxNodes);

    void stopThread();

    void setupTT();

    void setupPosition(Position pos, const std::vector<Move>& moves);

    /**
     * Try to find a move to ponder from the transposition table.
     */
    Move getPonderMove(Position pos, const Move& m);

    static std::string moveToString(const Move& m);
};


#endif /* ENGINECONTROL_HPP_ */