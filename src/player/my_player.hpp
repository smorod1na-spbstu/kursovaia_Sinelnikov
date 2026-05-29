#pragma once

#include "core/game.hpp"

namespace ttt::my_player {

using game::Event;
using game::IPlayer;
using game::Point;
using game::Sign;
using game::State;

class MyPlayer : public IPlayer {
public:
    MyPlayer(const char *name) : m_sign(Sign::NONE), m_name(name) {}
    ~MyPlayer() override = default;

    void set_sign(Sign sign) override;
    Point make_move(const State &state) override;
    const char *get_name() const override;
    void handle_event(const State &state, const Event &event) override;

private:
    Sign m_sign;
    const char *m_name;
};

} // namespace ttt::my_player
