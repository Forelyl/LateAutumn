#ifndef PLAYER_H
#define PLAYER_H
#include "Movable.h"
#include "Map.h"

class Player : public Movable_entity {
public:
    inline static constexpr int FRAME_WIDTH  = static_cast<int>(52 * SCALE);
    inline static constexpr int FRAME_HEIGHT = static_cast<int>(60 * SCALE);
    inline static const SDL_Point POSITION_OFFSET = {FRAME_WIDTH / 2, FRAME_HEIGHT - TILE_SIZE};

    inline static const Movable_entity::Character_entity_info INFO = {
        .texture_path = "assets/player/Player.png",
        .texture_box  = {0, 0, FRAME_WIDTH, FRAME_HEIGHT},
        .boundary_box = {16, 6, 24, 54}
    };

public:
    Player (SDL_Texture* texture, SDL_FPoint tile_position, const Map& map);
    void make_logical_tick (unsigned long long ticks_time);
    [[nodiscard]] bool has_won () const;

public: // speed and position
    /**
     * @brief Get the camera object
     *
     * @return SDL_Rect - in pixels
     */
    [[nodiscard]] SDL_Rect get_camera () const;

protected:
    inline static constexpr int CAMERA_WIDTH  = 42 * TILE_SIZE;
    inline static constexpr int CAMERA_HEIGHT = 24 * TILE_SIZE;

    void update_camera (float rate);

protected:
    SDL_FRect  m_camera;

protected: // animation
    inline static const std::map<Animation_type, Animation_data> ANIMATION_DATA_MAP {
        {Animation_type::Idle, {.frame_start=1,  .frame_end=9,  .speed_multiplier=1}},
        {Animation_type::Run,  {.frame_start=10, .frame_end=17, .speed_multiplier=1}},
        {Animation_type::Jump, {.frame_start=18, .frame_end=21, .speed_multiplier=1}},
        {Animation_type::Fall, {.frame_start=22, .frame_end=25, .speed_multiplier=1}},
        {Animation_type::Hit,  {.frame_start=26, .frame_end=27, .speed_multiplier=1}}
    };



};

#endif // PLAYER_H