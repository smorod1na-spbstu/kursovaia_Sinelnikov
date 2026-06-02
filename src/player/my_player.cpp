#include "my_player.hpp"
#include <vector>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <fstream>

namespace ttt::my_player {

using Board = std::vector<ttt::game::Sign>;

// ================= ЛОГГЕР =================
void MyPlayer::set_sign(ttt::game::Sign sign) { m_sign = sign; }
const char *MyPlayer::get_name() const { return m_name; }

void MyPlayer::handle_event(const ttt::game::State &state, const ttt::game::Event &) {
    std::ofstream log_file("all_games_moves.txt", std::ios::app);
    if (!log_file.is_open()) return;

    int cols = state.get_opts().cols;
    int rows = state.get_opts().rows;
    int pieces = 0;
    Board current_board(rows * cols, ttt::game::Sign::NONE);
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            ttt::game::Sign s = state.get_value(x, y);
            current_board[y * cols + x] = s;
            if (s != ttt::game::Sign::NONE) pieces++;
        }
    }

    static int last_pieces_count = -1;
    static int local_game_id = 0;
    static Board last_known_board;

    if (pieces == 0 && last_pieces_count != 0) {
        local_game_id++;
        log_file << "\n=========================================\n";
        log_file << "ИГРА #" << local_game_id << " НАЧАЛАСЬ\n";
        log_file << "=========================================\n";
        last_pieces_count = 0;
        last_known_board.assign(rows * cols, ttt::game::Sign::NONE);
    }

    if (pieces > last_pieces_count && pieces > 0) {
        if (last_known_board.size() != static_cast<size_t>(rows * cols)) {
            last_known_board.assign(rows * cols, ttt::game::Sign::NONE);
        }
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                int idx = y * cols + x;
                if (current_board[idx] != ttt::game::Sign::NONE && last_known_board[idx] == ttt::game::Sign::NONE) {
                    char sign_char = (current_board[idx] == ttt::game::Sign::X) ? 'X' : 'O';
                    log_file << "Ход " << pieces << ": Игрок [" << sign_char << "] сходил на (" << x << ", " << y << ")\n";
                    last_known_board[idx] = current_board[idx];
                }
            }
        }
        last_pieces_count = pieces;
    }

    if (state.get_status() == ttt::game::Status::ENDED && last_pieces_count != -2) {
        log_file << "Игра завершена. Результат: ";
        if (state.get_winner() == ttt::game::Sign::X) log_file << "Победил X\n";
        else if (state.get_winner() == ttt::game::Sign::O) log_file << "Победил O\n";
        else log_file << "НИЧЬЯ\n";
        log_file << "-----------------------------------------\n";
        last_pieces_count = -2; 
    }
}

// ================= ХЭШИРОВАНИЕ И ТАБЛИЦЫ =================
static int history_table[30][30];
static uint64_t ZOBRIST_TABLE[30*30*3];
static bool zobrist_init = false;

struct TTEntry { long long score; int depth; };
static std::unordered_map<uint64_t, TTEntry> transposition_table;

void init_zobrist() {
    if (zobrist_init) return;
    uint64_t seed = 9876543210ULL;
    for (int i = 0; i < 30*30*3; ++i) {
        seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17;
        ZOBRIST_TABLE[i] = seed;
    }
    zobrist_init = true;
}

uint64_t zobrist_hash(const Board& b, int cols, int rows) {
    uint64_t h = 0;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            ttt::game::Sign s = b[y*cols+x];
            if (s == ttt::game::Sign::X || s == ttt::game::Sign::O)
                h ^= ZOBRIST_TABLE[(y*cols + x)*3 + (s == ttt::game::Sign::X ? 1 : 2)];
        }
    }
    return h;
}

// ================= ЭКСПЕРТНАЯ ЭВРИСТИКА (С ДИНАМИЧЕСКИМ WIN_LEN) =================

