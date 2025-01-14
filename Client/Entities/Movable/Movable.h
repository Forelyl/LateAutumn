#ifndef MOVABLE_H
#define MOVABLE_H
#include "Entity.h"
#include "SDL3/SDL_rect.h"
#include <cstdint>
#include <map>
#include <optional>
#include "Map.h"
#include "SDL3/SDL_render.h"

class Movable_entity : public Entity {

public:
    enum class Animation_type: uint8_t {
        Idle = 1,
        Run  = 2,
        Jump = 3,
        Hit  = 4,
        Fall = 5
    };

    struct Animation_data {
        int    frame_start;
        int    frame_end;
        double speed_multiplier; // TODO: check if needed
    };

    struct Character_entity_info {
        const char*     texture_path;
        const SDL_Rect  texture_box;
        const SDL_Rect  boundary_box;
    };

    inline static constexpr int FRAME_WIDTH  = static_cast<int>(52 * SCALE);
    inline static constexpr int FRAME_HEIGHT = static_cast<int>(60 * SCALE);
    inline static const SDL_Point POSITION_OFFSET = {FRAME_WIDTH / 2, FRAME_HEIGHT - TILE_SIZE};

public:
    Movable_entity (const Character_entity_info& info, SDL_Texture* texture, SDL_Point position_offset,
                    const std::map<Animation_type, Animation_data>* m_animation_data_map,
                    int frame_width, int frame_hight, SDL_FPoint tile_position, const Map& map);

    void make_tick (unsigned long long ticks_time); // process state one time
    std::optional<float> make_logical_tick (unsigned long long ticks_time);
    [[nodiscard]] bool is_flipped   () const; // for rendering

    // ---
    // speed and position
    // ---

    void go_left  ();
    void go_right ();
    void stop_x_movement ();
    void reset_x_movement ();
    bool jump ();

    bool in_air () { return m_in_air; }
    [[nodiscard]] SDL_FPoint get_speed () const;


    void update_position (SDL_FPoint position);
    void update_speed    (SDL_FPoint speed);
    void update_velocity (SDL_FPoint velocity);

protected:
    // ---
    // global
    // ---
    const int m_frame_width;
    const int m_frame_height;

    // ---
    // animation
    // ---
    const std::map<Animation_type, Animation_data>* m_animation_data_map; // maybe bug with clangd

    inline static const int ANIMATION_DELAY = 100; // ms
    unsigned long m_animation_delay_bank;

    int m_frame;
    Animation_type m_animation_state;
    bool m_is_fliped;

    // is fulled in public function that should be called
    void update_texture ();
    void do_animation_tick (unsigned long long ticks_time);
    void change_animation_state (Animation_type type);

    // ---
    // movement and colisions
    // ---

    const Map& m_map;
    void set_in_air  (bool in_air);
    struct found_colision { bool left; bool top; bool right; bool bottom;};
    void do_colision (SDL_FPoint move_true_vector, float rate);
    /**
     *  @brief find first tile with wich character colides on its way
     *         uses tiles coordinates and returns tiles coordinates
     *         based on DTA algorithm
     */
    std::optional<std::pair<SDL_FPoint, found_colision>> find_tiled_colision (SDL_FPoint old_tiled_position, SDL_FPoint move_tiled_vector);

protected:
    SDL_FPoint    m_dVector  = {0, 0};
    SDL_FPoint    m_ddVector = {0, 0};
    bool          m_in_air;
    unsigned long m_ticks_of_logical_update_bank = 0;

protected:
    inline static const float GRAVITY_ACCELERATION = 30 * TILE_SIZE;
    inline static const float RUN_SPEED = 15 * TILE_SIZE;
    inline static const float RUN_SPEED_ACCELERATION = 40 * TILE_SIZE;
    inline static const float JUMP_SPEED = 12 * TILE_SIZE;

    inline static const unsigned long long MAX_TICKS_TO_PROCESS = 100L; // to prevent broken colisions

};

#endif // MOVABLE_H