#ifndef NETWORK_H
#define NETWORK_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <optional>


inline uint64_t ntohll(uint64_t value) {
    return ((static_cast<uint64_t>(ntohl(value & 0xFFFFFFFF)) << 32) | ntohl(value >> 32));
}

inline uint32_t ntohi(uint32_t value) {
    return ((static_cast<uint32_t>(ntohs(value & 0xFFFF)) << 16) | ntohs(value >> 16));
}

inline uint32_t htoni(uint32_t value) {
    return ntohi(value);
}

// Helper to convert uint64_t network-ordered double to host-ordered double
inline double network_to_host_double(uint64_t net_double) {
    net_double = ntohll(net_double);  // Convert from network to host byte order
    return *reinterpret_cast<double*>(&net_double);
}

using id_game_t = unsigned long long;

struct Package {
public:
    enum class Type : std::int8_t {
        EMPTY         = -1,
        LOGIN         =  0,
        MESSAGE       =  1,
        GET_OTHER     =  2,
        FINISH        =  3,
        BREAK_SESSION =  4
    };
    inline static const int MIN_TYPE = 0;
    inline static const int MAX_TYPE = 4;

    struct Package_Payload {
        double x   = 0;
        double y   = 0;
        double dx  = 0;
        double dy  = 0;
        double ddx = 0;
        double ddy = 0;
        unsigned long long time = 0;
    };

public:
    Type type = Type::EMPTY;
    std::optional<id_game_t> id = std::nullopt;
    std::optional<Package_Payload> payload = std::nullopt;
};


struct Answer {
public:
    enum class Type : std::int8_t {
        ERROR_VALUE_INCORRECT = -2,
        BAD_FORMED            = -1,
        REGISTERED_ANSWER     = 0,
        ACKNOWLEDGE           = 1,
        OTHER                 = 2,
        FINISH                = 3,
        BREAK_SESSION         = 4,
    };
public:
    struct Finish {
        size_t x;
        size_t y;
        unsigned long long time;
        bool          has_finished;
    };
    struct Other_Payload {
        double x;
        double y;
        double dx;
        double dy;
        double ddx;
        double ddy;
        unsigned long long time = 0;
    };

    struct Other {
        Other_Payload payload;
        uint64_t id;
    };
public:
    Type type = Type::BAD_FORMED;
    std::optional<id_game_t> id = std::nullopt;
    std::optional<Finish> finish = std::nullopt;
    std::optional<std::vector<Other>> other = std::nullopt;
};


class Network {
public:
    Network ();
    ~Network ();
    Network (const Network& network) = delete;

    [[nodiscard]] bool is_connected () const;

public:
    std::optional<Answer>    get_answer         ();

    std::optional<bool>      send_connection    ();
    std::optional<bool>      send_disconnection ();
    std::optional<id_game_t> send_ask_other     ();
    std::optional<id_game_t> send_message       (const Package::Package_Payload& payload);
    std::optional<id_game_t> send_finish        (const Package::Package_Payload& payload);

public:
    static std::optional<std::map<id_game_t, Answer::Other_Payload>> parse_type_other_answer (const Answer& answer);

private:
    bool setup_socket ();
    void process_error ();
    void find_server ();


private:
    static bool decode_message (std::vector<uint8_t>& message);
    static std::optional<Answer> parse_answer (const std::vector<uint8_t>& message);

private:
    static bool encode_message (std::vector<uint8_t>& message);
    static void serialize_package (const Package& package, std::vector<uint8_t>& buffer); // helper for send_answer
    bool send_package (const Package& package);

private:
    addrinfo*     m_server_address;
    SOCKET        m_client_socket;
    unsigned long m_id;
    bool          m_is_socket_setuped      = false;
    bool          m_is_client_running      = true;
    bool          m_is_connected_to_server = false;
    id_game_t     m_next_package_id        = 0;
};


#endif // NETWORK_H