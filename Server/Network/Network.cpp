#include "Network.h"
#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <array>
#include <winsock2.h>
#include <syncstream>
#include <ws2ipdef.h>
#include <error_repairing.h>

using std::to_string;

Network::Network() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::osyncstream(std::cerr) << "Socket startup failed: " << WSAGetLastError() << '\n';
        exit(1);
    }
    std::osyncstream(std::cout) << "Socket startup success" << '\n';
}

Network::~Network () {
    if (WSACleanup() != 0) {
        std::osyncstream(std::cerr) << "Socket shutdown failed: " << WSAGetLastError() << '\n';
        exit(1);
    }
    std::osyncstream(std::cout) << "Socket shutdown success" << '\n';
}

bool Network::setup_socket () {
    // get server info
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;     // AF_INET or AF_INET6 (IPv4 or IPv6)
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    hints.ai_protocol = 0;           // use default protocol implementation for UDP
    hints.ai_flags    = AI_PASSIVE;  // used resolution for socket creation and binding
    addrinfo* result;
    int error = getaddrinfo(nullptr, "20123", &hints, &result);
    if (error != 0) {
        std::osyncstream(std::cerr) << "Self address resolution failed: " << gai_strerror(error) << '\n';
        return false;
    }

    // UDP socket
    m_server_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_server_socket == INVALID_SOCKET) {
        std::osyncstream(std::cerr) << "Socket creation failed: " << WSAGetLastError() << '\n';
        freeaddrinfo(result);
        closesocket(m_server_socket);
        return false;
    }

    // set socket options
    int opt = 1;
    if (setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        std::osyncstream(std::cerr) << "Socket modification failed: " << WSAGetLastError() << '\n';
        freeaddrinfo(result);
        closesocket(m_server_socket);
        return false;
    }

    // binding
    error = bind(m_server_socket, result->ai_addr, int(result->ai_addrlen));
    freeaddrinfo(result); // freeing memory - unneaded after binding
    if (error == SOCKET_ERROR) {
        std::osyncstream(std::cerr) << "Socket binding failed: " << WSAGetLastError() << '\n';
        closesocket(m_server_socket);
        return false;
    }

    return true;
}

void Network::process_error (const SOCKADDR_IN& client_address) {
    int wsa_error = WSAGetLastError();

    // this seems to be get when client is disconnects - in connection less protocol ;(
    if (wsa_error == 10054) return; // FIXME try to find a const

    std::osyncstream(std::cerr) << "Socket receive failed: " << wsa_error << '\n' << '\n';
    response_bad_formed(inet_ntoa(client_address.sin_addr), std::to_string(client_address.sin_port));
}

void Network::socket_main () {
    // setup
    if (not setup_socket()) return;

    // get data cycle
    SOCKADDR_IN client_address;
    int client_address_size = sizeof(client_address);
    std::array<char, 1024> buffer;
    int buffer_size = static_cast<int>(buffer.size());

    while (server_running) {
        memset(&client_address, 0, sizeof(client_address));
        int data_size = recvfrom(m_server_socket, buffer.data(), buffer_size, 0, (SOCKADDR*)&client_address, &client_address_size);
        client_address.sin_port = htons(client_address.sin_port);

        if (data_size == SOCKET_ERROR) {
            process_error(client_address);
            continue;
        }

        push_message(inet_ntoa(client_address.sin_addr),
                     to_string(client_address.sin_port),
                     std::vector<uint8_t>(buffer.data(), buffer.data() + static_cast<size_t>(data_size)));
    }

    // cleaning
    closesocket(m_server_socket);
}


// ===================================
// start of thread-made functions
// ===================================

bool Network::is_server_running() const {
    return server_running;
}

void Network::stop_server () {
    if (not server_running) return;
    server_running = false;
}


bool Network::has_message() {
    return not messages.empty();
}

bool Network::decode_message (std::vector<uint8_t>& message) {
    return custom_utils::decode_package(message);
}

std::optional<Network_package> Network::pop_message () {
    std::unique_lock<std::mutex> lock(mutex_messages); // use message queue

    cv_has_message.wait(lock, [this] { return not messages.empty() or not server_running; });
    if (messages.empty()) { // server was stopped
        std::osyncstream(std::cout) << "Attempt to pop message from empty queue when server was stopped" << '\n';
        return std::nullopt;
    }

    Raw_message raw_message = messages.front();
    messages.pop_front();

    // decode
    bool was_decoded = decode_message(raw_message.package);
    if (not was_decoded) {
        // change to error
        response_bad_formed(raw_message.ip, raw_message.port);
        return std::nullopt;
    }

    // parse
    std::optional<Package> package = parse_message(raw_message.package);
    if (not package.has_value()) {
        // change to error
        response_bad_formed(raw_message.ip, raw_message.port);
        return std::nullopt;
    };

    return Network_package{.package=package.value(), .ip=std::move(raw_message.ip), .port=std::move(raw_message.port)};
}

