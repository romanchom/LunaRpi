#include "discovery_responder.hpp"

#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

discovery_responder::discovery_responder(uint16_t listening_port, const std::string & response) :
    m_should_run(true),
    m_thread([this, listening_port, response](){ task(listening_port, response); })
{}

discovery_responder::~discovery_responder() {
    m_should_run.store(false);
    m_thread.join();
}

void discovery_responder::task(uint16_t listening_port, const std::string & response) {
    std::cout << "Starting discovery task" << std::endl;
    
    int discovery_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    std::cout << "Created discovery socket" << std::endl;
    
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    int error = setsockopt(discovery_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    if (0 != error) {
        printf("Unable to enable timeout: %d\n", errno);
        close(discovery_socket);
        return;
    }
    std::cout << "Set discovery read timeout" << std::endl;

    sockaddr_in6 bind_address;
    bind_address.sin6_family = AF_INET6;
    bind_address.sin6_port = htons(listening_port);
    bind_address.sin6_addr  = IN6ADDR_ANY_INIT;

    error = bind(discovery_socket, reinterpret_cast<sockaddr *>(&bind_address), sizeof(sockaddr_in6));
    if (0 != error) {
        printf("Unable to bind: %d\n", errno);
        close(discovery_socket);
        return;
    }
    std::cout << "Bound discovery task to port " << listening_port << std::endl;

    std::cout << "Entering discovery loop" << std::endl;
    while (m_should_run.load()) {
        sockaddr_in6 from_address;
        auto address_size = static_cast<socklen_t>(sizeof(sockaddr_in6));
        char buffer[1024];
        
        auto message_length = recvfrom(discovery_socket, buffer, sizeof(buffer), 0,
            reinterpret_cast<sockaddr *>(&from_address), &address_size);

        if (message_length < 0) {
            error = errno;
            if (error == EAGAIN || error == EWOULDBLOCK) {
                continue;
            } else {
                std::cout << "Socket recvfrom error " << error << std::endl;    
                break;
            }
        }
        
        std::cout << "Got message " << buffer << std::endl;
        
        auto sent_length = sendto(discovery_socket, response.data(), response.size(), 0,
            reinterpret_cast<sockaddr *>(&from_address), address_size);
        if (sent_length != response.size()) {
            std::cout << "Socket sentto error " << errno << std::endl;
            break;
        }
    }

    close(discovery_socket);
}
