#include "Players.h"
#include "Network.h"
#include <iostream>
#include <ostream>
#include <syncstream>

Players::Players (Network& network) : m_network{network}, m_start_info{.x=0, .y=0, .dx=0, .dy=0, .ddx=0, .ddy=0, .time=0} {
    // TODO: add map
}

Players::~Players () {

}

void Players::process_message (const Network_package& package) {
    std::string client = package.ip + ":" + package.port;

    switch (package.package.type) {
        case Package::Type::LOGIN:
            // std::osyncstream(std::cout) << "Server adding player: " << client << "\n";
            add_player(client, package.ip, package.port);
            break;
        case Package::Type::MESSAGE:
            // std::osyncstream(std::cout) << "Server getting info: " << client << "\n";
            add_player_info(client, package.package);
            break;
        case Package::Type::BREAK_SESSION:
            // std::osyncstream(std::cout) << "Server deleting player: " << client << "\n";
            delete_player(client, package.ip, package.port);
            break;
        case Package::Type::GET_OTHER:
            std::osyncstream(std::cout) << "Server getting others: " << client << "\n";
            get_other_players(client, package);
            break;
        default:
            break;
    }
}

void Players::process_players () {
    for (const auto& player : m_players) {
        process_player_info(player.first);
    }
}

void Players::add_player (const std::string& client, const std::string& ip, const std::string& port) {
    m_network.registered_acknowledge(ip, port);

    if (m_players.find(client) != m_players.end()) return;

    m_players[client] = Player{
        .x=   m_start_info.x,     .y=  m_start_info.y,
        .dx=  m_start_info.dx,    .dy= m_start_info.dy,
        .ddx= m_start_info.ddx,   .ddy=m_start_info.ddy,
        .time=m_start_info.time,
        .ip=ip,                  .port=port,
        .unprocessed={}
    };
}

void Players::add_player_info (const std::string& client, const Package& message) {
    if (m_players.find(client) == m_players.end()) return;
    m_players[client].unprocessed[message.id] = {.x=message.x, .y=message.y, .dx=message.dx, .dy=message.dy, .ddx=message.ddx, .ddy=message.ddy, .time=message.time};
}

void Players::process_player_info (const std::string& client) {
    std::osyncstream(std::cout) << "Server evaluating for: " << client << "\n";
    auto find_player = m_players.find(client);
    if (find_player == m_players.end()) return;
    Player& player = find_player->second;

    id last_id;
    for (auto it = player.unprocessed.begin(); it != player.unprocessed.end();) { // no ++it - using erase
        // TODO: add physics check
        player.x    = it->second.x;
        player.y    = it->second.y;
        player.dx   = it->second.dx;
        player.dy   = it->second.dy;
        player.time = it->second.time;
        last_id     = it->first;

        // std::osyncstream(std::cout) << "1111111: mew mew mew " << player.x << "\n";
        m_network.acknowledge(player.ip, player.port, last_id);

        // ---
        it = player.unprocessed.erase(it);
    }
}

void Players::delete_player (const std::string& client, const std::string& ip, const std::string& port) {
    m_players.erase(client);
    m_network.deleted_acknowledge(ip, port);
}

void Players::get_other_players(const std::string& client, const Network_package& package) {
    // make package
    auto find_player = m_players.find(client);
    if (find_player == m_players.end()) return;

    // not only player
    if (m_players.size() == 1) return;


    // add data about other
    std::vector<Answer::Other> other;
    other.reserve(m_players.size() - 1);

    unsigned long long pseudo_id = 0; // TODO: make better id system
    for (const auto& other_player : m_players) {
        if (other_player.first == client) continue;
        other.push_back({
            .x   =other_player.second.x,
            .y   =other_player.second.y,
            .dx  =other_player.second.dx,
            .dy  =other_player.second.dy,
            .ddx =other_player.second.ddx,
            .ddy =other_player.second.ddy,
            .id  =pseudo_id++,
            .time=other_player.second.time,
        });
    }

    m_network.send_info_of_other(package.ip, package.port, package.package.id, other);
}