#include "Network.h"
#include <cstdint>
#include <optional>
#include <syncstream>
#include <iostream>
#include <error_repairing.h>


Network::Network() : m_server_address{nullptr} {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::osyncstream(std::cerr) << "Socket startup failed: " << WSAGetLastError() << '\n';
        exit(1);
    }
    std::osyncstream(std::cout) << "Socket startup success" << '\n';
    find_server();
}

Network::~Network () {
    if (WSACleanup() != 0) {
        std::osyncstream(std::cerr) << "Socket shutdown failed: " << WSAGetLastError() << '\n';
        exit(1);
    }
    std::osyncstream(std::cout) << "Socket shutdown success" << '\n';

    if (m_server_address != nullptr) freeaddrinfo(m_server_address);
}

bool Network::is_connected () const { return m_is_connected_to_server; }

bool Network::setup_socket () {
    if (m_is_socket_setuped) return true;

    // get server info
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;     // AF_INET or AF_INET6 (IPv4 or IPv6)
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    hints.ai_protocol = 0;           // use default protocol implementation for UDP
    hints.ai_flags    = AI_PASSIVE;  // used resolution for socket creation and binding
    addrinfo* result;
    int error = getaddrinfo("localhost", nullptr, &hints, &result);
    if (error != 0) {
        std::osyncstream(std::cerr) << "Self address resolution failed: " << gai_strerror(error) << '\n';
        return false;
    }

    // UDP socket
    m_client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_client_socket == INVALID_SOCKET) {
        std::osyncstream(std::cerr) << "Socket creation failed: " << WSAGetLastError() << '\n';
        freeaddrinfo(result);
        closesocket(m_client_socket);
        return false;
    }

    // binding
    error = bind(m_client_socket, result->ai_addr, int(result->ai_addrlen));
    freeaddrinfo(result); // freeing memory - unneaded after binding
    if (error == SOCKET_ERROR) {
        std::osyncstream(std::cerr) << "Socket binding failed: " << WSAGetLastError() << '\n';
        closesocket(m_client_socket);
        return false;
    }

    m_is_socket_setuped = true;
    return true;
}

void Network::find_server () {
    // get server info
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;     // AF_INET or AF_INET6 (IPv4 or IPv6)
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    hints.ai_protocol = 0;           // use default protocol implementation for UDP
    hints.ai_flags    = AI_PASSIVE;  // used resolution for socket creation and binding
    int error = getaddrinfo("127.0.0.1", "20123", &hints, &m_server_address);
    if (error != 0) {
        std::osyncstream(std::cerr) << "Self address resolution failed: " << gai_strerror(error) << '\n';
        exit(1);
    }
}

void Network::process_error () {
    int wsa_error = WSAGetLastError();

    // this seems to be get when client is disconnects - in connection less protocol ;(
    if (wsa_error == 10054) return; // FIXME try to find a const

    std::osyncstream(std::cerr) << "Socket receive failed: " << wsa_error << '\n' << '\n';
}


// ========================================================
// receive data
// ========================================================
bool Network::decode_message (std::vector<uint8_t>& message) {
    return custom_utils::decode_package(message);
}

