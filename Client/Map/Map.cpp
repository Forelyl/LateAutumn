#include "Map.h"
#include "Block.h"
#include "Entity.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <fstream>

Map::Map (const std::string& map_name)
  : m_map(), m_i_len(0), m_j_len(0) {
    read_map(map_name);

    m_body_block_vect.vector.reserve(220);
    m_body_block_vect.index     = 0;
    m_body_block_vect.prepaired = 0;

    m_top_block_vect.vector.reserve(220);
    m_top_block_vect.index      = 0;
    m_top_block_vect.prepaired  = 0;

    m_finish_body_block_vect.vector.reserve(2);
    m_finish_body_block_vect.index = 0;
    m_finish_body_block_vect.prepaired = 0;


}

void Map::init_textures (const std::function<SDL_Texture*(const std::string &path, unsigned long* id)>& load_texture_func) {
    const char* body_texture_path         = Entity_block::TEXTURE_PATH_MAP.at(size_t(Entity_block::Block_type::Body  ));
    const char* top_texture_path          = Entity_block::TEXTURE_PATH_MAP.at(size_t(Entity_block::Block_type::Top   ));
    const char* finish_body_texture_path  = Entity_block::TEXTURE_PATH_MAP.at(size_t(Entity_block::Block_type::Finish));

    m_body_block_vect.texture            = load_texture_func(body_texture_path,         &m_body_block_vect.texture_id);
    m_top_block_vect.texture             = load_texture_func(top_texture_path,          &m_top_block_vect.texture_id);
    m_finish_body_block_vect.texture     = load_texture_func(finish_body_texture_path,  &m_finish_body_block_vect.texture_id);
}


Map::MAP_LEGEND Map::get (size_t i, size_t j) const {
    if (i >= m_i_len or j >= m_j_len or j * m_i_len + i >= m_map.size()) {
        // SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Game memory leak... (1)");
        // exit(1);

        return MAP_LEGEND::Empty;
    }

    return static_cast<MAP_LEGEND>(m_map[j * m_i_len + i]);
}

bool Map::is_wall (size_t i, size_t j) const {
    MAP_LEGEND legend = get(i, j);
    return legend == MAP_LEGEND::Body or
           legend == MAP_LEGEND::Top  or
           legend == MAP_LEGEND::Finish_body;
}


bool Map::is_win (size_t i, size_t j) const {
    return get(i, j) == MAP_LEGEND::Finish;
}

Entity_block::Block_type Map::legend_type_block_to_block_type (Map::MAP_LEGEND type) {
    if (type == MAP_LEGEND::Body)        return Entity_block::Block_type::Body;
    if (type == MAP_LEGEND::Top)         return Entity_block::Block_type::Top;
    if (type == MAP_LEGEND::Finish_body) return Entity_block::Block_type::Finish;

    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Internal problem while drawing map - unknow object conversion");
    exit(1);
}

Entity_block* Map::check_prepare_next_block (Map::MAP_LEGEND type) {
    block_vector* vector_var = nullptr;
    switch (type) {
        case MAP_LEGEND::Body:
            vector_var = &m_body_block_vect;
            break;
        case MAP_LEGEND::Top:
            vector_var = &m_top_block_vect;
            break;
        case MAP_LEGEND::Finish_body:
            vector_var = &m_finish_body_block_vect;
            break;
        default:
            return nullptr;
    }

    // no need to prepare
    if (vector_var->index < vector_var->prepaired) {
        return &vector_var->vector[vector_var->index++];
    }

    // preparing
    // Entity_block::Block_type block_type = legend_type_block_to_block_type(type);
    Entity_block block = Entity_block{vector_var->texture, {0, 0}};

    vector_var->vector.emplace_back(block);
    vector_var->prepaired++;
    return &vector_var->vector[vector_var->index++];
}

void Map::draw_block (SDL_Point lt_block,
                      const std::function<void(const Entity* entity, const SDL_Point& diposition)>& add_entity_func,
                      Map::MAP_LEGEND type, SDL_Point disposition) {

    Entity_block* block = check_prepare_next_block(type);
    // return;
    // std::cout << lt_block.x << " " << lt_block.y << " ";
    block->move(lt_block);
    add_entity_func(block, disposition);
}

void Map::reset_block_vectors () {
    m_body_block_vect.index = 0;
    m_top_block_vect.index  = 0;
    m_finish_body_block_vect.index = 0;
}

