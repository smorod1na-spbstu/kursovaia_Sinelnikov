#pragma once

#include "core/game.hpp"
#include <chrono>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace ttt::my_player {

struct TTEntry { long long score; int depth; };
struct Move { ttt::game::Point p; long long score; };

class MyPlayer : public game::IPlayer {
private:
    game::Sign m_sign = game::Sign::NONE;
    const char *m_name;
    int m_win_len = 5;

    // 1. Избавляемся от глобальных переменных. Теперь у каждого бота своя память!
    std::vector<std::vector<int>> m_history_table;
    std::vector<uint64_t> m_zobrist_table;
    bool m_zobrist_init = false;
    std::unordered_map<uint64_t, TTEntry> m_transposition_table;

    // 2. Все вспомогательные функции становятся приватными методами класса
    void init_zobrist();
    uint64_t zobrist_hash(const std::vector<ttt::game::Sign>& b, int cols, int rows);
    long long evaluate_cell(const std::vector<ttt::game::Sign>& b, int cols, int rows, int wl, int cx, int cy, ttt::game::Sign player);
    long long evaluate_board(const std::vector<ttt::game::Sign>& b, int cols, int rows, int wl, ttt::game::Sign me, ttt::game::Sign opp);
    std::vector<Move> generate_moves(const std::vector<ttt::game::Sign>& b, int cols, int rows, int wl, ttt::game::Sign cur_player, ttt::game::Sign opp_player);
    ttt::game::Point find_any_free_cell(const std::vector<ttt::game::Sign>& b, int cols, int rows);
    long long alphabeta(std::vector<ttt::game::Sign>& b, int cols, int rows, int wl, int depth, long long alpha, long long beta, bool maxing, ttt::game::Sign cur, ttt::game::Sign me, ttt::game::Sign opp, std::chrono::steady_clock::time_point start, int tl, uint64_t hash);

public:
    MyPlayer(const char *name);
    void set_sign(game::Sign sign) override;
    game::Point make_move(const game::State &state) override;
    const char *get_name() const override;
    void handle_event(const game::State &state, const game::Event &event) override;
};

} // namespace ttt::my_player