std::optional<Answer> Network::parse_answer (const std::vector<uint8_t>& message) {
    // Ensure that the message size is exactly what we expect
    if (message.size() == 0) {
        return std::nullopt;
    }

    // Ensure the message size is valid
    if (message.size() < sizeof(uint8_t)) {
        return Answer{.type=Answer::Type::BAD_FORMED};
    }

    Answer answer;
    uint64_t llong_value;
    size_t data_disposition = 0;
    const uint8_t* data = message.data();

    // 1. Type (1 byte)
    answer.type = static_cast<Answer::Type>(data[0]);
    if (int(answer.type) < int(Answer::Type::BAD_FORMED) || int(answer.type) > int(Answer::Type::BREAK_SESSION)) {
        return Answer{.type=Answer::Type::BAD_FORMED};
    }
    data_disposition += sizeof(uint8_t);

    // Handle specific answer types
    switch (answer.type) {
        case Answer::Type::FINISH: {
            // 2. Finish (x: 8 bytes, y: 8 bytes, time: 8 bytes, has_finished: 1 byte)
            if (message.size() != data_disposition + sizeof(uint64_t) * 3 + sizeof(uint8_t)) {
                return Answer{.type=Answer::Type::BAD_FORMED};
            }

            Answer::Finish finish;

            memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
            llong_value = ntohll(llong_value);
            finish.x = static_cast<uint64_t>(llong_value);
            data_disposition += sizeof(uint64_t);

            memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
            finish.y = ntohll(llong_value);
            data_disposition += sizeof(uint64_t);

            memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
            finish.time = ntohll(llong_value);
            data_disposition += sizeof(uint64_t);

            finish.has_finished = static_cast<bool>(data[data_disposition]);
            data_disposition += sizeof(uint8_t);

            answer.finish = finish;
            break;
        }
        case Answer::Type::OTHER: {
            // 3. Other (id: 8 bytes, cound: 8 bytes, x/y/dx/dy/ddx/ddy: 6 * 8 bytes each)
            if (message.size() < data_disposition + sizeof(uint64_t)) { // has id
                return Answer{.type=Answer::Type::BAD_FORMED};
            }

            memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
            answer.id = ntohll(llong_value);
            data_disposition += sizeof(uint64_t);

            // ---

            if (message.size() < data_disposition + sizeof(uint64_t)) { // has count
                return Answer{.type=Answer::Type::BAD_FORMED};
            }

            uint64_t count_net;
            memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
            count_net = ntohll(llong_value);
            size_t count = *reinterpret_cast<size_t*>(&count_net);
            data_disposition += sizeof(uint64_t);

            // sizeof(Answer::Other) = sizeof(uint64_t) + 6 * sizeof(double)
            if (message.size() != data_disposition + count * sizeof(Answer::Other)) { // enough data for count
                return Answer{.type=Answer::Type::BAD_FORMED};
            }

            std::vector<Answer::Other> others;
            others.reserve(count);

            for (uint64_t i = 0; i < count; ++i) {
                Answer::Other other;

                memcpy(&llong_value, data + data_disposition, sizeof(uint64_t));
                other.id = ntohll(llong_value);
                data_disposition += sizeof(uint64_t);

                memcpy(&llong_value, data + data_disposition, sizeof(uint64_t));
                other.payload.x = network_to_host_double(llong_value);
                data_disposition += sizeof(uint64_t);

                memcpy(&llong_value, data + data_disposition, sizeof(uint64_t));
                other.payload.y = network_to_host_double(llong_value);
                data_disposition += sizeof(uint64_t);

                memcpy(&llong_value, data + data_disposition, sizeof(uint64_t));
                other.payload.dx = network_to_host_double(llong_value);
                data_disposition += sizeof(uint64_t);

                memcpy(&llong_value, data + data_disposition, sizeof(uint64_t));
                other.payload.dy = network_to_host_double(llong_value);
                data_disposition += sizeof(uint64_t);

                memcpy(&llong_value, data + data_disposition, sizeof(uint64_t));
                other.payload.ddx = network_to_host_double(llong_value);
                data_disposition += sizeof(uint64_t);

                memcpy(&llong_value, data + data_disposition, sizeof(uint64_t));
                other.payload.ddy = network_to_host_double(llong_value);
                data_disposition += sizeof(uint64_t);

                memcpy(&llong_value, data + data_disposition, sizeof(uint64_t));
                other.payload.time = ntohll(llong_value);
                data_disposition += sizeof(uint64_t);

                others.push_back(other);
            }

            answer.other = others;
            break;
        }
        case Answer::Type::ACKNOWLEDGE: {
            // 4. ID (8 bytes)
            if (message.size() != data_disposition + sizeof(uint64_t)) {
                return Answer{.type=Answer::Type::BAD_FORMED};
            }
            memcpy(&llong_value, data + data_disposition, sizeof(llong_value));
            answer.id = ntohll(llong_value);

            break;
        }
        case Answer::Type::REGISTERED_ANSWER: {
            if (message.size() != data_disposition) {
                return Answer{.type=Answer::Type::BAD_FORMED};
            }
            break;
        }
        default: { break; }
    }

    return answer;
}

// ========================================================
// send data
// ========================================================


bool Network::encode_message (std::vector<uint8_t>& message) {
    return custom_utils::encode_package(message);
}

void Network::serialize_package (const Package& package, std::vector<uint8_t>& buffer) {
    uint8_t type = *reinterpret_cast<const uint8_t*>(&package.type);
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&type), reinterpret_cast<uint8_t*>(&type) + sizeof(type));

    // due to how type works - in current implementation, only one payload can be in the answer
    if (package.id.has_value()) {
        uint64_t id = ntohll(package.id.value());
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&id), reinterpret_cast<uint8_t*>(&id) + sizeof(id));
    }

    if (package.payload.has_value()) {
        uint64_t d_value;
        d_value = ntohll(*reinterpret_cast<const unsigned long long*>(&package.payload.value().x));
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&d_value), reinterpret_cast<uint8_t*>(&d_value) + sizeof(d_value));
        d_value = ntohll(*reinterpret_cast<const unsigned long long*>(&package.payload.value().y));
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&d_value), reinterpret_cast<uint8_t*>(&d_value) + sizeof(d_value));

        d_value = ntohll(*reinterpret_cast<const unsigned long long*>(&package.payload.value().dx));
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&d_value), reinterpret_cast<uint8_t*>(&d_value) + sizeof(d_value));
        d_value = ntohll(*reinterpret_cast<const unsigned long long*>(&package.payload.value().dy));
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&d_value), reinterpret_cast<uint8_t*>(&d_value) + sizeof(d_value));

        d_value = ntohll(*reinterpret_cast<const unsigned long long*>(&package.payload.value().ddx));
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&d_value), reinterpret_cast<uint8_t*>(&d_value) + sizeof(d_value));
        d_value = ntohll(*reinterpret_cast<const unsigned long long*>(&package.payload.value().ddy));
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&d_value), reinterpret_cast<uint8_t*>(&d_value) + sizeof(d_value));

        uint64_t time = package.payload.value().time;
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&time), reinterpret_cast<uint8_t*>(&time) + sizeof(time));
    }
}

