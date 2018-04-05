#include "luna_connection.hpp"


#include <cstdint>

#include <tls/standard_cookie.hpp>
#include <tls/standard_timer.hpp>
#include <tls/standard_cookie.hpp>

#include "binarystream.h"
#include "packets.h"


static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    std::cout << file << ", " << line << ", " << str << std::endl;
}

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

        if (!handshake_succesful) {
            std::cout << "Handshake failed." << std::endl;
            m_command_ssl.reset_session();
            continue;
        }
        std::cout << "Handshake successful." << std::endl;

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

        m_data_configuration.set_random_generator(&m_random);
        m_data_configuration.set_defaults(tls::endpoint::server, tls::transport::datagram, tls::preset::default_);
        static int const cipher_suites[] = {
            MBEDTLS_TLS_PSK_WITH_AES_128_CCM,
            0,
        };
        m_data_configuration.set_cipher_suites(cipher_suites);
        
        tls::standard_cookie cookie;
        cookie.setup(&m_random);
        m_data_configuration.set_dtls_cookies(&cookie);
        m_data_configuration.enable_debug(&my_debug, 2);
        
        char const identity[] = "Luna";
        m_data_configuration.set_shared_key(shared_key.data(),
                    shared_key.size(),
                    reinterpret_cast<uint8_t const*>(identity),
                    sizeof(identity));
                    
        tls::ssl data_ssl;
        data_ssl.setup(&m_data_configuration);

        auto port = std::to_string(static_cast<int>(base_port + 1));

        while (true) {
            tls::socket_input_output udp_socket;
            {
                tls::socket_input_output udp_listen_socket;
                std::cout << "Bind to " << client_address.to_string() << " : " << port << std::endl;
                udp_listen_socket.bind(nullptr, port.c_str(), tls::protocol::udp);  
                udp_listen_socket.accept(&udp_socket, &client_address);
                //socket.bind(client_address.to_string().c_str(), port.c_str(), tls::protocol::udp);
            }

            data_ssl.set_client_id(
                reinterpret_cast<unsigned char *>(client_address.data),
                client_address.size);
            data_ssl.set_input_output(&udp_socket);

            tls::standard_timer timer;
            data_ssl.set_timer(&timer);
            std::cout << "Handshaking DTLS" << std::endl;
            if (!data_ssl.handshake()) { 
                data_ssl.reset_session();
                continue;
            }
            std::cout << "Handshake done" << std::endl;

            for (int i = 0; i < 10; ++i) {
                std::stringstream ss;
                ss << "This is message number " << i;
                auto str = ss.str();
                data_ssl.write(str.c_str(), str.size() + 1);
                std::cout << "Wrote " << str;
            }
        }
    }
} 
