#ifndef NETWORK_H
#define NETWORK_H

#include <condition_variable>
#include <cstdint>
#include <inaddr.h>
#include <limits>
#include <list>
#include <mutex>
#include <string>
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

public:
    Type type               = Type::EMPTY;
    unsigned long long id   = std::numeric_limits<unsigned long long>::max();
    double x                = 0.0;
    double y                = 0.0;
    double dx               = 0.0;
    double dy               = 0.0;
    double ddx              = 0.0;
    double ddy              = 0.0;
    unsigned long long time = 0ll;
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
        unsigned long long x;
        unsigned long long y;
        unsigned long long time;
        bool has_finished;
    };
    struct Other {
        double x;
        double y;
        double dx;
        double dy;
        double ddx;
        double ddy;
        uint64_t id;
        unsigned long long time = 0;
    };
public:
    Type type;
    std::optional<unsigned long> id;
    std::optional<Finish> finish;
    std::optional<std::vector<Other>> other;
};

struct Network_package {
    Package package;
    std::string ip;
    std::string port;
};

struct Raw_message {
    std::vector<uint8_t> package;
    std::string ip;
    std::string port;
};

class Network {
public:
    Network ();
    ~Network ();
    Network (const Network& network) = delete;

public:
    void socket_main ();

public:
    [[nodiscard]] bool is_server_running() const;
    [[nodiscard]] bool has_message();

public:
    std::optional<Network_package> pop_message ();
    void stop_server ();

public:
    void registered_acknowledge  (const std::string& ip, const std::string& port);
    void acknowledge             (const std::string& ip, const std::string& port, uint64_t id);
    void response_bad_formed     (const std::string& ip, const std::string& port);
    void deleted_acknowledge     (const std::string& ip, const std::string& port);
    void send_info_of_other      (const std::string& ip, const std::string& port, uint64_t id, const std::vector<Answer::Other>& other);

private:
    bool setup_socket ();
    void process_error (const SOCKADDR_IN& client_address);

private:
    void push_message (std::string&& client, std::string&& port, std::vector<uint8_t>&& message);
    static bool decode_message (std::vector<uint8_t>& message);
    static std::optional<Package> parse_message (const std::vector<uint8_t>& message);

private:
    static bool encode_message (std::vector<uint8_t>& message);
    static void serialize_answer (const Answer& answer, std::vector<uint8_t>& buffer); // helper for send_answer
    void send_answer (const std::string& ip, const std::string& port, const Answer& answer);

private:
    SOCKET m_server_socket;
    std::mutex mutex_messages;
    std::condition_variable cv_has_message;
    std::list<Raw_message> messages;
    bool server_running = true;
};


#endif // NETWORK_H