#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <iostream>

// Включаем подмену прав доступа для тестирования приватных методов
#define private public
#define protected public
#include "../src/player/my_player.hpp"
#include "../src/core/game.hpp"
#include "../src/core/state.hpp"
#undef private
#undef protected

using namespace ttt::game;
using namespace ttt::my_player;

// Единая фикстура для всех тестов, содержащая общие параметры поля
class MyPlayerUnitRulesTest : public ::testing::Test {
protected:
    int cols = 15;
    int rows = 15;
    int wl = 5;
    std::vector<Sign> empty_board;
    
    // Структура настроек внутри ttt::game::State
    State::Opts opts; 

    void SetUp() override {
        empty_board.assign(cols * rows, Sign::NONE); 
        
        opts.cols = cols;
        opts.rows = rows;
        opts.win_len = wl; 
    }
};

// 1. Тест на get_weight
TEST_F(MyPlayerUnitRulesTest, Test_GetWeight) {
    MyPlayer player("TestBot");
    int win_weight = player.get_weight(5, 0);
    EXPECT_GE(win_weight, 1000000);
    
    int blocked_weight = player.get_weight(3, 0);
    int open_weight = player.get_weight(3, 2);
    EXPECT_GT(open_weight, blocked_weight);
}

// 2. Тест на хеширование
TEST_F(MyPlayerUnitRulesTest, Test_ZobristHashing) {
    MyPlayer player("TestBot");
    uint64_t hash_empty = player.zobrist_hash(empty_board, cols, rows);
    
    std::vector<Sign> modified_board = empty_board;
    modified_board[0] = Sign::X;
    uint64_t hash_modified = player.zobrist_hash(modified_board, cols, rows);
    
    EXPECT_NE(hash_empty, hash_modified);
}

// 3. Тест на поиск свободной клетки (центра)
TEST_F(MyPlayerUnitRulesTest, Test_FindAnyFreeCell) {
    MyPlayer player("TestBot");
    Point central_point = player.find_any_free_cell(empty_board, cols, rows);
    EXPECT_EQ(central_point.x, cols / 2);
    EXPECT_EQ(central_point.y, rows / 2);
}

// 4. Тест на генерацию ходов
TEST_F(MyPlayerUnitRulesTest, Test_GenerateMoves) {
    MyPlayer player("TestBot");
    std::vector<Sign> board = empty_board;
    board[(rows / 2) * cols + (cols / 2)] = Sign::X;
    
    auto candidates = player.generate_moves(board, cols, rows, wl, Sign::O, Sign::X);
    EXPECT_FALSE(candidates.empty());
}

// 5. Тест на оценку конкретной клетки
TEST_F(MyPlayerUnitRulesTest, Test_EvaluateCell) {
    MyPlayer player("TestBot");
    long long score_empty = player.evaluate_cell(empty_board, cols, rows, wl, 5, 5, Sign::X);
    
    std::vector<Sign> board = empty_board;
    board[5 * cols + 4] = Sign::X;
    long long score_with_neighbor = player.evaluate_cell(board, cols, rows, wl, 5, 5, Sign::X);
    
    EXPECT_GT(score_with_neighbor, score_empty);
}

// 6. Тест на общую оценку доски
TEST_F(MyPlayerUnitRulesTest, Test_EvaluateBoard) {
    MyPlayer player("TestBot");
    long long initial_score = player.evaluate_board(empty_board, cols, rows, wl, Sign::X, Sign::O);
    EXPECT_EQ(initial_score, 0);
}

// 7. Тест на ядро поиска (alphabeta)
TEST_F(MyPlayerUnitRulesTest, Test_AlphaBetaSearch) {
    MyPlayer player("TestBot");
    auto start_time = std::chrono::steady_clock::now();
    uint64_t current_hash = player.zobrist_hash(empty_board, cols, rows);
    
    long long search_result = player.alphabeta(
        empty_board, cols, rows, wl, 1, 
        -2000000000000LL, 2000000000000LL, 
        true, Sign::X, Sign::X, Sign::O, 
        start_time, 90, current_hash
    );
    SUCCEED();
}

// --- Поведенческие тесты ИИ ---

TEST_F(MyPlayerUnitRulesTest, FirstMoveIsCenter) {
    State state(opts);
    MyPlayer player("TestBot");
    player.set_sign(Sign::X);
    
    Point move = player.make_move(state);
    
    EXPECT_EQ(move.x, opts.cols / 2);
    EXPECT_EQ(move.y, opts.rows / 2);
}

TEST_F(MyPlayerUnitRulesTest, MoveIsWithinBounds) {
    State state(opts);
    MyPlayer player("TestBot");
    player.set_sign(Sign::O);
    
    state.process_move(Sign::X, 7, 7); 
    Point move = player.make_move(state);
    
    EXPECT_GE(move.x, 0);
    EXPECT_LT(move.x, opts.cols);
    EXPECT_GE(move.y, 0);
    EXPECT_LT(move.y, opts.rows);
}

TEST_F(MyPlayerUnitRulesTest, TakesWinningMoveHorizontal) {
    State state(opts);
    state.process_move(Sign::X, 5, 5);
    state.process_move(Sign::O, 0, 0);
    state.process_move(Sign::X, 6, 5);
    state.process_move(Sign::O, 0, 1);
    state.process_move(Sign::X, 7, 5);
    state.process_move(Sign::O, 0, 2);
    state.process_move(Sign::X, 8, 5); 
    
    MyPlayer player("TestBot");
    player.set_sign(Sign::X);
    
    Point move = player.make_move(state);
    
    bool is_winning_move = (move.x == 4 && move.y == 5) || (move.x == 9 && move.y == 5);
    EXPECT_TRUE(is_winning_move);
}

TEST_F(MyPlayerUnitRulesTest, RespectsTimeLimit) {
    State state(opts);
    MyPlayer player("TestBot");
    player.set_sign(Sign::X);
    
    state.process_move(Sign::O, 7, 7);
    state.process_move(Sign::X, 7, 8);
    
    auto start_time = std::chrono::steady_clock::now();
    player.make_move(state);
    auto end_time = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    EXPECT_LT(duration_ms, 200); 
}

TEST_F(MyPlayerUnitRulesTest, BlocksDiagonalThreat) {
    State state(opts);
    state.process_move(Sign::O, 5, 5);
    state.process_move(Sign::X, 0, 0);
    state.process_move(Sign::O, 6, 6);
    state.process_move(Sign::X, 0, 1);
    state.process_move(Sign::O, 7, 7);
    state.process_move(Sign::X, 0, 2);
    state.process_move(Sign::O, 8, 8); 

    MyPlayer player("TestBot");
    player.set_sign(Sign::X); 
    
    Point move = player.make_move(state);
    
    bool blocks_threat = (move.x == 4 && move.y == 4) || (move.x == 9 && move.y == 9);
    EXPECT_TRUE(blocks_threat);
}