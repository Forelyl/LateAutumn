#include "Game.h"
#include "Network.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_timer.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "Text.h"
#include <iostream>



Game::Game () : m_window(setup_window()), m_map{add_map()},
                m_player{add_player(m_window, m_map)},
                m_network{add_network()},
                m_characters{add_character_array(m_window, m_map, m_player, m_network)} {

    m_map.init_textures(
        [&](const std::string& path, unsigned long* id){ return m_window.load_texture(path, id); }
    );
    draw_background();
}

Game::~Game () {
    delete back;

    SDL_Quit();
    TTF_Quit();
    IMG_Quit();
}


bool Game::game_cycle () {
    write_text(); // TODO: to remove
    connect_to_server();
    this->draw();

    auto start_game = static_cast<float>(SDL_GetTicks());
    m_characters.start_time();
    m_characters.start_network_threads();

    float refresh_rate = m_window.get_refresh_rate();
    float frame_time = 1000.0f / refresh_rate;
    auto frame_start_time = static_cast<float>(SDL_GetTicks());

    while (not m_player.has_won()) {

        // ---- Input
        bool to_stay_in_cycle = handle_events();
        if (not to_stay_in_cycle) break; // QUIT event
        m_characters.handle_keyboard_input();


        // ---- Process
        m_characters.update();

        // ------------- Drawing
        this->draw();

        // ------------- Refresh rate
        float frame_run_time = static_cast<float>(SDL_GetTicks()) - frame_start_time;
        frame_start_time     = static_cast<float>(SDL_GetTicks()); // update draw frame timer
        if (frame_time > frame_run_time) {
            SDL_Delay( static_cast<unsigned int>(frame_time - frame_run_time));
        }
    }

    m_characters.stop_time();
    m_characters.stop_network_threads();
    std::cout << "You have won in " << static_cast<float>(SDL_GetTicks()) - start_game << '\n';
    return false;
}


RenderWindow Game::setup_window () {
    if (not SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        std::cerr << "SDL init video was errornouse: "    << SDL_GetError() << '\n' << std::flush;
        exit(1);
    }
    if (not SDL_InitSubSystem(SDL_INIT_EVENTS)) {
        std::cerr << "SDL init events was errornouse: "   << SDL_GetError() << '\n' << std::flush;
        exit(1);
    }
    if (not IMG_Init(IMG_INIT_PNG)) {
        std::cerr << "SDL init img png was errornouse: "  << SDL_GetError() << '\n' << std::flush;
        exit(1);
    }
    if (not TTF_Init()) {
        std::cerr << "SDL init ttf was errornouse: " << SDL_GetError() << '\n' << std::flush;
        exit(1);
    }

    return RenderWindow{"Late autumn", 1300, 700};
}

Player Game::add_player (RenderWindow& window, const Map& map) {
    auto player_texture = window.load_texture(Player::INFO.texture_path);
    return Player{player_texture, map.get_player_start_position(), map};
}

Map Game::add_map () {
    return Map{"map1"};
}

Network Game::add_network () {
    return Network{};
}

Character_array Game::add_character_array (RenderWindow& window, const Map& map, Player& player, Network& network) {
    return Character_array{player, network, window, map};
}

void Game::connect_to_server () {
    const int font_size = 125;
    Entity_text text = Entity_text(
        "Trying to connect to server...",
        {210, 217, 218, 255},
        Entity_text::Text_font::Jersey10,
        font_size,
        TTF_HORIZONTAL_ALIGN_CENTER,
        {50, 0, 1200, 700}
    );
    auto allow_rectangle_processed = text.get_destination_rect();
    allow_rectangle_processed.y = (700 - allow_rectangle_processed.h) / 2;
    text.set_position({allow_rectangle_processed.x, allow_rectangle_processed.y});

    m_window.clear();
    m_window.draw_entity(&text, {0, 0});
    m_window.display();


    // -----------------

    m_network.send_connection();
    std::optional<Answer> answer = m_network.get_answer();

    for (int i = 0; i < 5 and not m_network.is_connected(); ++i) {
        std::cout << "Waiting for connection {" << i << "}\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        m_network.send_connection();
        answer = m_network.get_answer();
    }

    if (not answer.has_value()) {
        std::cerr << "No answer from server\n";
        exit(1);
    }
    if (answer.value().type != Answer::Type::REGISTERED_ANSWER) {
        std::cerr << "Server did not accept connection\n";
        exit(1);
    }

}

void Game::write_text () {
    const int font_size = 200;
    Entity_text text = Entity_text(
        "Hello warrior",
        {210, 217, 218, 255},
        Entity_text::Text_font::Jersey10,
        font_size,
        TTF_HORIZONTAL_ALIGN_CENTER,
        {50, 0, 1200, 700}
    );
    auto allow_rectangle_processed = text.get_destination_rect();
    allow_rectangle_processed.y = (700 - allow_rectangle_processed.h) / 2;
    text.set_position({allow_rectangle_processed.x, allow_rectangle_processed.y});

    m_window.draw_entity(&text, {0, 0});
    m_window.display();



    SDL_Event b;
    while (true) {
        SDL_WaitEvent(&b);
        if (b.type == SDL_EVENT_KEY_DOWN and b.key.key == SDLK_SPACE) {
            break;
        }
    }
    // Clean up
    return;
}


void Game::draw_background () {
    texture = m_window.load_texture("assets/tiles/city/Background1.png");
    back = new Entity{
        texture, {0, 0, 1800, 1200},
        {0, 0, 0, 0}, {0, 0}, {0, 10}
    };
}

void Game::draw () {
    m_window.clear();

    // FIXME remove
    m_window.draw_entity(back, {0, 0});


    m_characters.draw();
    m_map.draw(m_player.get_camera(),
               [&](const Entity* entity, const SDL_Point& disposition){
                   m_window.draw_entity(entity, disposition);
                }
    );
    m_window.display();
}


// returns false on SDL_EVENT_QUIT
bool Game::handle_events () {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) return false;

    }
    return true;
}
