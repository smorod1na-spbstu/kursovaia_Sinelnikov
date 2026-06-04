#include "player/my_player.hpp"
#include "core/baseline.hpp"
#include "test_stats.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[]) {
    std::cout << "Testing MyPlayer vs baseline easy player\n";
    if (argc >= 2) {
        std::srand(atoi(argv[1]));
    }

    ttt::my_player::MyPlayer p1("MyPlayer");
    ttt::game::IPlayer *p2 = ttt::baseline::get_harder_player("BaselineHard");

    auto result = ttt::test::run_game_tests(p1, *p2, 100);
    
    ttt::test::print_test_results(result, "MyPlayer", "BaselineHard");
    
    delete p2;
    return 0;
}