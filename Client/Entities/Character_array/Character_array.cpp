#include "Character_array.h"
#include "Network.h"
#include "Opponent.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include <cstddef>
#include <iostream>
#include <mutex>
#include <syncstream>
#include <thread>
#include <vector>

Character_array::Character_array (Player& player, Network& network, RenderWindow& window, const Map& map)
    : m_player{player}, m_network{network},  m_window{window}, m_map{map},
      m_characters{},   m_other_requests{},  m_requests{} {
        m_characters.reserve(10);
}

Character_array::~Character_array  () {
    std::optional<bool> disconected = m_network.send_disconnection();
    for (int i = 0; i < 5 and not disconected.has_value(); ++i) {
        disconected = m_network.send_disconnection();
    }

    if (not disconected.has_value()) {
        std::cerr << "Network send disconnection failed" << '\n';
    }
}

void Character_array::start_network_threads () {
    if (not m_network.is_connected()) {
        std::cerr << "Network is not connected to server" << '\n';
        exit(1);
    }
    if (m_is_network_thread_running.load()) {
        std::cerr << "Network thread is already running" << '\n';
        return;
    }

    m_is_network_thread_running.store(true);

    m_send_thread = std::jthread([&] { this->send_thread(); });
    m_receive_thread = std::jthread([&] { this->receive_thread(); });
}

void Character_array::stop_network_threads () {
m_is_network_thread_running.store(false);
}

void Character_array::update () {
    if (not m_network.is_connected()) {
        std::cerr << "Network is not connected to server - game has started (inner error)" << '\n';
        exit(1);
    }
    if (not m_is_time_running) {
        std::cerr << "Time is not started - game has started (inner error)" << '\n';
        exit(1);
    }

    std::lock_guard<std::mutex> lock(m_update_mutex); // check for ownership

    unsigned long long new_time = SDL_GetTicks();
    unsigned long long run_time = new_time - m_time_start;
    m_time_start = new_time;

    m_player.make_tick(run_time);
    m_player.make_logical_tick(run_time);
    for (auto& character : m_characters) {
        character.make_tick(run_time);
        character.make_logical_tick(run_time);
    }
}

void Character_array::draw () {
    static int RENDER_OFFSET = Entity::TILE_SIZE * 5; // 5 tiles

    const SDL_Rect& camera = m_player.get_camera();

    int left_most_x   = camera.x - RENDER_OFFSET;
    int right_most_x  = camera.x + camera.w + RENDER_OFFSET;
    int top_most_y    = camera.y - RENDER_OFFSET;
    int bottom_most_y = camera.y + camera.h + RENDER_OFFSET;

    m_window.draw_entity(&m_player, {camera.x, camera.y});
    // std::cout << "amount of nya: " << m_amount_of_opponents << '\n';
    for (size_t i = 0; i < m_amount_of_opponents; ++i) {
        Opponent& character = m_characters[i];
        bool is_x_ok = character.get_position().x >= left_most_x and character.get_position().x <= right_most_x;
        if (not is_x_ok) {
            std::osyncstream(std::cout) << "character: " << i << " is out of camera view (x)" << '\n';
            continue;
        }
        bool is_y_ok = character.get_position().y >= top_most_y  and character.get_position().y <= bottom_most_y;
        if (not is_y_ok) {
            std::osyncstream(std::cout) << "character: " << i << " is out of camera view (y)" << '\n';
            continue;
        }
        m_window.draw_entity(&character, {camera.x, camera.y});
    }
}

void Character_array::start_time () {
    m_is_time_running = true;
    m_time_start      = SDL_GetTicks();
}

void Character_array::stop_time  () {
    m_is_time_running = false;
}

void Character_array::handle_keyboard_input () {
    static bool has_pressed_up = false;

    const bool* state = SDL_GetKeyboardState(nullptr);

    std::lock_guard<std::mutex> lock(m_update_mutex); // check for ownership

    if (state[SDL_SCANCODE_A] or state[SDL_SCANCODE_LEFT]) {
        m_player.go_left();
    } else if (state[SDL_SCANCODE_D] or state[SDL_SCANCODE_RIGHT]) {
        m_player.go_right();
    } else {
        m_player.reset_x_movement();
        m_player.stop_x_movement();
    }

    if ((state[SDL_SCANCODE_W] or state[SDL_SCANCODE_UP]) or state[SDL_SCANCODE_SPACE]) {
        if (not has_pressed_up) has_pressed_up = m_player.jump();
    } else {
        has_pressed_up = false;
    }
    if (state[SDL_SCANCODE_S] or state[SDL_SCANCODE_DOWN]) {

    }
}


