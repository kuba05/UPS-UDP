#pragma once

#include <chrono>
#include <string>
#include <vector>

enum GameState { Waiting, Started, Over };
enum PlayerStatus { Active, Afk, Kicked };

struct Player {
  std::string id;
  std::chrono::steady_clock::time_point last_pinged =
      std::chrono::steady_clock::now();
  PlayerStatus status = PlayerStatus::Active;
};

class Game {
public:
  Game() = default;

  bool is_started() const;
  bool is_over() const;
  int player_count() const;
  std::vector<std::string> get_all_players() const;
  std::string hash() const;

  bool play_move(const std::string &move);
  bool remove_player(const std::string &id);
  bool add_player(const std::string &id);
  bool player_active(const std::string &id);

  void send_update();

private:
  std::vector<std::string> moves_;
  GameState state_;
  std::vector<Player> players_;
};
