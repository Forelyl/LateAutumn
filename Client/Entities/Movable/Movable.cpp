#include "Network.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include "utils.h"
#include <optional>
#include "Movable.h"

// #define DEBUG_ANIMATION
// #define DEBUG_MOVE
// #define DEBUG_COLISION

using std::tuple;

// Movable_entity(Character_type type, SDL_Texture *texture,
                                //    SDL_FPoint tile_position, const Map &map)
Movable_entity::Movable_entity (const Character_entity_info& info, SDL_Texture* texture, SDL_Point position_offset,
                                const std::map<Animation_type, Animation_data>* animation_data_map,
                                int frame_width, int frame_hight, SDL_FPoint tile_position, const Map& map)
    : Entity(
          texture, info.texture_box, info.boundary_box,
          tile_position, position_offset
      ),

      m_frame_width{frame_width}, m_frame_height{frame_hight},
      m_animation_data_map{animation_data_map},
      m_animation_delay_bank{0}, m_frame{0},
      m_animation_state{Animation_type::Idle}, m_is_fliped{false},

      m_map{map},
      m_dVector({.x = 0, .y = 0}), m_ddVector({.x = 0, .y = 0}),
      m_in_air{false} {

}

std::optional<float> Movable_entity::make_logical_tick (unsigned long long ticks_time) {

    ticks_time = std::min(ticks_time, MAX_TICKS_TO_PROCESS);

    m_ticks_of_logical_update_bank += ticks_time;
    if (m_ticks_of_logical_update_bank < 30) return std::nullopt;

    float rate = static_cast<float>(m_ticks_of_logical_update_bank) / 1000.0f;
    m_ticks_of_logical_update_bank = 0;


    // ------------- movement -------------
    SDL_FPoint move_true_vector;
    m_dVector.x  = m_ddVector.x > 0 ? std::min(m_dVector.x + m_ddVector.x * rate, RUN_SPEED) : std::max(m_dVector.x + m_ddVector.x * rate, -RUN_SPEED);
    m_dVector.y  = std::max(std::min(m_dVector.y + m_ddVector.y * rate, JUMP_SPEED), -JUMP_SPEED * 3);

    move_true_vector.x =  m_dVector.x * rate;
    move_true_vector.y = -m_dVector.y * rate;

    if (m_true_position.x + move_true_vector.x < 0.0f) move_true_vector.x = -m_true_position.x;
    if (m_true_position.y + move_true_vector.y < 0.0f) move_true_vector.y = -m_true_position.y;


    #ifdef DEBUG_MOVE
    if (m_dVector.x != 0 or m_dVector.y != 0)
        std::cout << m_dVector.x << " " << m_dVector.y << " | " << m_ddVector.x << " " << m_ddVector.y << " | " << m_true_position.x << " " << m_true_position.y << std::endl;
    #endif

    if (m_dVector.y < -1) {
        change_animation_state(Animation_type::Fall);
    } else if (not (m_animation_state == Animation_type::Run) and std::abs(m_dVector.y) < 0.001f) {
        // not (m_animation_state == Animation_type::Run) - workaround of how run animation is working
        // previous expression was: m_dVector.x == 0
        change_animation_state(Animation_type::Idle);
    }

    // ------------- check colision -------------

    do_colision(move_true_vector, rate);

    return rate;
}


void Movable_entity::make_tick (unsigned long long ticks_time) {
    ticks_time = std::min(ticks_time, MAX_TICKS_TO_PROCESS);

    // ------------- animation -------------
    do_animation_tick(ticks_time);

}

bool Movable_entity::is_flipped () const { return m_is_fliped; }

void Movable_entity::go_left () {
    m_is_fliped = true;
    m_ddVector.x = -RUN_SPEED_ACCELERATION;
    if (std::abs(m_dVector.y) < 1) {   // is falling
        change_animation_state(Animation_type::Run);
    }

    if (m_dVector.x > 2) {  // was previously moving faslty to right
        m_dVector.x = 2;
    }
}

void Movable_entity::go_right () {
    m_is_fliped = false;
    m_ddVector.x = RUN_SPEED_ACCELERATION;
    if (std::abs(m_dVector.y) < 1) {  // is falling
        change_animation_state(Animation_type::Run);
    }

    if (m_dVector.x < -2) {  // was previously moving fastly to left
        m_dVector.x = -2;
    }
}

bool Movable_entity::jump () {
    if (m_in_air) return false;
    m_dVector.y = JUMP_SPEED;
    change_animation_state(Animation_type::Jump);
    return true;
}

SDL_FPoint Movable_entity::get_speed () const {
    return m_dVector;
}