long long evaluate_cell(const Board& b, int cols, int rows, int wl, int cx, int cy, ttt::game::Sign player) {
    long long score = 0;
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (auto d : dirs) {
        int dx = d[0], dy = d[1];
        for (int offset = -(wl - 1); offset <= 0; ++offset) {
            int start_x = cx + offset * dx;
            int start_y = cy + offset * dy;
            int end_x = start_x + (wl - 1) * dx;
            int end_y = start_y + (wl - 1) * dy;

            if (start_x >= 0 && end_x >= 0 && start_x < cols && end_x < cols &&
                start_y >= 0 && end_y >= 0 && start_y < rows && end_y < rows) {

                int count = 0;
                bool blocked = false;
                for (int i = 0; i < wl; ++i) {
                    int px = start_x + i * dx;
                    int py = start_y + i * dy;
                    ttt::game::Sign s = b[py * cols + px];
                    
                    if (px == cx && py == cy) count++; 
                    else if (s == player) count++;
                    else if (s != ttt::game::Sign::NONE) { blocked = true; break; }
                }
                
                if (!blocked) {
                    // Динамическая оценка: защита от ухода в минус при малом wl
                    if (count == wl) score += 100000000LL;       // Win
                    else if (count == wl - 1 && count > 0) score += 1000000LL; // 1 шаг до победы
                    else if (count == wl - 2 && count > 0) score += 50000LL;   // 2 шага до победы
                    else if (count == wl - 3 && count > 0) score += 1000LL;    // 3 шага до победы
                    else if (count == 1) score += 10LL;           // Просто камень
                }
            }
        }
    }
    return score;
}

long long evaluate_board(const Board& b, int cols, int rows, int wl, ttt::game::Sign me, ttt::game::Sign opp) {
    long long my_score = 0;
    long long opp_score = 0;
    const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            for (int d = 0; d < 4; ++d) {
                int dx = dirs[d][0], dy = dirs[d][1];
                int end_x = x + (wl - 1) * dx;
                int end_y = y + (wl - 1) * dy;
                if (end_x < 0 || end_x >= cols || end_y < 0 || end_y >= rows) continue;

                int me_count = 0, opp_count = 0;
                for (int i = 0; i < wl; ++i) {
                    ttt::game::Sign s = b[(y + i * dy) * cols + (x + i * dx)];
                    if (s == me) me_count++;
                    else if (s == opp) opp_count++;
                }

                if (opp_count == 0 && me_count > 0) {
                    if (me_count == wl) my_score += 100000000LL;
                    else if (me_count == wl - 1 && me_count > 0) my_score += 1000000LL;
                    else if (me_count == wl - 2 && me_count > 0) my_score += 50000LL;
                    else if (me_count == wl - 3 && me_count > 0) my_score += 1000LL;
                } else if (me_count == 0 && opp_count > 0) {
                    if (opp_count == wl) opp_score += 100000000LL;
                    else if (opp_count == wl - 1 && opp_count > 0) opp_score += 1000000LL;
                    else if (opp_count == wl - 2 && opp_count > 0) opp_score += 50000LL;
                    else if (opp_count == wl - 3 && opp_count > 0) opp_score += 1000LL;
                }
            }
        }
    }
    return (my_score * 12) / 10 - opp_score; 
}

struct Move { ttt::game::Point p; long long score; };

