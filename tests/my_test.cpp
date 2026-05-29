#include "../src/player/my_player.hpp"
#include <iostream>

int main() {
    ttt::my_player::MyPlayer bot;
    ttt::game::State state(10, 10);
    
    // Проверка: бот должен вернуть ход
    ttt::Point move = bot.make_move(state);
    std::cout << "Bot moved to: " << move.x << ", " << move.y << std::endl;
    
    return 0;
}