#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

/// data struct for all COMMANDs from players
struct Command {
  std::string type;
  std::string player_id;
  std::string hash;
  std::string payload;
};

/// blocking RECIEVE one message
Command listen();

/// start the server
bool init_connection();

/// stop the server
void teardown_connection();

/// send a PONG message
void send_pong(std::string id, std::string payload);

/// send a ERROR message
void send_error(std::string id, std::string payload);

/// send a UPDATE message
void send_update(std::string id, std::string payload);

/// send a LEFT message
void send_list(std::string id, std::string payload);
