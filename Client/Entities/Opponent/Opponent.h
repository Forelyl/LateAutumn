#ifndef OPPONENT_H
#define OPPONENT_H
#include "Movable.h"
#include "Network.h"
// #include <mutex>

class Opponent : public Movable_entity {
public:
    inline static constexpr int FRAME_WIDTH  = static_cast<int>(52 * SCALE);
    inline static constexpr int FRAME_HEIGHT = static_cast<int>(60 * SCALE);
    inline static const SDL_Point POSITION_OFFSET = {FRAME_WIDTH / 2, FRAME_HEIGHT - TILE_SIZE};

    inline static const Movable_entity::Character_entity_info INFO = {
        .texture_path = "assets/player/Other_player.png",
        .texture_box  = {0, 0, FRAME_WIDTH, FRAME_HEIGHT},
        .boundary_box = {16, 6, 24, 54}
    };

public:
    Opponent (SDL_Texture* texture, SDL_FPoint tile_position, const Map& map);
    // void make_logical_tick (unsigned long ticks_time);
    [[nodiscard]] bool has_won () const;

    void make_tick (unsigned long long ticks_time);
    std::optional<float> make_logical_tick (unsigned long long ticks_time);

    void set_next_state (const Answer::Other_Payload& next_state);

protected:
    inline static constexpr int CAMERA_WIDTH  = 42 * TILE_SIZE;
    inline static constexpr int CAMERA_HEIGHT = 24 * TILE_SIZE;

protected: // animation
    inline static const std::map<Animation_type, Animation_data> ANIMATION_DATA_MAP {
        {Animation_type::Idle, {.frame_start=1,  .frame_end=9,  .speed_multiplier=1}},
        {Animation_type::Run,  {.frame_start=10, .frame_end=17, .speed_multiplier=1}},
        {Animation_type::Jump, {.frame_start=18, .frame_end=21, .speed_multiplier=1}},
        {Animation_type::Fall, {.frame_start=22, .frame_end=25, .speed_multiplier=1}},
        {Animation_type::Hit,  {.frame_start=26, .frame_end=27, .speed_multiplier=1}}
    };

protected:
    Answer::Other_Payload m_state_current;
    Answer::Other_Payload m_state_next;
    // std::mutex            m_state_mutex{};

    bool                  m_first_state_set = false;
    unsigned long long    m_last_tick_time  = 0;
};

#endif // OPPONENT_H