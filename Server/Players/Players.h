#ifndef PLAYERS_H
#define PLAYERS_H

#include <map>
#include <string>
#include "Network.h"

class Players {
public:
    using id = unsigned long long;
public:
    Players (Network& network);
    ~Players ();
    Players (const Players& players) = delete;

public:
    void process_message (const Network_package& package); // process from network
    void process_players ();                               // process from buffer

private:
    struct Player_info {
        double x;
        double y;
        double dx;
        double dy;
        double ddx;
        double ddy;
        unsigned long long time;
    };

    struct Player {
        double x;
        double y;
        double dx;
        double dy;
        double ddx;
        double ddy;
        unsigned long long time;
        std::string ip; // TODO: add constructor and make ip and port const
        std::string port;
        std::map<id, Player_info> unprocessed;
    };

private:
    void add_player          (const std::string& client, const std::string& ip, const std::string& port); // done so to reduce string processing
    void add_player_info     (const std::string& client, const Package& message);
    void process_player_info (const std::string& client);
    void delete_player       (const std::string& client, const std::string& ip, const std::string& port);
    void get_other_players   (const std::string& client, const Network_package& package);
private:
    Network& m_network;
    std::map<std::string, Player> m_players;
    const Player_info m_start_info;
};

#endif // PLAYERS_H