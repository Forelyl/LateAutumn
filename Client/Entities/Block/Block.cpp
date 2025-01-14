#include "Block.h"
#include "Entity.h"
Entity_block::Entity_block (SDL_Texture* texture, SDL_Point tile_position)
    : Entity(
        texture,
        {0, 0, Entity::TILE_SIZE, Entity::TILE_SIZE},
        {0, 0, Entity::TILE_SIZE, Entity::TILE_SIZE},
        SDL_FPoint(float(tile_position.x), float(tile_position.y)),
        {0, 0}
      ) {}
