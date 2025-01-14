#ifndef CHARACTER_ARRAY_H
#define CHARACTER_ARRAY_H

#include "Player.h"
#include "Opponent.h"
#include "Network.h"
#include "Movable.h"
#include "Map.h"
#include "RenderWindow.h"
#include <list>
#include <thread>

/**
 * @brief Using for:
 *        1. encapsulation of update and draw functions for all characters
 *        2. encapsulation of network thread - [updates from]/[sending of data to]/[acknowledging by] server
 *        3. contains inner timer for update
 *        4. processes both network and keyboard input
 */
class Character_array {
    public:
        Character_array (Player& player, Network& network, RenderWindow& window, const Map& map);
        ~Character_array ();

        /**
         * @brief network should be connected to server (registered) before call
         *
         */
        void start_network_threads ();
        void stop_network_threads  ();

        /**
         * @brief Updates all characters - contains inner timer - need to be started
         */
        void update                ();
        void draw                  ();
        void start_time            ();
        void stop_time             ();
        void handle_keyboard_input ();

    private:
        // main threads
        void send_thread    ();
        void receive_thread ();

        // network functions
        void process_answer          (Answer& answer);
        void update_opponents        (Answer& answer);
        void update_player           (Answer& answer);
        void rollback                (Answer& answer);
        void finish_process_response (Answer& answer);

        // network additional endpoints (beside network's ones)
        void send_current_position   ();

        // update buffer
        void update_opponents_amount   ();
        void update_get_other_requests (id_game_t processed_id);

    private:
        Player&               m_player;
        Network&              m_network;
        RenderWindow&         m_window;
        const Map&            m_map;
        std::vector<Opponent> m_characters; // id doesn't matter here so we can use n amount of opponents while n >= amount of opponents on server (and then just increase if needed)
        size_t                m_amount_of_opponents = 0;

        std::list<id_game_t>         m_other_requests;
        std::map<id_game_t, Package> m_requests;

        std::mutex         m_update_mutex;
        std::atomic<bool>  m_is_network_thread_running = false;

        bool               m_is_time_running = false;
        unsigned long long m_time_start      = 0.0f;

        std::jthread       m_send_thread;
        std::jthread       m_receive_thread;

        unsigned long long SEND_TIME_MS    = 100;
        unsigned long long RECEIVE_TIME_MS = 100;

};

#endif // CHARACTER_ARRAY_H