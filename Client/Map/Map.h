#ifndef MAP_H
#define MAP_H
#include <_mingw_mac.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include "Block.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include <vector>
#include <functional>

class Map {
public:
    enum class MAP_LEGEND : uint8_t {
        Top         = 1,
        Body        = 2,
        Empty       = 3,
        Finish      = 4,
        Start       = 5,
        Finish_body = 6
    };

public:
    Map (const std::string& map_name);
    // TODO: on changing-destruction map it is nice idea to maybe release textures ids somehow
    // returns next free id
    void init_textures (const std::function<SDL_Texture*(const std::string &path, unsigned long* id)>& load_texture_func);

public:
    [[nodiscard]] Map::MAP_LEGEND   get     (size_t i, size_t j) const;
    [[nodiscard]] bool              is_wall (size_t i, size_t j) const;
    [[nodiscard]] bool              is_win  (size_t i, size_t j) const;

    void                            draw    (SDL_Rect camera_view,
                                            const std::function<void(const Entity *entity, const SDL_Point& disposition)>& add_entity_func);

    [[nodiscard]] const SDL_FPoint& get_player_start_position () const;
    [[nodiscard]] const SDL_FPoint& get_finish_position       () const;

private:
    std::vector<char> m_map; // char is ok for 1-4, maybe even redundant
                           // it is matrix
    size_t m_i_len;
    size_t m_j_len;

    inline static const SDL_FPoint NO_POSITION{-1, -1};
    SDL_FPoint m_player_start = NO_POSITION;
    SDL_FPoint m_finish       = NO_POSITION;

    struct block_vector {
        std::vector<Entity_block> vector;       // will be used to remove need of constantly creating blocks
        size_t index;
        size_t prepaired;
        SDL_Texture* texture;
        unsigned long texture_id;
    };

    block_vector m_body_block_vect;
    block_vector m_top_block_vect;
    block_vector m_finish_body_block_vect;

private:
    static void error_while_reading_file ();
    static MAP_LEGEND char_to_legend (char ch);
    static Entity_block::Block_type legend_type_block_to_block_type (MAP_LEGEND type);

    // check if there available next block of type type in its corespondent vector (makes next one if there no block)
    // returns block of specified type
    Entity_block* check_prepare_next_block (Map::MAP_LEGEND type);
    void draw_block (SDL_Point lt_block,
                    const std::function<void(const Entity* entity, const SDL_Point& diposition)>& add_entity_func,
                    MAP_LEGEND type, SDL_Point disposition);
    void reset_block_vectors ();

private:
    void set (size_t i, size_t j, Map::MAP_LEGEND value);
    void read_map (const std::string& map_name);

};

#endif // MAP_H