// TODO: make atomic dequeue
void Network::push_message (std::string&& client, std::string&& port, std::vector<uint8_t>&& message) {
    std::lock_guard<std::mutex> lock(mutex_messages);
    messages.emplace_back(std::move(message), std::move(client), std::move(port));
    cv_has_message.notify_one();
    return;
}

// ===================================
// end of thread-made functions
// ===================================

void Network::registered_acknowledge  (const std::string& ip, const std::string& port) {
    Answer answer;
    answer.type     = Answer::Type::REGISTERED_ANSWER;
    answer.finish   = std::nullopt;
    answer.other    = std::nullopt;
    answer.id       = std::nullopt;

    send_answer(ip, port, answer);
}

void Network::acknowledge (const std::string& ip,  const std::string& port, uint64_t id) {
    Answer answer;
    answer.type     = Answer::Type::ACKNOWLEDGE;
    answer.finish   = std::nullopt;
    answer.other    = std::nullopt;
    answer.id       = id;

    send_answer(ip, port, answer);
}

void Network::response_bad_formed (const std::string& ip, const std::string& port) {
    Answer answer;
    answer.type     = Answer::Type::BAD_FORMED;
    answer.finish   = std::nullopt;
    answer.other    = std::nullopt;
    answer.id       = std::nullopt;
    send_answer(ip, port, answer);
}

void Network::deleted_acknowledge (const std::string& ip, const std::string& port) {
    Answer answer;
    answer.type     = Answer::Type::BREAK_SESSION;
    answer.finish   = std::nullopt;
    answer.other    = std::nullopt;
    answer.id       = std::nullopt;
    send_answer(ip, port, answer);
}

void Network::send_info_of_other (const std::string& ip, const std::string& port, uint64_t id, const std::vector<Answer::Other>& other) {
    Answer answer;
    answer.type     = Answer::Type::OTHER;
    answer.finish   = std::nullopt;
    answer.other    = other;
    answer.id       = id;

    if (other.empty()) answer.other = std::nullopt;

    for (const auto& x : other) {
        std::osyncstream(std::cout) << "Sending info of other: " << x.id << '\n';
    }

    send_answer(ip, port, answer);
}

std::optional<Package> Network::parse_message (const std::vector<uint8_t>& message) {
    // Ensure that the message size is exactly what we expect - must be less than max amount of data
    if (message.size() > sizeof(uint8_t) + 2 * sizeof(uint64_t) + 6 * sizeof(double) or message.size() == 0) {
        return std::nullopt;
    }

    Package package;
    uint64_t llong_value;
    size_t data_disposition = 0;
    const uint8_t* data = message.data();

    // 1. Type (1 byte)
    if (data_disposition + sizeof(uint8_t) > message.size()) return std::nullopt; // no enough data left
    package.type = static_cast<Package::Type>(data[0]);
    if (int(package.type) < Package::MIN_TYPE || int(package.type) > Package::MAX_TYPE) {
        return std::nullopt;
    }
    data_disposition += sizeof(uint8_t);

    // End types - types without additional payload
    bool is_type_only_package = (
        (package.type == Package::Type::BREAK_SESSION) or
        (package.type == Package::Type::LOGIN        )
    );

    if (is_type_only_package) return package;

    // 2. Id (8 bytes, network to host order)
    if (data_disposition + sizeof(uint64_t) > message.size()) return std::nullopt; // no enough data left
    memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
    package.id = static_cast<unsigned long>(ntohll(llong_value));
    data_disposition += sizeof(uint64_t);

    // Id package - packages with type and id
    if (package.type == Package::Type::GET_OTHER) return package;

    // [3, 9]
    if (data_disposition + 6 * sizeof(double) + sizeof(uint64_t) != message.size()) return std::nullopt; // no correct amount of data

    // 3. x (8 bytes, double in network byte order)
    memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
    package.x = network_to_host_double(llong_value);
    data_disposition += sizeof(double);

    // 4. y (8 bytes, double in network byte order)
    memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
    package.y = network_to_host_double(llong_value);
    data_disposition += sizeof(double);

    // 5. dx (8 bytes, double in network byte order)
    memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
    package.dx = network_to_host_double(llong_value);
    data_disposition += sizeof(double);

    // 6. dy (8 bytes, double in network byte order)
    memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
    package.dy = network_to_host_double(llong_value);
    data_disposition += sizeof(double);

    // 7. ddx (8 bytes, double in network byte order)
    memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
    package.ddx = network_to_host_double(llong_value);
    data_disposition += sizeof(double);

    // 8. ddy (8 bytes, double in network byte order)
    memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
    package.ddy = network_to_host_double(llong_value);
    data_disposition += sizeof(double);

    // 9. timestamp (8 bytes, unsigned long long in network order)
    memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
    package.time = static_cast<unsigned long>(ntohll(llong_value));
    // Unneaded: data_disposition += sizeof(uint64_t);

    return package;
}

