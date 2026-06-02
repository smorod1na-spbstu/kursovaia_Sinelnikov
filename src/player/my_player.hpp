#pragma once

#include "core/game.hpp"
#include <chrono>

namespace ttt::my_player {

class MyPlayer : public game::IPlayer {
private:
    game::Sign m_sign = game::Sign::NONE;
    const char *m_name;
    int m_win_len = 5; // <--- Наша новая переменная для длины победы

public:
    MyPlayer(const char *name) : m_name(name) {}
    void set_sign(game::Sign sign) override;
    game::Point make_move(const game::State &state) override;
    const char *get_name() const override;
    void handle_event(const game::State &state, const game::Event &event) override;
};

} // namespace ttt::my_player