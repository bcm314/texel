/*
 * chesstool.cpp
 *
 *  Created on: Dec 24, 2013
 *      Author: petero
 */

#include "chesstool.hpp"
#include "search.hpp"
#include "textio.hpp"
#include "gametree.hpp"
#include "stloutput.hpp"


ScoreToProb::ScoreToProb(double pawnAdvantage0)
    : pawnAdvantage(pawnAdvantage0) {
    for (int i = 0; i < MAXCACHE; i++)
        cache[i] = -1;
}

double
ScoreToProb::getProb(int score) {
    bool neg = false;
    if (score < 0) {
        score = -score;
        neg = true;
    }
    double ret = -1;
    if (score < MAXCACHE) {
        if (cache[score] < 0)
            cache[score] = computeProb(score);
        ret = cache[score];
    }
    if (ret < 0)
        ret = computeProb(score);
    if (neg)
        ret = 1 - ret;
    return ret;
}

// --------------------------------------------------------------------------------

void
ChessTool::pgnToFen(std::istream& is) {
    static std::vector<U64> nullHist(200);
    static TranspositionTable tt(19);
    static ParallelData pd(tt);
    static KillerTable kt;
    static History ht;
    static auto et = Evaluate::getEvalHashTables();
    static Search::SearchTables st(tt, kt, ht, *et);
    static TreeLogger treeLog;

    Position pos;
    const int mate0 = SearchConst::MATE0;
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    const int plyScale = SearchConst::plyScale;

    GameTree gt(is);
    while (gt.readPGN()) {
        GameTree::Result result = gt.getResult();
        if (result == GameTree::UNKNOWN)
            continue;
        double rScore = 0;
        switch (result) {
        case GameTree::WHITE_WIN: rScore = 1.0; break;
        case GameTree::BLACK_WIN: rScore = 0.0; break;
        case GameTree::DRAW:      rScore = 0.5; break;
        default: break;
        }
        GameNode gn = gt.getRootNode();
        while (true) {
            pos = gn.getPos();
            std::string fen = TextIO::toFEN(pos);
            if (gn.nChildren() == 0)
                break;
            gn.goForward(0);
//            std::string move = TextIO::moveToUCIString(gn.getMove());
            std::string comment = gn.getComment();
            int commentScore;
            if (!getCommentScore(comment, commentScore))
                continue;

            sc.init(pos, nullHist, 0);
            int score = sc.negaScout(-mate0, mate0, 0, 0*plyScale, -1, MoveGen::inCheck(pos));
            if (!pos.getWhiteMove()) {
                score = -score;
                commentScore = -commentScore;
            }

            std::cout << fen << " : " << rScore << " : " << commentScore << " : " << score << std::endl;
        }
    }
}

void
ChessTool::pawnAdvTable(std::istream& is) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    qEval(positions);
    for (int pawnAdvantage = 1; pawnAdvantage <= 400; pawnAdvantage += 1) {
        ScoreToProb sp(pawnAdvantage);
        double avgErr = computeAvgError(positions, sp);
        std::stringstream ss;
        ss << "pa:" << pawnAdvantage << " err:" << std::setprecision(14) << avgErr;
        std::cout << ss.str() << std::endl;
    }
}

void
ChessTool::filterFEN(std::istream& is) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    ScoreToProb sp;
    Position pos;
    for (const PositionInfo& pi : positions) {
        double p1 = sp.getProb(pi.searchScore);
        double p2 = sp.getProb(pi.qScore);
        if ((std::abs(p1 - p2) < 0.05) && (std::abs(pi.searchScore - pi.qScore) < 200)) {
            pos.deSerialize(pi.posData);
            std::string fen = TextIO::toFEN(pos);
            std::cout << fen << " : " << pi.result << " : " << pi.searchScore << " : " << pi.qScore << std::endl;
        }
    }
}

void ChessTool::paramEvalRange(std::istream& is, ParamDomain& pd) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    ScoreToProb sp;
    double bestVal = 1e100;
    for (int i = pd.minV; i <= pd.maxV; i += pd.step) {
        Parameters::instance().set(pd.name, num2Str(i));
        qEval(positions);
        double avgErr = computeAvgError(positions, sp);
        bool best = avgErr < bestVal;
        bestVal = std::min(bestVal, avgErr);
        std::stringstream ss;
        ss << "i:" << i << " err:" << std::setprecision(14) << avgErr << (best?" *":"");
        std::cout << ss.str() << std::endl;
    }
}

bool
ChessTool::getCommentScore(const std::string& comment, int& score) {
    double fScore;
    if (!str2Num(comment, fScore))
        return false;
    score = (int)std::round(fScore * 100);
    return true;
}

void
splitString(const std::string& line, const std::string& delim, std::vector<std::string>& fields) {
    size_t start = 0;
    while (true) {
        size_t n = line.find(delim, start);
        if (n == std::string::npos)
            break;
        fields.push_back(line.substr(start, n - start));
        start = n + delim.length();
    }
    if (start < line.length())
        fields.push_back(line.substr(start));
}

void
ChessTool::readFENFile(std::istream& is, std::vector<PositionInfo>& data) {
    Position pos;
    PositionInfo pi;
    while (true) {
        std::string line;
        std::getline(is, line);
        if (!is || is.eof())
            break;
        std::vector<std::string> fields;
        splitString(line, " : ", fields);
        if (fields.size() != 4) {
            std::cout << "line:" << line << std::endl;
            std::cout << "fields:" << fields << std::endl;
            throw ChessParseError("Invalid file format");
        }
        pos = TextIO::readFEN(fields[0]);
        pos.serialize(pi.posData);
        if (!str2Num(fields[1], pi.result) ||
            !str2Num(fields[2], pi.searchScore) ||
            !str2Num(fields[3], pi.qScore))
            throw ChessParseError("Invalid file format");
        data.push_back(pi);
    }
}

void ChessTool::qEval(std::vector<PositionInfo>& positions) {
    TranspositionTable tt(19);

    const int nPos = positions.size();
    const int chunkSize = 100000;

#pragma omp parallel for default(none) shared(positions,tt)
    for (int c = 0; c < nPos; c += chunkSize) {
        std::vector<U64> nullHist(200);
        ParallelData pd(tt);
        KillerTable kt;
        History ht;
        auto et = Evaluate::getEvalHashTables();
        Search::SearchTables st(tt, kt, ht, *et);
        TreeLogger treeLog;

        Position pos;
        const int mate0 = SearchConst::MATE0;
        Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
        const int plyScale = SearchConst::plyScale;

        for (int i = 0; i < chunkSize; i++) {
            if (c + i >= nPos)
                break;
            PositionInfo& pi = positions[c + i];
            pos.deSerialize(pi.posData);
            sc.init(pos, nullHist, 0);
            int score = sc.negaScout(-mate0, mate0, 0, 0*plyScale, -1, MoveGen::inCheck(pos));
            if (!pos.getWhiteMove())
                score = -score;
            pi.qScore = score;
        }
    }
}

double
ChessTool::computeAvgError(const std::vector<PositionInfo>& positions, ScoreToProb& sp) {
    double errSum = 0;
    for (const PositionInfo& pi : positions) {
        double p = sp.getProb(pi.qScore);
        double err = p - pi.result;
        errSum += err * err;
    }
    double avgErr = sqrt(errSum / positions.size());
    return avgErr;
}
