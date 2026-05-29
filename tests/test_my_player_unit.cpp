#include <gtest/gtest.h>
#include <chrono>
#include "../src/player/my_player.hpp"
#include "../src/core/game.hpp"
#include "../src/core/state.hpp"


using namespace ttt::game;
using namespace ttt::my_player;

class MyPlayerTest : public ::testing::Test {
protected:
    State::Opts opts;
    
    void SetUp() override {
        
        opts.cols = 15;
        opts.rows = 15;
        opts.win_len = 5;
    }
};

// ТЕСТ 1: Первый ход бота всегда должен быть в центр поля 
TEST_F(MyPlayerTest, FirstMoveIsCenter) {
    State state(opts);
    MyPlayer player("TestBot");
    player.set_sign(Sign::X);
    
    Point move = player.make_move(state);
    
    EXPECT_EQ(move.x, opts.cols / 2) << "Первый ход должен быть по центру X";
    EXPECT_EQ(move.y, opts.rows / 2) << "Первый ход должен быть по центру Y";
}

// ТЕСТ 2: Бот никогда не должен пытаться сходить за пределы поля
TEST_F(MyPlayerTest, MoveIsWithinBounds) {
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

// ТЕСТ 3: Бот должен мгновенно побеждать, если у него есть открытая четверка
TEST_F(MyPlayerTest, TakesWinningMove) {
    State state(opts);
    
    
    state.process_move(Sign::X, 5, 5);
    state.process_move(Sign::O, 5, 6); 
    state.process_move(Sign::X, 6, 5);
    state.process_move(Sign::O, 6, 6);
    state.process_move(Sign::X, 7, 5);
    state.process_move(Sign::O, 7, 6);
    state.process_move(Sign::X, 8, 5); 
    state.process_move(Sign::O, 8, 7); 
    
    MyPlayer player("TestBot");
    player.set_sign(Sign::X);
    
    // Ход X. Бот должен понять, что может выиграть, поставив камень на 4,5 или 9,5
    Point move = player.make_move(state);
    
    bool is_winning_move = (move.x == 4 && move.y == 5) || (move.x == 9 && move.y == 5);
    EXPECT_TRUE(is_winning_move) << "Бот не забрал 100% победу!";
}

// ТЕСТ 4: Бот ОБЯЗАН блокировать противника, если у того 4 в ряд
TEST_F(MyPlayerTest, BlocksOpponentWinningMove) {
    State state(opts);
    
   
    state.process_move(Sign::X, 2, 2); 
    state.process_move(Sign::O, 5, 5);
    state.process_move(Sign::X, 2, 3);
    state.process_move(Sign::O, 6, 5);
    state.process_move(Sign::X, 2, 4);
    state.process_move(Sign::O, 7, 5);
    state.process_move(Sign::X, 2, 8); 
    state.process_move(Sign::O, 8, 5); 
    
    MyPlayer player("TestBot");
    player.set_sign(Sign::X); 
    

    Point move = player.make_move(state);
    
    bool blocks_threat = (move.x == 4 && move.y == 5) || (move.x == 9 && move.y == 5);
    EXPECT_TRUE(blocks_threat) << "Бот проигнорировал смертельную угрозу и не заблокировал ее!";
}

// ТЕСТ 5: Тайминг Alpha-Beta поиска (Stress Test)
TEST_F(MyPlayerTest, RespectsTimeLimit) {
    State state(opts);
    MyPlayer player("TestBot");
    player.set_sign(Sign::X);
    
    state.process_move(Sign::O, 7, 7);
    state.process_move(Sign::X, 7, 8);
    state.process_move(Sign::O, 8, 7);
    state.process_move(Sign::X, 6, 7);
    state.process_move(Sign::O, 6, 6);
    state.process_move(Sign::X, 8, 8);
    
    auto start_time = std::chrono::steady_clock::now();
    player.make_move(state);
    auto end_time = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    EXPECT_LT(duration_ms, 120) << "Бот думал слишком долго (" << duration_ms << " мс)!";
}