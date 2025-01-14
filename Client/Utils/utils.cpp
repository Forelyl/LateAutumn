#include "utils.h"
#include <cmath>

std::optional<SDL_FPoint> utils::find_colision_on_line_ray_cast (SDL_FPoint start, SDL_FPoint end, const std::function<bool(SDL_Point)>& colision_function) {
        // std::cout << "result: " << result.x << " " << result.y << '\n';
    float step_x = start.x < end.x ? 0.001f : -0.001f;
    float step_y = start.y < end.y ? 0.001f : -0.001f;
    // float step_x = 0, step_y = 0;
    if (start.x == std::roundf(start.x)) start.x += step_x;
    if (start.y == std::roundf(start.y)) {
        start.y += step_y;
    }


    // -------------------

    // using DDA algorithm
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float vector_len = std::sqrtf(dx * dx +  dy * dy);

    SDL_FPoint move_len_normal = {dx / vector_len, dy / vector_len};                     // use to determine coordinate from length of move
    SDL_Point  position        = {static_cast<int>(start.x), static_cast<int>(start.y)}; // coordinates ray position - for colision function
    SDL_Point  end_position    = {static_cast<int>(end.x),   static_cast<int>(end.y)  }; // coordinates ray end position

    float actual_distance = 0;    // distance actually [already] traveled from start

    SDL_FPoint move_len_step = {  // how to change distance when moved by x/y coordinates
        .x=vector_len / dx,
        .y=vector_len / dy
    };

    // ---

    SDL_FPoint move_len;      // distance that will traveled due to move of SDL_point position
    SDL_Point  position_step; // distance that will be traveled on move of SDL_point position

    // set step direction and start move_len (start position is no necessary on grid intersection)

    if (move_len_normal.x > 0) { // right
        position_step.x = 1;
        move_len.x      = (static_cast<float>(position.x + 1) - start.x) * move_len_step.x;
    } else {                    // left
        position_step.x = -1;
        move_len.x      = (start.x - static_cast<float>(position.x)) * move_len_step.x;
    }

    if (move_len_normal.y > 0) { // down
        position_step.y = 1;
        move_len.y      = (static_cast<float>(position.y + 1) - start.y) * move_len_step.y;
    } else {                    // up
        position_step.y = -1;
        move_len.y      = (start.y - static_cast<float>(position.y)) * move_len_step.y;
    }

    // --- Ray cast

    bool end_of_line_x  = position.x == end_position.x;
    bool end_of_line_y  = position.y == end_position.y;
    bool end_of_line    = end_of_line_x and end_of_line_y;
    bool colision_found = colision_function(position);
    while (not (end_of_line or colision_found)) {
        if (end_of_line_y or (not end_of_line_x and move_len.x < move_len.y)) {
            position.x     += position_step.x;
            actual_distance = move_len.x;
            move_len.x     += move_len_step.x;
            end_of_line_x   = position.x == end_position.x;
        } else { // end_of_line_x or (not end_of_line_y and move_len.x > move_len.y)
            position.y     += position_step.y;
            actual_distance = move_len.y;
            move_len.y     += move_len_step.y;
            end_of_line_y   = position.y == end_position.y;
        }

        colision_found = colision_function(position);
        end_of_line    = end_of_line_x and end_of_line_y;
    }

    // --- Result

    if (not colision_found) return std::nullopt;

    start.x += move_len_normal.x * std::fabs(actual_distance);
    start.y += move_len_normal.y * std::fabs(actual_distance);

    return start;
}

float utils::smoth (float a, float b, float t, float max_difference) {
    float faster_t = t * 3;
    float x = (1 - faster_t) * a + b * faster_t;
    float dx = x - b;
    if (std::abs(dx) > max_difference) x = dx < 0 ? b - max_difference : b + max_difference;
    return x;
}