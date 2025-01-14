#include "Opponent.h"
#include "Movable.h"
#include "SDL3/SDL_rect.h"
#include <cmath>
#include <iostream>
// #include <mutex>
#include <optional>
#include <limits>
#include <syncstream>

Opponent::Opponent(SDL_Texture *texture, SDL_FPoint tile_position, const Map &map)
    : Movable_entity(
          INFO,
          texture,
          POSITION_OFFSET,
          &ANIMATION_DATA_MAP,
          FRAME_WIDTH,
          FRAME_HEIGHT,
          tile_position,
          map
      ) {}


bool Opponent::has_won () const {
    return m_map.is_win(
        size_t(m_position.x + m_position_offset.x) / TILE_SIZE, // integer division
        size_t(m_position.y + m_position_offset.y) / TILE_SIZE  // integer division
    );

}
void Opponent::make_tick (unsigned long long ticks_time) {
   return Movable_entity::make_tick(ticks_time);
}

std::optional<float> Opponent::make_logical_tick (unsigned long long ticks_time) {

    std::optional<float> result = Movable_entity::make_logical_tick(ticks_time);
    // ---
    // std::lock_guard<std::mutex> lock(m_state_mutex);
    unsigned long long span_time = m_state_next.time - m_state_current.time;
    if (span_time < 10 * 1000) {
        float t = static_cast<float>(ticks_time) / static_cast<float>(span_time);
        std::osyncstream(std::cout) << "span_time: " << span_time << '\n';
        std::osyncstream(std::cout) << "t: " << t << '\n';
        t = 0;

        m_true_position.x = (1 - t) * m_true_position.x + t * static_cast<float>(m_state_next.x);
        m_true_position.y = (1 - t) * m_true_position.y + t * static_cast<float>(m_state_next.y);
        update_integer_coordinates();

        m_dVector.x = (1 - t) * m_dVector.x + t * static_cast<float>(m_state_next.dx);
        m_dVector.y = (1 - t) * m_dVector.y + t * static_cast<float>(m_state_next.dy);

        m_ddVector.x = (1 - t) * m_ddVector.x + t * static_cast<float>(m_state_next.ddx);
        m_ddVector.y = (1 - t) * m_ddVector.y + t * static_cast<float>(m_state_next.ddy);

        m_state_next.time = m_state_current.time + ticks_time;
    }
    // ---

    if (m_dVector.y > 0.01f) {
        change_animation_state(Animation_type::Jump);
    }

    if (m_dVector.x < 0.0f) {
        m_is_fliped = true;
    } else if (m_dVector.x > 0.0f) {
        m_is_fliped = false;
    }

    if (std::fabs(m_dVector.x) > std::numeric_limits<float>::epsilon()) {
        change_animation_state(Animation_type::Run);
    } else if (std::fabs(m_dVector.y) < std::numeric_limits<float>::epsilon()) {
        change_animation_state(Animation_type::Idle);
    }
    return result;
}

void Opponent::set_next_state (const Answer::Other_Payload& next_state) {
    // std::lock_guard<std::mutex> lock(m_state_mutex);
    if (not m_first_state_set) {
        m_state_current   = next_state;
        m_first_state_set = true;
        m_last_tick_time  = next_state.time;
    } else {
        m_state_current = m_state_next;
        m_state_next = next_state;
    }
    m_true_position = SDL_FPoint{static_cast<float>(next_state.x),   static_cast<float>(next_state.y)};
    m_dVector       = SDL_FPoint{static_cast<float>(next_state.dx),  static_cast<float>(next_state.dy)};
    m_ddVector      = SDL_FPoint{static_cast<float>(next_state.ddx), static_cast<float>(next_state.ddy)};
}
