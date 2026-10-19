
#include "comms.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <regex>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>

static int g_sockfd = -1;

static const uint16_t PORT = 8080;
static const std::regex COMMAND_REGEX(R"(([^|]*)\|([^|]*)\|([^|]*)\|(.*)\n?)");

std::unordered_map<std::string, sockaddr_in> player_target;

void send_msg(const std::string &id, const std::string &msg) {
  if (g_sockfd < 0)
    return;

  auto msg2 = msg + "\n";
  auto target = player_target[id];

  ssize_t sent =
      sendto(g_sockfd, msg2.data(), msg2.size(), 0,
             reinterpret_cast<const sockaddr *>(&target), sizeof(target));

  if (sent < 0) {
    std::cerr << "[ERROR] sendto failed: " << strerror(errno) << '\n';
  }
}

std::pair<std::string, sockaddr_in> recieve_raw_msg() {
  sockaddr_in sender{};
  socklen_t sender_len = sizeof(sender);
  char buffer[4096];

  ssize_t bytes_read =
      recvfrom(g_sockfd, buffer, sizeof(buffer) - 1, 0,
               reinterpret_cast<sockaddr *>(&sender), &sender_len);

  if (bytes_read < 0) {
    return {"", sender};
  }

  buffer[bytes_read] = '\0';
  return {std::string(buffer, bytes_read), sender};
}

Command listen() {
  while (true) {
    auto [msg, sender] = recieve_raw_msg();
    if (msg.empty())
      continue;

    std::smatch match;
    if (!std::regex_match(msg, match, COMMAND_REGEX)) {
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(sender.sin_addr), ip, INET_ADDRSTRLEN);
      std::cerr << "[INVALID_MSG] From " << ip << ":" << ntohs(sender.sin_port)
                << " -> \"" << msg << "\"\n";
      continue;
    }

    std::string command = match[1].str();
    std::string player_id = match[2].str();
    std::string hash = match[3].str();
    std::string payload = match[4].str();

    player_target[player_id] = sender;
    return Command{command, player_id, hash, payload};
  }
}

bool init_connection() {
  g_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (g_sockfd < 0) {
    std::cerr << "[FATAL] Socket creation failed\n";
    return false;
  }

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(g_sockfd, reinterpret_cast<const sockaddr *>(&server_addr),
           sizeof(server_addr)) < 0) {
    std::cerr << "[FATAL] Bind failed on port " << PORT << '\n';
    close(g_sockfd);
    return false;
  }

  std::cout << "[INFO] UDP server listening on port " << PORT << "...\n";
  return true;
}

void teardown_connection() { close(g_sockfd); }

void send_pong(std::string id, std::string payload) {
  std::cout << "[INFO] sending poong to " << id << " with payload " << payload
            << "\n";
  send_msg(id, std::format("PONG|{}|{}", id, payload));
}

void send_error(std::string id, std::string payload) {
  std::cout << "[INFO] sending error to " << id << " with payload " << payload
            << "\n";
  send_msg(id, std::format("ERROR|{}|{}", id, payload));
}

void send_update(std::string id, std::string payload) {
  std::cout << "[INFO] sending update to " << id << " with payload " << payload
            << "\n";
  send_msg(id, std::format("UPDATE|{}|{}", id, payload));
}

void send_list(std::string id, std::string payload) {
  std::cout << "[INFO] sending list to " << id << " with payload " << payload
            << "\n";
  send_msg(id, std::format("LIST|{}|{}", id, payload));
}