void Movable_entity::update_position (SDL_FPoint position) {
    m_true_position = position;
    update_integer_coordinates();
}

void Movable_entity::update_speed (SDL_FPoint speed) {
    m_dVector = speed;
}

void Movable_entity::update_velocity (SDL_FPoint velocity) {
    m_ddVector = velocity;
}



void Movable_entity::stop_x_movement () {
    m_dVector.x = 0;
    if (m_animation_state == Animation_type::Run) change_animation_state(Animation_type::Idle);
}

void Movable_entity::reset_x_movement () {
    m_ddVector.x = 0;
}


void Movable_entity::update_texture () {
    m_texture_box.x = FRAME_WIDTH * (m_frame - 1);
}

void Movable_entity::do_animation_tick (unsigned long long ticks_time) {
    m_animation_delay_bank += ticks_time;
    if (m_animation_delay_bank < ANIMATION_DELAY) return;

    int frames = m_animation_delay_bank / ANIMATION_DELAY;   // integer division
    m_animation_delay_bank %= ANIMATION_DELAY;              // skip on lag

    // ---
    Animation_data animation_data = m_animation_data_map->at(m_animation_state);
    int next_frame = m_frame + frames;
    if (next_frame > animation_data.frame_end) next_frame = animation_data.frame_start;
    m_frame = next_frame;

    update_texture();
}

void Movable_entity::change_animation_state (Movable_entity::Animation_type type) {
    // return;
    if (m_animation_state == type) return;

    #ifdef DEBUG_ANIMATION
    std::cout << int(type) << std::endl;
    #endif


    m_animation_state = type;
    Animation_data animation_data = m_animation_data_map->at(m_animation_state);
    m_frame = animation_data.frame_start;
}


void Movable_entity::set_in_air (bool in_air) {
    if (in_air == m_in_air) return;
    if (not in_air) m_dVector.y = 0;
    m_in_air = in_air;
    m_ddVector.y = m_in_air ? -GRAVITY_ACCELERATION : 0;
}

void Movable_entity::do_colision (SDL_FPoint move_true_vector, float rate) {
    SDL_FPoint start_tile        = {m_true_position.x / TILE_SIZE_F,  m_true_position.y / TILE_SIZE_F };
    SDL_FPoint tiled_move_vector = {move_true_vector.x / TILE_SIZE_F, move_true_vector.y / TILE_SIZE_F};
    std::optional<std::pair<SDL_FPoint, found_colision>> colision_tiled_opt =
        find_tiled_colision(start_tile, tiled_move_vector);

    if (not colision_tiled_opt.has_value()) {
        move_lt_true({m_true_position.x + move_true_vector.x, m_true_position.y + move_true_vector.y});
        set_in_air(true);
        return;
    }

    SDL_FPoint& colision_tiled = colision_tiled_opt.value().first;
    SDL_FPoint colision_coordinates = {colision_tiled.x * TILE_SIZE, colision_tiled.y * TILE_SIZE};

    found_colision colision = colision_tiled_opt.value().second;
    bool is_x_colision = colision.left or colision.right;
    SDL_FPoint old_true_position = m_true_position;
    move_lt_true(colision_coordinates);


    // ---
    // swept - push
    if (is_x_colision) {
        if (std::fabsf(move_true_vector.x) <= std::numeric_limits<float>::epsilon()) return;
        rate = rate * (move_true_vector.x + old_true_position.x - m_true_position.x) / (move_true_vector.x); // TODO: optimize math
    } else {
        if (std::fabsf(move_true_vector.y) <= std::numeric_limits<float>::epsilon()) return;
        rate = rate * (move_true_vector.y + old_true_position.y - m_true_position.y) / (move_true_vector.y); // TODO: optimize math
    }

    start_tile = SDL_FPoint{m_true_position.x / TILE_SIZE_F,  m_true_position.y / TILE_SIZE_F };
    if (is_x_colision) {
        float true_end = rate * -m_dVector.y;
        move_true_vector = SDL_FPoint{0, true_end};
    } else  {
        float true_end = rate * m_dVector.x;
        move_true_vector = SDL_FPoint{true_end, 0};
    }

    if        (colision.top) {
        start_tile.y        += 1.0f / TILE_SIZE_F;
        move_true_vector.y  -= 1.0f;
    } else if (colision.left) {
        start_tile.x        += 1.0f / TILE_SIZE_F;
        move_true_vector.x  -= 1.0f;
    } else if (colision.right) {
        start_tile.x        -= 1.0f / TILE_SIZE_F;
        move_true_vector.x  += 1.0f;
    } else if (colision.bottom) {
        start_tile.y        -= 1.0f / TILE_SIZE_F;
        move_true_vector.y  += 1.0f;
    }

    tiled_move_vector = SDL_FPoint{move_true_vector.x / TILE_SIZE_F, move_true_vector.y / TILE_SIZE_F};
    colision_tiled_opt = find_tiled_colision(start_tile, tiled_move_vector);

    // ---
    if (not colision_tiled_opt.has_value()) {
        move_lt_true({m_true_position.x + move_true_vector.x, m_true_position.y + move_true_vector.y});
        set_in_air(true);
        return;
    }
    // std::cout << move_true_vector.x << '\t' << move_true_vector.y << '\t' <<  m_dVector.y << '\n';

    SDL_FPoint& colision_tiled2 = colision_tiled_opt.value().first;
    colision_coordinates = SDL_FPoint{colision_tiled2.x * TILE_SIZE, colision_tiled2.y * TILE_SIZE};
    colision = colision_tiled_opt.value().second;
    move_lt_true(colision_coordinates);
    // ---

    if (colision.top) {
        m_dVector.y = 0;
    }
    set_in_air(not colision.bottom);
}



