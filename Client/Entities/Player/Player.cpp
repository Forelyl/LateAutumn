#include "Player.h"
#include "Movable.h"
#include "utils.h"
#include <cmath>
#include <cstdio>
#include <minwindef.h>


Player::Player(SDL_Texture *texture, SDL_FPoint tile_position, const Map &map)
    : Movable_entity(
          INFO,
          texture,
          POSITION_OFFSET,
          &ANIMATION_DATA_MAP,
          FRAME_WIDTH,
          FRAME_HEIGHT,
          tile_position,
          map
      ),
      m_camera{0, 0, static_cast<float>(CAMERA_WIDTH), static_cast<float>(CAMERA_HEIGHT)} {

    update_camera(1.0f);
}


void Player::make_logical_tick (unsigned long long ticks_time) {
    std::optional<float> rate_opt = Movable_entity::make_logical_tick(ticks_time);
    if (not rate_opt.has_value()) return;
    update_camera(rate_opt.value());
}


bool Player::has_won () const {
    return m_map.is_win(
        size_t(m_position.x + m_position_offset.x) / TILE_SIZE, // integer division
        size_t(m_position.y + m_position_offset.y) / TILE_SIZE  // integer division
    );

}

SDL_Rect Player::get_camera () const { return {long(m_camera.x), long(m_camera.y), CAMERA_WIDTH, CAMERA_HEIGHT}; }


void Player::update_camera (float rate) {
    // ------------- player center -------------
    SDL_Point center = {m_position.x + m_position_offset.x, m_position.y + m_position_offset.y};


    // ------------- update camera -------------
    SDL_FPoint camera_end = {
        float(center.x - CAMERA_WIDTH / 2),  // integer division
        float(center.y - CAMERA_HEIGHT / 2)  // integer division
    };
    m_camera.x = utils::smoth(m_camera.x, camera_end.x, rate, TILE_SIZE_F * 3);
    m_camera.y = utils::smoth(m_camera.y, camera_end.y, rate, TILE_SIZE_F * 3);

}