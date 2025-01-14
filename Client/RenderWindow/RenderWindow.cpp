#include "RenderWindow.h"
#include "Entity.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_video.h"
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <csetjmp>
#include <cstdlib>
#include <iostream>
#include <map>


RenderWindow::RenderWindow (const std::string& title, int width, int height) : m_next_id{0} {
    m_window = SDL_CreateWindow(title.c_str(), width, height, 0L);
    if (m_window == nullptr) {
        std::cerr << "Window can't be initialized: " << SDL_GetError() << '\n';
        exit(1);
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (m_renderer == nullptr) {
        std::cerr << "Renderer can't be initialized: " << SDL_GetError() << '\n';
        exit(1);
    }
    SDL_SetRenderVSync(m_renderer, 2);

}

RenderWindow::~RenderWindow () {
    SDL_DestroyWindow(m_window);
    SDL_DestroyRenderer(m_renderer);

    // clear textures
    for (auto texture : m_textures) {
        SDL_DestroyTexture(std::get<1>(texture));
    }
}

SDL_Texture* RenderWindow::load_texture (const std::string& path, unsigned long* id) {
    SDL_Texture* result = nullptr;
    result = IMG_LoadTexture(m_renderer, path.c_str());
    if (result == nullptr) {
        std::cerr << "Failed to load texture: " << SDL_GetError() << '\n';
        exit(1);
    }

    if (id != nullptr) *id = m_next_id;
    m_textures[m_next_id] = result;
    m_next_id++;

    return result;
}


SDL_Texture* RenderWindow::load_text_texture (const std::string& text, TTF_Font* font, SDL_Color color) {
    SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
	if (surf == nullptr) return nullptr;

	SDL_Texture *result = SDL_CreateTextureFromSurface(m_renderer, surf);
	SDL_DestroySurface(surf);

    if (result == nullptr) {
        return nullptr;
    }

    return result; // returned texture is managed by receiver
}

void RenderWindow::clear () {
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
}

void RenderWindow::draw_entity (const Entity* entity, const SDL_Point& disposition) {
    SDL_Rect source      = entity->get_source_rect();
    SDL_Rect destination = entity->get_destination_rect();
    destination.x += disposition.x;
    destination.y += disposition.y;

    SDL_FRect source_f = {
        static_cast<float>(source.x),
        static_cast<float>(source.y),
        static_cast<float>(source.w),
        static_cast<float>(source.h)
    };
    SDL_FRect destination_f = {
        static_cast<float>(destination.x),
        static_cast<float>(destination.y),
        static_cast<float>(destination.w),
        static_cast<float>(destination.h)
    };

    SDL_RenderTexture(m_renderer, entity->get_texture(), &source_f, &destination_f);
}


void RenderWindow::draw_entity (const Movable_entity* entity, const SDL_Point& disposition) {
    SDL_Rect source      = entity->get_source_rect();
    SDL_Rect destination = entity->get_destination_rect();
    destination.x -= disposition.x;
    destination.y -= disposition.y;
    destination.w  = destination.w;
    destination.h  = destination.h;

    SDL_FRect source_f = {
        static_cast<float>(source.x),
        static_cast<float>(source.y),
        static_cast<float>(source.w),
        static_cast<float>(source.h)
    };

    SDL_FRect destination_f = {
        static_cast<float>(destination.x),
        static_cast<float>(destination.y),
        static_cast<float>(destination.w),
        static_cast<float>(destination.h)
    };


    SDL_FlipMode to_flip  = entity->is_flipped() ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderTextureRotated(m_renderer, entity->get_texture(), &source_f, &destination_f, 0, nullptr, to_flip);
}

void RenderWindow::draw_entity (Entity_text* entity, const SDL_Point& disposition) {
    SDL_Rect destination = entity->get_destination_rect();
    destination.x -= disposition.x;
    destination.y -= disposition.y;
    // destination.w = std::floor(destination.w);
    // destination.h = std::floor(destination.h);

    SDL_FRect destination_f = {
        static_cast<float>(destination.x),
        static_cast<float>(destination.y),
        static_cast<float>(destination.w),
        static_cast<float>(destination.h)
    };

    std::cout << "texture: " << entity->get_texture(m_renderer) << '\n';
    SDL_RenderTexture(m_renderer, entity->get_texture(m_renderer), nullptr, &destination_f);
}

void RenderWindow::display () {
    SDL_RenderPresent(m_renderer);
}
float RenderWindow::get_refresh_rate () const {
    return SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(m_window))->refresh_rate;
}

SDL_Texture* RenderWindow::get_texture (unsigned long id) const {
   auto found = m_textures.find(id);
   return found == m_textures.end() ? nullptr : found->second;
}
