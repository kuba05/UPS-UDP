#include "comms.hpp"
#include "game.hpp"
#include <arpa/inet.h>
#include <format>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <ranges>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
std::unordered_map<std::string, std::shared_ptr<Game>> player_game;
std::unordered_map<std::string, std::shared_ptr<Game>> lobbys;
#include <random>
#include <string>
#include <string_view>

const int LOBBY_SIZE = 10;
const int KEY_SIZE = 12;

/***********
 * HELPERS *
 ***********/
std::string generate_random_string(std::size_t length) {
  static constexpr std::string_view charset = "abcdefghijklmnopqrstuvwxyz"
                                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  // thread_local avoids re-seeding and remains thread-safe in a server
  thread_local static std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<std::size_t> dist(0, charset.size() - 1);

  std::string result;
  result.reserve(length);

  for (std::size_t i = 0; i < length; ++i) {
    result += charset[dist(rng)];
  }

  return result;
}

/// a helper to get either pointer to game or nullptr
std::shared_ptr<Game> get_game(const std::string &key) {
  auto it = player_game.find(key);
  if (it == player_game.end()) {
    return nullptr;
  }
  return std::ref(it->second);
}

/// Check a hash corresponds to the game player is part of.
bool check_hash(std::string player_id, std::string hash) {
  auto game = get_game(player_id);
  std::cout << "[INFO] Checking hash " << hash << " ";
  bool output;
  if (game) {
    std::cout << "Game found ";
    output = hash == game->hash();
  } else {
    std::cout << "No game found ";
    output = hash == "NO_GAME";
  }
  std::cout << "result: " << output << "\n";
  return output;
}

/// Send update to the game player is part of.
void send_update(std::string player_id) {
  auto game = get_game(player_id);
  if (game) {
    game->send_update();
  } else {
    send_update(player_id, "NO_GAME");
  }
}

/********************
 * COMMAND HANDLERS *
 ********************/

void handle_ping(Command command) {
  /*
   * The payload is only for the client to identify pongs and can therefore be
   * anything.
   */

  // always pong back
  send_pong(command.player_id, command.hash);
}

void handle_move(Command command) {
  /*
   * Payload contains the move.
   */

  auto game = get_game(command.player_id);
  if (!game) {
    send_error(command.player_id, "You are not part of any game!");
    return;
  }

  if (game->play_move(command.payload)) {
    // valid move
    game->send_update();

    // game is started yet is still in lobbys
    if (game->is_started()) {
      std::erase_if(lobbys,
                    [game](const auto &pair) { return pair.second == game; });
    }

  } else {
    // invalid move
    send_error(command.player_id, "Invalid move");
  }
}

void handle_join(Command command) {
  /*
   * Payload is the ID of lobby we want to join.
   */
  if (get_game(command.player_id)) {
    send_error(command.player_id, "You are already in game!");
    return;
  }

  auto lobby = lobbys.find(command.payload);
  if (lobby == lobbys.end()) {
    send_error(command.player_id, "Not a valid lobby!");
    return;
  }

  auto game = lobby->second;

  if (game->add_player(command.player_id)) {
    player_game[command.player_id] = game;
    game->send_update();
  } else {
    send_error(command.player_id, "Cannot join game!");
  }
}

void handle_leave(Command command) {
  /*
   * Payload is to be ignored.
   */

  auto game = get_game(command.player_id);
  if (!game) {
    send_error(command.player_id, "You are not in a game!");
  }

  if (game->remove_player(command.player_id)) {
    player_game.erase(command.player_id);
    send_update(command.player_id);
    game->send_update();

  } else {
    send_error(command.player_id, "Cannot be removed from game!");
  }
}

void handle_list(Command command) {
  /*
   * Payload is to be ignored.
   */

  // oneliner goes brrr
  send_list(command.player_id,
            lobbys | std::views::transform([](const auto &pair) {
              return std::format("{}:{}", pair.first,
                                 pair.second->player_count());
            }) | std::views::join_with(';') |
                std::ranges::to<std::string>());
};

// Handle a general command
void handle_command(Command command) {
  std::cout << "recieved TYPE:" << command.type << " ID:" << command.player_id
            << " HASH:" << command.hash << " P:" << command.payload << "\n";

  bool handled = false;

  // COMMANDS TO ALWAYS HANDLE
  if (command.type == "PING") {
    handle_ping(command);
    handled = true;
  }

  if (command.type == "LIST") {
    handle_list(command);
    handled = true;
  }

  // If player things they are in a wrong position, correct them
  if (!check_hash(command.player_id, command.hash)) {
    send_update(command.player_id);
  } else {

    // COMMANDS TO HANDLE ONLY IF HASH IS OK *
    if (command.type == "MOVE") {

      handle_move(command);
      handled = true;
    }
    if (command.type == "JOIN") {
      handle_join(command);
      handled = true;
    }
    if (command.type == "LEAVE") {
      handle_leave(command);
      handled = true;
    }
  }
  if (!handled)
    send_error(command.player_id, "Unknown command");
}

void create_lobbys() {
  while (lobbys.size() < LOBBY_SIZE) {
    auto key = generate_random_string(KEY_SIZE);
    lobbys.emplace(key, std::make_shared<Game>());
  }
}

int main() {
  if (!init_connection())
    return 1;

  while (true) {
    create_lobbys();
    handle_command(listen());
  }

  teardown_connection();
}
