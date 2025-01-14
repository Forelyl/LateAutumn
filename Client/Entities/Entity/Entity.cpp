#include "Entity.h"
#include "SDL3/SDL_rect.h"
#include <cmath>

Entity::Entity (SDL_Texture* texture, SDL_Rect texture_box, SDL_Rect boundary_box, SDL_FPoint tile_position, SDL_Point position_offset)
  : m_texture(texture), m_texture_box(texture_box), m_boundary_box(boundary_box), m_position_offset(position_offset)  {
    move(tile_position);

    // APPLY SCALE - only graphic changes
    m_texture_box.w  = long(float(m_texture_box.w) * SCALE);
    m_texture_box.h  = long(float(m_texture_box.h) * SCALE);
    m_texture_box.x  = long(float(m_texture_box.x) * SCALE);
    m_texture_box.y  = long(float(m_texture_box.y) * SCALE);

}

SDL_Texture*      Entity::get_texture       () const { return m_texture;      }
SDL_Point         Entity::get_texture_size  () const { return {m_texture_box.w, m_texture_box.h}; }
SDL_Rect          Entity::get_source_rect   () const { return m_texture_box; }
const SDL_Point&  Entity::get_position      () const { return m_position;     }

SDL_Rect Entity::get_destination_rect  () const {
    return {
        long(m_true_position.x * SCALE),
        long(m_true_position.y * SCALE),
        long(m_texture_box.w * SCALE),
        long(m_texture_box.h * SCALE)
    };
}

// relative to left top of entity
Boundary Entity::get_boundary () const {
    return {
        .x1=m_position.x + m_boundary_box.x,
        .y1=m_position.y + m_boundary_box.y,

        .x2=m_position.x + m_boundary_box.x + m_boundary_box.w,
        .y2=m_position.y + m_boundary_box.y + m_boundary_box.h
    };
}

void Entity::move (SDL_FPoint new_tile_position) {
    new_tile_position.x *= TILE_SIZE;
    new_tile_position.y *= TILE_SIZE;

    m_position = {
        .x=long(new_tile_position.x) - m_position_offset.x,
        .y=long(new_tile_position.y) - m_position_offset.y
    };
    update_true_coordinates();
}

void Entity::move (SDL_Point new_tile_position) {
    new_tile_position.x *= TILE_SIZE;
    new_tile_position.y *= TILE_SIZE;

    m_position.x = new_tile_position.x - m_position_offset.x;
    m_position.y = new_tile_position.y - m_position_offset.y;

    update_true_coordinates();
}

void Entity::move_lt (SDL_Point new_position) {
    m_position = new_position;
    update_true_coordinates();
}

void  Entity::move_lt_true (SDL_FPoint new_position) {
    m_true_position = new_position;
    update_integer_coordinates();
}

void Entity::update_true_coordinates () {
    m_true_position.x = float(m_position.x);
    m_true_position.y = float(m_position.y);
}

void Entity::update_integer_coordinates () {
    m_position.x = long(m_true_position.x);
    m_position.y = long(m_true_position.y);
}