std::vector<Move> generate_moves(const Board& b, int cols, int rows, int wl, ttt::game::Sign cur_player, ttt::game::Sign opp_player) {
    std::vector<Move> moves;
    int center_x = cols / 2, center_y = rows / 2;
    
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            if (b[y * cols + x] != ttt::game::Sign::NONE) continue;
            
            bool near = false;
            for (int dy = -2; dy <= 2 && !near; ++dy) {
                for (int dx = -2; dx <= 2 && !near; ++dx) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && nx < cols && ny >= 0 && ny < rows && b[ny * cols + nx] != ttt::game::Sign::NONE) {
                        near = true;
                    }
                }
            }
            if (!near) continue;
            
            long long my_val = evaluate_cell(b, cols, rows, wl, x, y, cur_player);
            long long opp_val = evaluate_cell(b, cols, rows, wl, x, y, opp_player);
            
            long long score = (my_val * 12) / 10 + opp_val; 
            int dist_to_center = std::abs(x - center_x) + std::abs(y - center_y);
            score += std::max(0, 20 - dist_to_center); 
            
            // Каскад работает идеально для любого wl
            if (my_val >= 100000000LL) score += 10000000000LL;       // Мат в 1 ход
            else if (opp_val >= 100000000LL) score += 5000000000LL;  // Блок мата врага
            else if (my_val >= 2000000LL) score += 2000000000LL;     // Открытая линия длины wl-1
            else if (opp_val >= 2000000LL) score += 1000000000LL;    // Блок открытой линии длины wl-1 врага
            else if (my_val >= 1000000LL) score += 500000000LL;      // Закрытая линия длины wl-1 
            else if (opp_val >= 1000000LL) score += 250000000LL;     // Блок закрытой wl-1 врага
            else if (my_val >= 150000LL) score += 100000000LL;       // Открытая линия длины wl-2
            else if (opp_val >= 150000LL) score += 50000000LL;       // Блок открытой wl-2
            
            moves.push_back({{x, y}, score});
        }
    }
    
    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) { return a.score > b.score; });
    return moves;
}

ttt::game::Point find_any_free_cell(const Board& b, int cols, int rows) {
    int cx = cols/2, cy = rows/2;
    for (int r = 0; r < std::max(cols, rows); ++r)
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx) {
                int x = cx+dx, y = cy+dy;
                if (x>=0&&x<cols&&y>=0&&y<rows && b[y*cols+x] == ttt::game::Sign::NONE) return {x, y};
            }
    return {-1, -1};
}

// ================= ALPHA-BETA ПОИСК =================

long long alphabeta(Board& b, int cols, int rows, int wl, int depth, long long alpha, long long beta, 
                    bool maxing, ttt::game::Sign cur, ttt::game::Sign me, ttt::game::Sign opp,
                    std::chrono::steady_clock::time_point start, int tl, uint64_t hash) {
    
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now-start).count() > tl) 
        return evaluate_board(b, cols, rows, wl, me, opp);
    
    auto it = transposition_table.find(hash);
    if (it != transposition_table.end() && it->second.depth >= depth) return it->second.score;
    
    if (depth == 0) {
        long long sc = evaluate_board(b, cols, rows, wl, me, opp);
        transposition_table[hash] = {sc, 0}; 
        return sc;
    }
    
    ttt::game::Sign other = (cur == me) ? opp : me;
    auto moves = generate_moves(b, cols, rows, wl, cur, other);
    
    if (moves.empty()) return evaluate_board(b, cols, rows, wl, me, opp);
    
    if (moves[0].score >= 10000000000LL) {
        return maxing ? 1000000000000LL + depth : -1000000000000LL - depth;
    }
    
    if (moves[0].score >= 5000000000LL) {
        int keep = 0;
        for (auto& m : moves) if (m.score >= 5000000000LL) keep++;
        moves.resize(keep);
    } else {
        std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
            long long scoreA = a.score + history_table[a.p.y][a.p.x];
            long long scoreB = b.score + history_table[b.p.y][b.p.x];
            return scoreA > scoreB;
        });
        int limit = (depth >= 3) ? 14 : 8; 
        if ((int)moves.size() > limit) moves.resize(limit);
    }
    
    long long best_score = maxing ? -2000000000000LL : 2000000000000LL;
    
    for (const auto& mv : moves) {
        b[mv.p.y*cols+mv.p.x] = cur;
        uint64_t new_hash = hash ^ ZOBRIST_TABLE[(mv.p.y*cols + mv.p.x)*3 + (cur == ttt::game::Sign::X ? 1 : 2)];
        
        long long ev = alphabeta(b, cols, rows, wl, depth-1, alpha, beta, !maxing, other, me, opp, start, tl, new_hash);
        
        b[mv.p.y*cols+mv.p.x] = ttt::game::Sign::NONE;
        
        if (maxing) { 
            best_score = std::max(best_score, ev); 
            alpha = std::max(alpha, ev); 
        } else { 
            best_score = std::min(best_score, ev); 
            beta = std::min(beta, ev); 
        }
        
        if (beta <= alpha) { 
            history_table[mv.p.y][mv.p.x] += depth * depth; 
            break; 
        }
    }
    
    transposition_table[hash] = {best_score, depth};
    return best_score;
}