void Map::draw (SDL_Rect camera_view, const std::function<void(const Entity *entity, const SDL_Point& disposition)>& add_entity_func) {
    // l-left t-top r-right b-bottom

    // lt - at least most of the time (if not always) been less then 0 (both x and y) - to prevent errors use max 0
    int camera_x = std::max(camera_view.x, 0);
    int camera_y = std::max(camera_view.y, 0);

    SDL_Point lt = {long(camera_x) / long(Entity::TILE_SIZE), long(camera_y) / long(Entity::TILE_SIZE)};
    SDL_Point rb = {lt.x + long(camera_view.w + 1) / long(Entity::TILE_SIZE), lt.y + long(camera_view.h + 1) / long(Entity::TILE_SIZE)};

    for (long j = lt.y; j <= rb.y; ++j) {
        for (long i = lt.x; i <= rb.x; ++i) {
            if (not is_wall(static_cast<size_t>(i), static_cast<size_t>(j))) continue;

            auto legend = get(static_cast<size_t>(i), static_cast<size_t>(j));
            draw_block(
                {i, j},
                add_entity_func,
                legend,
                SDL_Point{
                    -camera_view.x,
                    -camera_view.y
                }
            );
        }
    }

    reset_block_vectors();
}

const SDL_FPoint& Map::get_player_start_position () const {
    static SDL_FPoint mew = {3, 1};
    return mew;
    // return m_player_start;
}
const SDL_FPoint& Map::get_finish_position       () const { return m_finish; }

void Map::set (size_t i, size_t j, Map::MAP_LEGEND value) {
    if (i >= m_i_len or j >= m_j_len or j * m_i_len + i >= m_map.size()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Game memory leak... (2)");
        exit(2);
    }
    m_map[j * m_i_len + i] = static_cast<char>(value);
}

void Map::error_while_reading_file () {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Map can't be loaded, bad reading (1)");
        exit(1);
}

Map::MAP_LEGEND Map::char_to_legend (char ch) {
    switch (ch) {
        case '#':
            return Map::MAP_LEGEND::Body;
        case ' ':
            return Map::MAP_LEGEND::Empty;
        case 'S':
            return Map::MAP_LEGEND::Start;
        case 'F':
            return Map::MAP_LEGEND::Finish;
        case '$':
            return Map::MAP_LEGEND::Finish_body;
    }
    error_while_reading_file();
    exit(1); // unreachable code
}

void Map::read_map (const std::string& map_name) {
    std::ifstream map_in("data/maps/" + map_name + ".map");
    if (not map_in.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Map can't be loaded... (1)");
        exit(1);
    }

    size_t num;
    map_in >> num;
    if (not map_in) error_while_reading_file();
    m_i_len = num;

    map_in >> num;
    if (not map_in) error_while_reading_file();
    m_j_len = num;

    m_map.resize(m_j_len * m_i_len);


    // remove \n
    if (map_in.get() != '\n') error_while_reading_file();

    // ---------
    for (size_t j = 0; j < m_j_len; ++j) {
        size_t i;
        for (i = 0; i < m_i_len; ++i) {
            if (map_in.peek() == '\n') {
                for (;i < m_i_len; ++i) set(i, j, MAP_LEGEND::Empty);
                break;
            }

            auto legend = char_to_legend(static_cast<char>(map_in.get()));
            switch (legend) { // update legend and do additional logic
                case MAP_LEGEND::Start:
                    m_player_start = SDL_FPoint{static_cast<float>(i), static_cast<float>(j)};
                    break;
                case MAP_LEGEND::Finish:
                    m_finish       = SDL_FPoint{static_cast<float>(i), static_cast<float>(j)};
                    break;
                case MAP_LEGEND::Body: {
                    bool is_block_with_free_up_side = j > 0 and (get(i, j - 1) != MAP_LEGEND::Body and get(i, j - 1) != MAP_LEGEND::Top);
                    is_block_with_free_up_side      = is_block_with_free_up_side or j == 0;

                    if (is_block_with_free_up_side) {
                        legend = MAP_LEGEND::Top;
                    } else {
                        legend = MAP_LEGEND::Body;
                    }
                    break;
                }
                default:
                    break;
            }
            set(i, j, legend);
        }

        if (j != m_j_len - 1) {
            if (map_in.get() != '\n') {
                error_while_reading_file();
            }
        }
    }

}
