#include <gtest/gtest.h>
#include <chrono>
#include "../src/player/my_player.hpp"
#include "../src/core/game.hpp"

using namespace ttt::game;
using namespace ttt::my_player;

class MyPlayerTest : public ::testing::Test {
protected:
    State::Opts opts;
    
    void SetUp() override {
        // Настраиваем стандартное поле 15x15 для Гомоку
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

// ТЕСТ 3: Бот должен мгновенно побеждать по горизонтали
TEST_F(MyPlayerTest, TakesWinningMoveHorizontal) {
    State state(opts);
    
    state.process_move(Sign::X, 5, 5);
    state.process_move(Sign::O, 5, 6);
    state.process_move(Sign::X, 6, 5);
    state.process_move(Sign::O, 6, 6);
    state.process_move(Sign::X, 7, 5);
    state.process_move(Sign::O, 7, 6);
    state.process_move(Sign::X, 8, 5); // 4 в ряд у X
    state.process_move(Sign::O, 8, 7); 
    
    MyPlayer player("TestBot");
    player.set_sign(Sign::X);
    
    Point move = player.make_move(state);
    
    bool is_winning_move = (move.x == 4 && move.y == 5) || (move.x == 9 && move.y == 5);
    EXPECT_TRUE(is_winning_move) << "Бот не забрал 100% победу по горизонтали!";
}

// ТЕСТ 4: Тайминг Alpha-Beta поиска (Stress Test)
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

// ТЕСТ 5: Бот ОБЯЗАН блокировать диагональную угрозу противника
TEST_F(MyPlayerTest, BlocksDiagonalThreat) {
    State state(opts);
    
    state.process_move(Sign::X, 1, 1);
    state.process_move(Sign::O, 5, 5);
    state.process_move(Sign::X, 1, 2);
    state.process_move(Sign::O, 6, 6);
    state.process_move(Sign::X, 2, 1);
    state.process_move(Sign::O, 7, 7);
    state.process_move(Sign::X, 2, 2); // X просто тратит ход
    state.process_move(Sign::O, 8, 8); // У O собрана смертельная линия: (5,5), (6,6), (7,7), (8,8)
    
    MyPlayer player("TestBot");
    player.set_sign(Sign::X); 
    
    Point move = player.make_move(state);
    
    // Бот обязан поставить камень на края этой диагонали: (4,4) или (9,9)
    bool blocks_threat = (move.x == 4 && move.y == 4) || (move.x == 9 && move.y == 9);
    EXPECT_TRUE(blocks_threat) << "Бот проигнорировал смертельную ДИАГОНАЛЬНУЮ угрозу!";
}

// ТЕСТ 6: Бот правильно адаптируется к другой длине победной линии (win_len)
TEST_F(MyPlayerTest, AdaptsToDynamicWinLength) {
    // Создаем нестандартное поле: играем до 3 победных камней (win_len = 3)
    State::Opts custom_opts{10, 10, 3}; 
    State state(custom_opts);
    
    // X (наш бот) ставит 2 камня в ряд. Для победы нужен всего один!
    state.process_move(Sign::X, 5, 5);
    state.process_move(Sign::O, 2, 2);
    state.process_move(Sign::X, 6, 5); // Камни X: (5,5) и (6,5)
    state.process_move(Sign::O, 2, 3);
    
    MyPlayer player("TestBot");
    player.set_sign(Sign::X);
    
    Point move = player.make_move(state);
    
    // Бот должен понимать, что win_len=3, и немедленно выигрывать на (4,5) или (7,5)
    bool wins = (move.x == 4 && move.y == 5) || (move.x == 7 && move.y == 5);
    EXPECT_TRUE(wins) << "Бот не смог адаптироваться к win_len = 3 и упустил победу!";
}