bool Network::encode_message (std::vector<uint8_t>& message) {
    return custom_utils::encode_package(message);
}

void Network::serialize_answer (const Answer& answer, std::vector<uint8_t>& buffer) {
    uint8_t type = *reinterpret_cast<const uint8_t*>(&answer.type);
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&type), reinterpret_cast<uint8_t*>(&type) + sizeof(type));


    if (answer.id.has_value()) {
        uint64_t id = ntohll(answer.id.value());
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&id), reinterpret_cast<uint8_t*>(&id) + sizeof(id));
    }
    if (answer.finish.has_value()) {
        uint8_t finish = answer.finish->has_finished;
        buffer.push_back(finish);

        uint64_t x = ntohll(answer.finish->x);
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&x), reinterpret_cast<uint8_t*>(&x) + sizeof(x));

        uint64_t y = ntohll(answer.finish->y);
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&y), reinterpret_cast<uint8_t*>(&y) + sizeof(y));

        uint64_t time = ntohll(answer.finish->time);
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&time), reinterpret_cast<uint8_t*>(&time) + sizeof(time));
    }
    if (answer.other.has_value()) {
        uint64_t size = ntohll(answer.other.value().size());
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&size), reinterpret_cast<uint8_t*>(&size) + sizeof(size));

        for (const auto& other : answer.other.value()) {
            uint64_t value = ntohll(other.id);
            buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&value), reinterpret_cast<uint8_t*>(&value) + sizeof(value));

            value = ntohll(*reinterpret_cast<const unsigned long long*>(&other.x));
            buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&value), reinterpret_cast<uint8_t*>(&value) + sizeof(value));
            value = ntohll(*reinterpret_cast<const unsigned long long*>(&other.y));
            buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&value), reinterpret_cast<uint8_t*>(&value) + sizeof(value));

            value = ntohll(*reinterpret_cast<const unsigned long long*>(&other.dx));
            buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&value), reinterpret_cast<uint8_t*>(&value) + sizeof(value));
            value = ntohll(*reinterpret_cast<const unsigned long long*>(&other.dy));
            buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&value), reinterpret_cast<uint8_t*>(&value) + sizeof(value));

            value = ntohll(*reinterpret_cast<const unsigned long long*>(&other.ddx));
            buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&value), reinterpret_cast<uint8_t*>(&value) + sizeof(value));
            value = ntohll(*reinterpret_cast<const unsigned long long*>(&other.ddy));
            buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&value), reinterpret_cast<uint8_t*>(&value) + sizeof(value));

            value = ntohll(*reinterpret_cast<const unsigned long long*>(&other.time));
            buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&value), reinterpret_cast<uint8_t*>(&value) + sizeof(value));
        }
    }
    // TODO: add check on size of array
}

void Network::send_answer (const std::string& ip, const std::string& port, const Answer& answer) {
    // prepare data
    std::vector<uint8_t> buffer;


    serialize_answer(answer, buffer);
    if (not encode_message(buffer)) {
        response_bad_formed(ip, port);
        return;
    }

    // get user info
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;   // AF_INET or AF_INET6 (IPv4 or IPv6)
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    hints.ai_protocol = 0;           // use default protocol implementation for UDP
    addrinfo* result;
    int error = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);
    if (error != 0) {
        std::osyncstream(std::cerr) << "User address resolution failed: " << gai_strerror(error) << '\n';
        return;
    }

    // FIXME case of long buffer - int overflow
    error = sendto(m_server_socket, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0, result->ai_addr, static_cast<int>(result->ai_addrlen));
    freeaddrinfo(result);
    if (error == SOCKET_ERROR) {
        std::osyncstream(std::cerr) << "Failed to send data to user: " << WSAGetLastError() << '\n';
        return;
    }
}
