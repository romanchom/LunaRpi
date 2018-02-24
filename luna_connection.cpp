#include "luna_connection.hpp"

#include <arpa/inet.h>

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
    uint16_t message_length;
    char buffer[1024];
    int count;
    while (true) {
        count = m_command_ssl.read(reinterpret_cast<char *>(&message_length), sizeof(message_length));
        if (count != sizeof(message_length)) break;

        message_length = ntohs(message_length);
        count = m_command_ssl.read(buffer, sizeof(buffer));
        if (count != message_length) break;

        parse_command(buffer, count);
    }
}

void luna_connection::parse_command(char * data, int size) {
    for (int i = 0; i < size; ++i) {
        std::cout << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::endl;
}
