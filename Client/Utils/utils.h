#ifndef UTILS_H
#define UTILS_H
#include <SDL3/SDL.h>
#include <functional>
#include <optional>

namespace utils {
    std::optional<SDL_FPoint> find_colision_on_line_ray_cast (SDL_FPoint start, SDL_FPoint end, const std::function<bool(SDL_Point)>& colision_function);
    float smoth (float a, float b, float t, float max_difference);
}

#endif // UTILS_H