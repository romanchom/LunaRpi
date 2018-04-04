#include "luna_connection.hpp"


#include <cstdint>
#include "binarystream.h"
#include "packets.h"

luna_connection::luna_connection(const std::string & certificate_directory) {
    m_random.seed(&m_entropy, "Some random string", 6);
    
    std::string path = certificate_directory + "/ca.crt";
    m_ca_certificate.parse_file(path.c_str());
    m_command_configuration.set_certifiate_authority_chain(&m_ca_certificate);

    path = certificate_directory + "/own.crt";
    m_own_certificate.parse_file(path.c_str());
    
    path = certificate_directory + "/own.key";
    m_own_private_key.parse_file(path.c_str(), nullptr);
    m_command_configuration.set_own_certificate(&m_own_certificate, &m_own_private_key);

    m_command_configuration.set_defaults(tls::endpoint::server, tls::transport::stream, tls::preset::default_);
    //m_command_configuration.set_authentication_mode(tls::authentication_mode::none);
    m_command_configuration.set_authentication_mode(tls::authentication_mode::required);
    m_command_configuration.set_random_generator(&m_random);

    m_command_ssl.setup(&m_command_configuration);
}

luna_connection::~luna_connection() {

}


void luna_connection::listen(uint16_t base_port) {
    std::cout << "Starting listening for incloming connections" << std::endl;
    auto command_port = std::to_string(static_cast<int>(base_port));

    while (true) {
        tls::address client_address;
        {
            tls::socket_input_output listening_socket;
            listening_socket.bind(nullptr, command_port.c_str(), tls::protocol::tcp);
            std::cout << "Bound to command socket" << std::endl;
            listening_socket.accept(&m_command_socket, &client_address);
            std::cout << "Accepted command stream" << std::endl;
        }
        std::cout << "Client requests connection" << std::endl;
        m_command_ssl.set_input_output(&m_command_socket);
            
        std::cout << "Performing handshake" << std::endl;
        auto handshake_succesful = m_command_ssl.handshake();

        if (handshake_succesful) {
            std::cout << "Handshake successful" << std::endl;
            read_commands();

        }
        m_command_ssl.reset_session();
    }
}

void luna_connection::read_commands() {
    std::vector<uint8_t> shared_key(16);
    m_random.generate(shared_key.data(), shared_key.size());

    BinaryStream stream(&m_command_ssl);
    
    Request request;
    stream >> request;
    std::cout << "Request " << static_cast<int>(request) << std::endl;
    if (Request::configuration == request) {
        std::cout << "Making response" << std::endl;
        stream << request;
        ConfigurationResponse response = {
            {
                {7, 120, {-100, -100, 0}, {-100, 100, 0}},
                {7, 120, {100, -100, 0}, {100, 100, 0}},
            },
            shared_key,
        };
        stream << response;
        
        std::cout << "Sent response" << std::endl;
    }
}