std::optional<std::pair<SDL_FPoint, Movable_entity::found_colision>> Movable_entity::find_tiled_colision (SDL_FPoint old_tiled_position, SDL_FPoint move_tiled_vector) {
    const static Boundary_F boundary_box_tiled = {
        .x1=static_cast<float>(m_boundary_box.x) / TILE_SIZE_F,
        .y1=static_cast<float>(m_boundary_box.y) / TILE_SIZE_F,

        .x2=static_cast<float>(m_boundary_box.x + m_boundary_box.w) / TILE_SIZE_F,
        .y2=static_cast<float>(m_boundary_box.y + m_boundary_box.h) / TILE_SIZE_F
    };

    const static float ANGLE_OFFSET = 1.0f / TILE_SIZE_F; // 1 pixel offset from any angle
    /*   TL    TR
       * # --- # *
    LT -         - RT
       |         |
    LM -         - RM
       |         |
    LB -         - RB
       * # --- # *
         BL    BR
    */
    enum Point : char { TL = 0, TR, RT, RM, RB, BR, BL, LB, LM, LT};

    const static std::array<SDL_FPoint, 10> offset = { // one time evaluation
        // TL
        SDL_FPoint{ boundary_box_tiled.x1 + ANGLE_OFFSET, boundary_box_tiled.y1},
        // TR
        SDL_FPoint{ boundary_box_tiled.x2 - ANGLE_OFFSET, boundary_box_tiled.y1},


        // RT
        SDL_FPoint{boundary_box_tiled.x2, boundary_box_tiled.y1 + ANGLE_OFFSET},
        // RM
        SDL_FPoint{boundary_box_tiled.x2, (boundary_box_tiled.y1 + boundary_box_tiled.y2) / 2.0f},  // R
        // RB
        SDL_FPoint{boundary_box_tiled.x2, boundary_box_tiled.y2 - ANGLE_OFFSET},


        // BR
        SDL_FPoint{boundary_box_tiled.x2 - ANGLE_OFFSET, boundary_box_tiled.y2},
        // BL
        SDL_FPoint{boundary_box_tiled.x1 + ANGLE_OFFSET, boundary_box_tiled.y2},


        // LB
        SDL_FPoint{boundary_box_tiled.x1, boundary_box_tiled.y2 - ANGLE_OFFSET},
        // LM
        SDL_FPoint{boundary_box_tiled.x1, (boundary_box_tiled.y1 + boundary_box_tiled.y2) / 2.0f},
        // LT
        SDL_FPoint{boundary_box_tiled.x1, boundary_box_tiled.y1},
    };

    std::array<SDL_FPoint, offset.size()> start = {
        SDL_FPoint{offset[TL].x + old_tiled_position.x, offset[TL].y + old_tiled_position.y},
        SDL_FPoint{offset[TR].x + old_tiled_position.x, offset[TR].y + old_tiled_position.y},
        SDL_FPoint{offset[RT].x + old_tiled_position.x, offset[RT].y + old_tiled_position.y},
        SDL_FPoint{offset[RM].x + old_tiled_position.x, offset[RM].y + old_tiled_position.y},
        SDL_FPoint{offset[RB].x + old_tiled_position.x, offset[RB].y + old_tiled_position.y},
        SDL_FPoint{offset[BR].x + old_tiled_position.x, offset[BR].y + old_tiled_position.y},
        SDL_FPoint{offset[BL].x + old_tiled_position.x, offset[BL].y + old_tiled_position.y},
        SDL_FPoint{offset[LB].x + old_tiled_position.x, offset[LB].y + old_tiled_position.y},
        SDL_FPoint{offset[LM].x + old_tiled_position.x, offset[LM].y + old_tiled_position.y},
        SDL_FPoint{offset[LT].x + old_tiled_position.x, offset[LT].y + old_tiled_position.y},

    };

    std::array<SDL_FPoint, offset.size()> end = {
        SDL_FPoint{start[TL].x + move_tiled_vector.x, start[TL].y + move_tiled_vector.y},
        SDL_FPoint{start[TR].x + move_tiled_vector.x, start[TR].y + move_tiled_vector.y},
        SDL_FPoint{start[RT].x + move_tiled_vector.x, start[RT].y + move_tiled_vector.y},
        SDL_FPoint{start[RM].x + move_tiled_vector.x, start[RM].y + move_tiled_vector.y},
        SDL_FPoint{start[RB].x + move_tiled_vector.x, start[RB].y + move_tiled_vector.y},
        SDL_FPoint{start[BR].x + move_tiled_vector.x, start[BR].y + move_tiled_vector.y},
        SDL_FPoint{start[BL].x + move_tiled_vector.x, start[BL].y + move_tiled_vector.y},
        SDL_FPoint{start[LB].x + move_tiled_vector.x, start[LB].y + move_tiled_vector.y},
        SDL_FPoint{start[LM].x + move_tiled_vector.x, start[LM].y + move_tiled_vector.y},
        SDL_FPoint{start[LT].x + move_tiled_vector.x, start[LT].y + move_tiled_vector.y},
    };

    static const std::function<bool(SDL_Point)> colision_function = [&](SDL_Point point) {
        return m_map.is_wall(static_cast<size_t>(point.x), static_cast<size_t>(point.y));
    };

    std::array<std::optional<SDL_FPoint>, offset.size()> colision = {
        utils::find_colision_on_line_ray_cast(start[TL], end[TL], colision_function),
        utils::find_colision_on_line_ray_cast(start[TR], end[TR], colision_function),
        utils::find_colision_on_line_ray_cast(start[RT], end[RT], colision_function),
        utils::find_colision_on_line_ray_cast(start[RM], end[RM], colision_function),
        utils::find_colision_on_line_ray_cast(start[RB], end[RB], colision_function),
        utils::find_colision_on_line_ray_cast(start[BR], end[BR], colision_function),
        utils::find_colision_on_line_ray_cast(start[BL], end[BL], colision_function),
        utils::find_colision_on_line_ray_cast(start[LB], end[LB], colision_function),
        utils::find_colision_on_line_ray_cast(start[LM], end[LM], colision_function),
        utils::find_colision_on_line_ray_cast(start[LT], end[LT], colision_function),
    };

    // ---

    int lesser_distance_index = -1;
    float squared_distance = std::numeric_limits<float>::max(); // squared - to remove need in sqrt

    for (size_t i = 0; i < colision.size(); ++i) {
        if (not colision[i].has_value()) continue;
        SDL_FPoint& point  = colision[i].value();
        SDL_FPoint  vector = {point.x - start[i].x, point.y - start[i].y};

        float distance = vector.x * vector.x + vector.y * vector.y;

        if (distance < squared_distance) {
            squared_distance = distance;
            lesser_distance_index = static_cast<int>(i);
        }
    }
    if (lesser_distance_index == -1) { return std::nullopt; }

    // ---
    auto index = static_cast<size_t>(lesser_distance_index);
    SDL_FPoint result = colision[index].value();

    found_colision result_colision = {
        .left   = index == LT or index == LM or index == LB,
        .top    = index == TL or index == TR,
        .right  = index == RT or index == RM or index == RB,
        .bottom = index == BL or index == BR
    };

    result.x -= offset[index].x;
    result.y -= offset[index].y;

    // a-a-a-a-a-a-a-aa-a-a-a-
    if (result_colision.right) {
        float delta_colision = (result.x + float(m_boundary_box.x + m_boundary_box.w) / TILE_SIZE_F);
        delta_colision = std::floorf(delta_colision) - delta_colision;
        if (std::fabs(delta_colision) > 5.0f * TILE_SIZE_F) delta_colision = 0.0f;
        result.x += delta_colision - 1.0f / TILE_SIZE_F;
    } else if (result_colision.left) {
        float delta_colision = (std::ceilf(result.x) - float(m_boundary_box.x) / TILE_SIZE_F) - result.x;
        if (std::fabs(delta_colision) > 5.0f * TILE_SIZE_F) delta_colision = 0.0f;
        result.x += delta_colision + 1.0f / TILE_SIZE_F;
    }
    // a-a-a-a-a-a-a-aa-a-a-a-

    return std::pair{SDL_FPoint{result.x, result.y}, result_colision};
};