// ========================================================
// network functions - update buffer
// ========================================================

void Character_array::update_opponents_amount () {
    if (m_characters.size() > m_amount_of_opponents) return; // TODO: maybe add algo for removing opponents

    for (size_t i = 0; i < m_amount_of_opponents - m_characters.size(); ++i) {
        SDL_Texture* oponent_texture = m_window.load_texture(Opponent::INFO.texture_path); // TODO: maybe add algo for removing opponents (could influence this line - or its results)
        m_characters.emplace_back(Opponent{oponent_texture, m_map.get_player_start_position(), m_map});
    }
}


void Character_array::update_get_other_requests (id_game_t processed_id) {
    if (m_other_requests.empty()) return;

    for (auto it = m_other_requests.begin(); it != m_other_requests.end();) {
        if (*it <= processed_id) {
            m_requests.erase(*it);
            it = m_other_requests.erase(it);
        } else {
            ++it;
        }
    }
}



// ========================================================
// network functions
// ========================================================

void Character_array::send_thread() {
    while (m_is_network_thread_running.load()) {
        send_current_position();
        m_network.send_ask_other();
        std::this_thread::sleep_for(std::chrono::milliseconds(SEND_TIME_MS));
    }
}

void Character_array::receive_thread () {
    std::this_thread::sleep_for(std::chrono::milliseconds(SEND_TIME_MS * 2)); // skip one send
    while (m_is_network_thread_running.load()) {
            std::optional<Answer> answer = m_network.get_answer();
            if (not answer.has_value()) continue;
            process_answer(answer.value());
            // std::this_thread::sleep_for(std::chrono::milliseconds(RECEIVE_TIME_MS));
    }
}

void Character_array::process_answer (Answer& answer) {
    switch (answer.type) {
    case Answer::Type::ACKNOWLEDGE:
        update_player(answer);
        break;
    case Answer::Type::OTHER:
        update_opponents(answer);
        break;
    case Answer::Type::FINISH:
        finish_process_response(answer);
        break;
    case Answer::Type::BREAK_SESSION:
        std::cerr << "Disconnected from server" << '\n';
        exit(1);
        break;
    case Answer::Type::ERROR_VALUE_INCORRECT:
        rollback(answer);
        break;
    default:
        std::cerr << "Unknown/unused answer type: " << static_cast<int>(answer.type) << '\n';
    }
}

void Character_array::update_opponents (Answer& answer) {
    std::lock_guard<std::mutex> lock(m_update_mutex); // check for ownership

    m_amount_of_opponents = answer.other->size();
    update_opponents_amount();

    if (not answer.other.has_value()) {
        std::cerr << "Inconsistent network answer - no other players\n";
        return;
    }

    // update data

    std::vector<Answer::Other>& other = answer.other.value();
    for (size_t i = 0; i < m_amount_of_opponents; ++i) {
        Answer::Other_Payload& info = other[i].payload;
        // m_characters[i].update_position({static_cast<float>(info.x), static_cast<float>(info.y)});
        // m_characters[i].update_speed({static_cast<float>(info.dx), static_cast<float>(info.dy)});
        // m_characters[i].update_velocity({static_cast<float>(info.ddx), static_cast<float>(info.ddy)});
        m_characters[i].set_next_state(info);
    }

    // update buffer
    if (not answer.id.has_value()) {
        std::cerr << "Inconsistent network answer - no id\n";
        return;
    }
    update_get_other_requests(answer.id.value());
}

void Character_array::update_player (Answer& answer) {
    if (not answer.id.has_value()) {
        std::cerr << "Inconsistent network answer - no id\n";
        return;
    }
    m_requests.erase(answer.id.value());
}

void Character_array::finish_process_response (Answer& answer) {
    std::osyncstream(std::cout) << "finish\n";
}

void Character_array::rollback (Answer& answer) {
    std::osyncstream(std::cout) << "rollback\n";
}

void Character_array::send_current_position () {
    // get info

    SDL_Point  position   = m_player.get_position();
    SDL_FPoint position_f = {static_cast<float>(position.x), static_cast<float>(position.y)};

    SDL_FPoint speed      = m_player.get_speed();

    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    unsigned long long ticks_time = static_cast<unsigned long long>(std::max(milliseconds,  0ll));

    // make package

    Package::Package_Payload payload;
    payload.x    = position_f.x;
    payload.y    = position_f.y;
    payload.time = ticks_time;
    payload.dx   = speed.x;
    payload.dy   = speed.y;

    // send
    m_network.send_message(payload);
}