bool Network::send_package (const Package& package) {
    // setup
    if (not setup_socket()) {
        std::osyncstream(std::cerr) << "Socket wasn't setuped before getting answer (internal error - send)" << '\n';
        exit(1);
    }

    // prepare data
    std::vector<uint8_t> buffer;
    serialize_package(package, buffer);
    if (not encode_message(buffer)) {
        std::osyncstream(std::cerr) << "Failed to encode message - inner error\n";
        exit(1);
    }

    // FIXME case of long buffer - int overflow
    int error = sendto(m_client_socket, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0, m_server_address->ai_addr, static_cast<int>(m_server_address->ai_addrlen));
    if (error == SOCKET_ERROR) {
        std::osyncstream(std::cerr) << "Failed to send data to server: " << WSAGetLastError() << '\n';
        return false;
    }

    return true;
}

// ========================================================
// data receivers
// ========================================================

std::optional<Answer> Network::get_answer () {
    // setup
    if (not setup_socket()) {
        std::osyncstream(std::cerr) << "Socket wasn't setuped before getting answer (internal error - answer)" << '\n';
        exit(1);
    }

    // get data cycle
    int server_address_size = sizeof(*m_server_address->ai_addr);
    static std::array<char, 1024> buffer;
    constexpr int buffer_size = static_cast<int>(buffer.size());

    int data_size = recvfrom(m_client_socket, buffer.data(), buffer_size,
                             0, m_server_address->ai_addr, &server_address_size);

    if (data_size == SOCKET_ERROR) {
        process_error();
        return std::nullopt;
    }

    // process data
    std::vector<uint8_t> data(buffer.data(), buffer.data() + static_cast<size_t>(data_size));
    decode_message(data);
    std::optional<Answer> answer = parse_answer(data);

    if (not answer.has_value()) return answer; // error - return

    // check if connected (registered) or disconnected (break session)
    if (answer->type == Answer::Type::REGISTERED_ANSWER) {
        m_is_connected_to_server = true;
    } else if (answer->type == Answer::Type::BREAK_SESSION) {
        m_is_connected_to_server = false;
    }

    return answer;
}

// ========================================================
// data senders
// ========================================================

std::optional<bool> Network::send_connection () {
    if (m_is_connected_to_server) return std::nullopt;

    // make data for connection
    Package data {.type=Package::Type::LOGIN};

    // send
    return send_package(data);
}

std::optional<bool> Network::send_disconnection () {
    if (not m_is_connected_to_server) return std::nullopt;

    // make data for disconnection
    Package data {.type=Package::Type::BREAK_SESSION};

    // send
    return send_package(data);

}

std::optional<id_game_t> Network::send_ask_other () {
    if (not m_is_connected_to_server) return std::nullopt;

    // make data for ask other
    Package data {.type=Package::Type::GET_OTHER, .id=m_next_package_id};
    ++m_next_package_id;

    // send
    if (not send_package(data)) return std::nullopt;

    return m_next_package_id - 1;
}
std::optional<id_game_t> Network::send_message (const Package::Package_Payload& payload) {
    if (not m_is_connected_to_server) return std::nullopt;

    // make data for message
    Package data {.type=Package::Type::MESSAGE, .id=m_next_package_id, .payload=payload};
    ++m_next_package_id;

    // send
    if (not send_package(data)) return std::nullopt;

    return m_next_package_id - 1;
}
std::optional<id_game_t> Network::send_finish (const Package::Package_Payload& payload) {
    if (not m_is_connected_to_server) return std::nullopt;

    // make data for finish
    Package data {.type=Package::Type::FINISH, .id=m_next_package_id, .payload=payload};
    ++m_next_package_id;

    // send
    if (not send_package(data)) return std::nullopt;

    return m_next_package_id - 1;
}


// ========================================================
// helpers
// ========================================================

std::optional<std::map<id_game_t, Answer::Other_Payload>> Network::parse_type_other_answer (const Answer& answer) {
    if (answer.type != Answer::Type::OTHER) return std::nullopt;
    if (not answer.other.has_value())       return std::nullopt;

    std::map<id_game_t, Answer::Other_Payload> result;
    for (const auto& other : answer.other.value()) {
        result.insert({other.id, other.payload});
    }

    return result;
}
