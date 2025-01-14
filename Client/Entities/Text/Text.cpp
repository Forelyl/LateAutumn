#include "Text.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include <iostream>
#include <limits>

const char* Entity_text::Text_font_to_string (Text_font font) {
    switch (font) {
        case Text_font::Jersey10:
            return "Jersey10";
        default:
            return nullptr;
    }
}

Entity_text::Entity_text  (std::string text, SDL_Color color, Text_font font, unsigned long font_size,
                           TTF_HorizontalAlignment alignment, const SDL_Rect& allow_rectangle)
  : m_font_type{font}, m_font{nullptr}, m_font_size{font_size},
  m_color(color), m_text{std::move(text)}, m_alignment{alignment},
  m_allow_rectangle{allow_rectangle}, m_texture{nullptr},
  m_texture_appearance_processed{false},
  m_texture_size_processed{false} {

    m_font = TTF_OpenFont(S_FONT_TO_PATH.at(m_font_type), static_cast<float>(m_font_size));
    if (m_font == nullptr) {
        std::cerr << "Error: font " << Text_font_to_string(font)  << " not found"
                  << "\nError: " << SDL_GetError() << '\n';
    }
    TTF_SetFontWrapAlignment(m_font, m_alignment);
}

Entity_text::~Entity_text () {
    destroy_texture();
    TTF_CloseFont(m_font);
}

void Entity_text::set_color(const SDL_Color &color) {
    if (color.a == m_color.a and color.b == m_color.b and
        color.g == m_color.g and color.r == m_color.r) return;

    m_texture_appearance_processed = false;
    m_color = color;
}

void Entity_text::set_text(const std::string &text) {
    if (m_text == text) return;

    m_texture_size_processed = false;
    m_text = text;
}

void Entity_text::set_font(Entity_text::Text_font font) {
    if (m_font_type == font) return;

    m_texture_appearance_processed = false;
    m_font_type = font;
}

void Entity_text::set_alignment (TTF_HorizontalAlignment alignment) {
    if (m_alignment == alignment) return;

    m_texture_appearance_processed = false;
    m_alignment = alignment;

    TTF_SetFontWrapAlignment(m_font, m_alignment);
}

void Entity_text::set_text_area (const SDL_Point& size) {
    m_allow_rectangle.w = size.x;
    m_allow_rectangle.h = size.y;
    m_texture_size_processed = false;
}

void Entity_text::set_position (const SDL_Point& position) {
    m_allow_rectangle.x = position.x;
    m_allow_rectangle.y = position.y;
    m_texture_appearance_processed = false;
}

SDL_Rect Entity_text::get_destination_rect () {
    if (m_texture_size_processed) return m_allow_rectangle;

    evaluate_font_size();
    m_texture_size_processed = true;
    return m_allow_rectangle;
}

void Entity_text::set_font_size (int size) {
    if (std::fabs(TTF_GetFontSize(m_font) - float(size)) <= std::numeric_limits<float>::epsilon()) return;

    m_texture_size_processed = false;
    TTF_SetFontSize(m_font, float(size));
}


void Entity_text::evaluate_font_size () {
    int w, h;
    TTF_GetStringSizeWrapped(m_font, m_text.c_str(), m_text.size(), m_allow_rectangle.w, &w, &h);

    // trying to get to ratio as close as possible to expected_area
    float ratio = std::max(float(w) / float(m_allow_rectangle.w), float(h) / float(m_allow_rectangle.h));
    m_allow_rectangle.h = h;

    if (ratio <= 1) return;

    m_font_size = static_cast<unsigned long>(float(m_font_size) * ratio);
    TTF_SetFontSize(m_font, float(m_font_size));
}

void Entity_text::destroy_texture () {
    if (m_texture == nullptr) return;
    SDL_DestroyTexture(m_texture);
    m_texture = nullptr;   // visual part
}

SDL_Texture* Entity_text::get_texture (SDL_Renderer *renderer) {
    if (m_texture_appearance_processed and m_texture_size_processed and m_texture != nullptr) return m_texture;
    if (m_texture != nullptr) destroy_texture();

    // size part
    evaluate_font_size();
    m_texture_size_processed = true;

    // visual part
    SDL_Surface* temp_surface = TTF_RenderText_Blended_Wrapped(m_font, m_text.c_str(),
                                                               m_text.size(), m_color,
                                                               m_allow_rectangle.w);
    // print parameters of temp_surface of function to check
    if (temp_surface == nullptr) {
        std::cerr << "Failed to create surface: " << SDL_GetError() << '\n';
        return nullptr;
    }
    m_texture_appearance_processed = true;

    // texture part
    m_texture = SDL_CreateTextureFromSurface(renderer, temp_surface);

    // cleanup
    SDL_DestroySurface(temp_surface);
    return m_texture;
}
