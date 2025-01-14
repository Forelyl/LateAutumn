#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H
#include "Movable.h"
#include "SDL3/SDL_render.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <map>
#include <string>
#include "Entity.h"
#include "Movable.h"
#include "Text.h"
#include <SDL3_ttf/SDL_ttf.h>

class RenderWindow {
public:
    /**
     * @return SDL_Texture* lifetime is managed by RenderWindow
     */
    SDL_Texture* load_texture (const std::string& path, unsigned long* id = nullptr);

    /**
     * @brief
     * @return SDL_Texture* lifetime is managed by receiver
     */
    SDL_Texture* load_text_texture (const std::string& text, TTF_Font* font, SDL_Color color);

public:
    RenderWindow (const std::string& title, int width, int height);
    ~RenderWindow ();
    SDL_Texture* get_texture (unsigned long id) const;
    float get_refresh_rate () const;

public:
    void  draw_entity  (const Movable_entity* entity, const SDL_Point& disposition);
    void  draw_entity  (Entity_text* entity, const SDL_Point& disposition); // TODO: implement
    void  draw_entity  (const Entity* entity, const SDL_Point& disposition);

public:
    void  clear   ();
    void  display ();

private:
    SDL_Renderer* m_renderer = nullptr;
    SDL_Window*   m_window   = nullptr;
    std::map<unsigned long, SDL_Texture*> m_textures;
    unsigned long m_next_id;
};

#endif // RENDER_WINDOW_H
