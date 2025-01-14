#ifndef BLOCK_H
#define BLOCK_H
#include "Entity.h"
#include "SDL3/SDL_rect.h"
#include <array>

class Entity_block : public Entity {
public:
    enum class Block_type : uint8_t {
        Top    = 0,
        Body   = 1,
        Finish = 2
    };

    inline static const std::array<const char*, 3> TEXTURE_PATH_MAP = {
        "assets/tiles/city/top.png",
        "assets/tiles/city/body.png",
        "assets/tiles/city/win.png"
    };

public:
    Entity_block (SDL_Texture* texture = nullptr, SDL_Point tile_position = {0, 0});


};

#endif // BLOCK_H