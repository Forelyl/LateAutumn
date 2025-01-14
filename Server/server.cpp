#include <chrono>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include "Network.h"
#include "Players.h"
#include "Players/Players.h"
#include <thread>
#include <syncstream>
#include <iostream>


/*
    while (network.is_server_running()) {
        // Frequently check if a message has arrived
        if (network.has_message()) {
            // Handle the message
            network.get_message();
        } else {
            // Yield to allow other threads to work while waiting
            std::this_thread::yield();
        }

        // If it's time to process players' packages
        if (std::chrono::steady_clock::now() >= next_process_time) {
            // Perform player package processing
            // Replace this with your actual player processing logic
            // e.g., players.process_packages();
            std::cout << "Processing player packages..." << std::endl;

            // Schedule the next processing time
            next_process_time += process_interval;
*/


void thread_reader_main (Network& network, Players& players) {
    const int MAX_PACKAGES_PER_TIME = 10;
    const std::chrono::microseconds PROCESS_INTERVAL = std::chrono::milliseconds(25); // Interval to process messages
    auto next_process_time = std::chrono::steady_clock::now() + PROCESS_INTERVAL;

    while (network.is_server_running()) {
        // try to get message (spinlock)
        while (not network.has_message()) {
            std::this_thread::yield();
        }

        // wait for min time to process messages
        std::this_thread::sleep_until(next_process_time);
        next_process_time = std::chrono::steady_clock::now() + PROCESS_INTERVAL;

        // get messages
        for (int i = 0; i < MAX_PACKAGES_PER_TIME and network.has_message(); ++i) {
            auto package = network.pop_message();
            if (not package.has_value() or package->package.type == Package::Type::EMPTY) {
                continue;
            }
            players.process_message(*package);
            // std::osyncstream(std::cout) << "Client: " << package->ip << ":" << package->port << "\n"
            //                             << "Type: " << int(package->package.type) << "\n" << std::flush;
        }

        // process messages
        std::osyncstream(std::cout) << "Processing player packages..." << '\n';
        players.process_players();
    }

}

int main () {
    Network network;
    Players players(network);
    std::thread t1(&Network::socket_main, &network);
    std::thread t2(&thread_reader_main, std::ref(network), std::ref(players));
    // network.socket_main();
    t1.join();
    t2.join();
    return 0;
}