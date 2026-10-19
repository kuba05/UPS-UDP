#include "game.hpp"
#include "comms.hpp"
#include <algorithm>
#include <ranges>

bool Game::play_move(const std::string &move) {
  if (state_ == GameState::Waiting && move == "START") {
    state_ = GameState::Started;
    return true;
  }
  if (state_ != GameState::Started)
    return false;

  moves_.push_back(move);
  return true;
}

bool Game::is_over() const { return state_ == GameState::Over; }
bool Game::is_started() const { return state_ != GameState::Waiting; }
int Game::player_count() const { return players_.size(); }

std::vector<std::string> Game::get_all_players() const {
  return players_ | std::views::transform(&Player::id) |
         std::ranges::to<std::vector>();
}

bool Game::remove_player(const std::string &id) {

  if (state_ == GameState::Over) {
    return false;
  }

  return std::erase_if(players_, [&](const Player &p) { return p.id == id; }) >
         0;
}

bool Game::add_player(const std::string &id) {

  if (state_ != GameState::Waiting) {
    return false;
  }
  bool already_exists = std::ranges::any_of(
      players_, [&](const Player &p) { return p.id == id; });

  if (already_exists) {
    return false;
  }

  players_.push_back(Player{id});
  return true;
}

bool Game::player_active(const std::string &id) {

  auto it = std::ranges::find_if(players_,
                                 [&](const Player &p) { return p.id == id; });

  // TODO: check for afk players

  if (it == players_.end()) {
    return false; // Player not found
  }

  it->last_pinged = std::chrono::steady_clock::now();
  return true;
}

void Game::send_update() {
  for (const auto &player : players_) {
    ::send_update(player.id, hash());
  }
}

std::string Game::hash() const {
  if (moves_.empty())
    return "";
  return moves_ | std::views::join_with(',') | std::ranges::to<std::string>();
}
