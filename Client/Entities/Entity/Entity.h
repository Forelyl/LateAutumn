#ifndef ENTITY_H
#define ENTITY_H
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include <SDL3/SDL.h>
#include <_mingw_stat64.h>

/**
1 -> .___.
     |   |
     |___| <.
             \ 2
**/
struct Boundary {
    long x1, y1, x2, y2;
};

struct Boundary_F {
    float x1, y1, x2, y2;
};


class Entity {
public:
    inline static constexpr float SCALE       = 1;
    inline static constexpr long  TILE_SIZE   = static_cast<long>(32 * SCALE);
    inline static constexpr float TILE_SIZE_F = static_cast<float>(TILE_SIZE);

public:
    /**
     * @param position in float tile coordinates
     * @param (other params) in normal pixel
     */
    Entity (SDL_Texture* texture, SDL_Rect texture_box, SDL_Rect boundary_box, SDL_FPoint tile_position, SDL_Point position_offset);

    [[nodiscard]] SDL_Texture*      get_texture           () const;

    /**
     * @brief Get the position object
     *
     * @return const SDL_Point& - position in pixels
     */
    [[nodiscard]] const SDL_Point&  get_position          () const;
    [[nodiscard]] SDL_Point         get_texture_size      () const;
    [[nodiscard]] SDL_Rect          get_source_rect       () const; // texture box
    [[nodiscard]] SDL_Rect          get_destination_rect  () const;

    [[nodiscard]] Boundary          get_boundary          () const; // relative to (0, 0)

public:
    /**
     * @brief for movement of inner anchor point (adding offset)
     * @param new_tile_position in float tile coordinates (non pixel)
     */
    void move (SDL_FPoint new_tile_position);
    /**
     * @brief for movement of inner anchor point (adding offset)
     * @param new_tile_position in integer tile coordinates (non pixel)
     */
    void move (SDL_Point new_tile_position);

protected:
    SDL_Texture*  m_texture;

    SDL_FPoint    m_true_position; // in pixels
    SDL_Point     m_position;      // in pixels

    SDL_Rect      m_texture_box;
    SDL_Rect      m_boundary_box;     // relative to upper-left corner
    SDL_Point     m_position_offset;  // by setting value: changing anchor from (0, 0) to this

protected:
    // for movement of lt (left top) point
    void move_lt      (SDL_Point  new_position);
    void move_lt_true (SDL_FPoint new_position);

    /**
     * @brief update true position from integer position
     */
    void update_true_coordinates    ();
    /**
     * @brief update integer position from true position
     *
     */
    void update_integer_coordinates ();
};

#endif // ENTITY_H