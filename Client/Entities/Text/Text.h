#ifndef ENTITY_TEXT_H
#define ENTITY_TEXT_H

#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdint>
#include <map>
#include <string>

class Entity_text {
public:
    enum class Text_font : uint8_t {
           Jersey10 = 1
    };

    // WARNING: should be updated on change of Text_font enum
    static const char* Text_font_to_string (Text_font font);

public:
    /**
     * @brief Size values are all in pixel
     */
    Entity_text  (std::string text, SDL_Color color, Text_font font,
                  unsigned long font_size, TTF_HorizontalAlignment alignment,
                  const SDL_Rect& allow_rectangle);
    // WARNING: this class manages lifetime of m_texture
    ~Entity_text ();
    Entity_text  (const Entity_text&) = delete;



public:
    void set_color           (const SDL_Color &color);
    void set_text            (const std::string &text);
    void set_font            (Text_font font);
    void set_font_size       (int size);
    void set_alignment       (TTF_HorizontalAlignment alignment);
    void set_text_area       (const SDL_Point& size);
    void set_position        (const SDL_Point& position);

public:
    SDL_Rect get_destination_rect ();
    /**
     * @brief Loads texture - if texture is already loaded, it is not reloaded
     */
    SDL_Texture* get_texture (SDL_Renderer *renderer);

private:
    inline static const std::map<Text_font, const char*> S_FONT_TO_PATH = {
        { Text_font::Jersey10, "assets/fonts/Jersey10-Regular.ttf" }
    };

private:
    /**
     * @brief should be called before printing
     */
    void evaluate_font_size ();

private:
    Text_font               m_font_type;
    TTF_Font*               m_font;
    unsigned long           m_font_size;

    SDL_Color               m_color;
    std::string             m_text;
    TTF_HorizontalAlignment m_alignment;

    SDL_Rect                m_allow_rectangle;
    SDL_Point               m_position;

    // on the contarary to other Entities, this texture's lifetime is managed by this class
    SDL_Texture*            m_texture;
    // set to false when any parameter, that influences texture appearance and isn't parameter of size_processed, is changed
    bool                    m_texture_appearance_processed;
    // set to false when any parameter that influences texture size is changed (text, font, font size, alignment, allow_rectangle)
    bool                    m_texture_size_processed;

private:
    void destroy_texture ();


};

#endif // TEXT_H