// ================= ГЛАВНЫЙ МЕТОД ХОДА =================

ttt::game::Point MyPlayer::make_move(const ttt::game::State &state) {
    init_zobrist();
    int cols = state.get_opts().cols;
    int rows = state.get_opts().rows;
    
    // Синхронизируем нашу внутреннюю переменную с настройками движка перед каждым ходом
    m_win_len = state.get_opts().win_len; 
    
    Board board(rows * cols, ttt::game::Sign::NONE);
    int stone_count = 0;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            board[y*cols+x] = state.get_value(x, y);
            if (board[y*cols+x] != ttt::game::Sign::NONE) stone_count++;
        }
    }
    
    ttt::game::Sign me = m_sign, opp = (me == ttt::game::Sign::X) ? ttt::game::Sign::O : ttt::game::Sign::X;
    
    if (stone_count == 0) return {cols/2, rows/2}; 
    
    auto smart_moves = generate_moves(board, cols, rows, m_win_len, me, opp);
    if (smart_moves.empty()) return find_any_free_cell(board, cols, rows);
    
    if (smart_moves[0].score >= 10000000000LL) return smart_moves[0].p;
    
    if (smart_moves[0].score >= 5000000000LL) {
        int keep = 0;
        for (auto& m : smart_moves) if (m.score >= 5000000000LL) keep++;
        smart_moves.resize(keep);
    } else {
        int limit = std::min((int)smart_moves.size(), 14);
        smart_moves.resize(limit);
    }
    
    if (stone_count % 5 == 0) {
        transposition_table.clear();
        memset(history_table, 0, sizeof(history_table));
    }
    
    auto start_time = std::chrono::steady_clock::now();
    ttt::game::Point best = smart_moves[0].p;
    uint64_t root_hash = zobrist_hash(board, cols, rows);
    
    for (int d = 2; d <= 8; d++) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start_time).count() > 85) break;
        
        std::vector<std::pair<long long, ttt::game::Point>> cur_sc;
        bool timeout = false; 
        long long best_score = -2000000000000LL;
        
        for (const auto& mv : smart_moves) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start_time).count() > 85) { 
                timeout = true; break; 
            }
            
            board[mv.p.y*cols+mv.p.x] = me;
            long long s = alphabeta(board, cols, rows, m_win_len, d-1, -2000000000000LL, 2000000000000LL, false, opp, me, opp, start_time, 90, root_hash ^ ZOBRIST_TABLE[(mv.p.y*cols+mv.p.x)*3 + (me==ttt::game::Sign::X?1:2)]);
            board[mv.p.y*cols+mv.p.x] = ttt::game::Sign::NONE;
            
            cur_sc.push_back({s, mv.p});
            if (s > best_score) { best_score = s; best = mv.p; }
        }
        
        if (timeout) break;
        
        std::sort(cur_sc.begin(), cur_sc.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        smart_moves.clear(); 
        for (const auto& cs : cur_sc) smart_moves.push_back({cs.second, cs.first});
    }
    
    return best;
}

} // namespace ttt::my_player