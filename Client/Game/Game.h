#ifndef GAME_H
#define GAME_H

#include "Character_array.h"
#include "Network.h"
#include "Player.h"
#include <cfloat>
#include <cstdlib>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "Entity.h"
#include "RenderWindow.h"
#include "SDL3/SDL_render.h"
#include "Map.h"

class Game {

public:
    Game  ();
    ~Game ();

public:
    bool game_cycle ();

private: // WARNING: order of decklaration is important
    RenderWindow     m_window;
    Map              m_map;
    Player           m_player;
    Network          m_network;    // network input and output
    Character_array  m_characters; // encapsulates all movable entities and both network and keyboard inputs/outputs

private:
    static RenderWindow    setup_window        ();
    static Player          add_player          (RenderWindow& window, const Map& map);
    static Map             add_map             ();
    static Network         add_network         ();
    static Character_array add_character_array (RenderWindow& window, const Map& map, Player& player, Network& network);

private:
    bool has_pressed_up = false;

private:
    void write_text ();
    void connect_to_server ();


    SDL_Texture* texture = nullptr;
    Entity* back;
    void draw_background ();

    void draw ();

    /**
     * @brief returns false on SDL_EVENT_QUIT
     */
    bool handle_events ();
};




#endif // GAME_H