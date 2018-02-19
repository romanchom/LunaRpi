#pragma once

#include <string>

#include <tls/standard_entropy.hpp>
#include <tls/counter_deterministic_random_generator.hpp>
#include <tls/standard_timer.hpp>

#include <tls/socket_input_output.hpp>
#include <tls/configuration.hpp>
#include <tls/ssl.hpp>

class luna_connection {
public:
    luna_connection(const std::string & certificate_directory);
    ~luna_connection();
    void listen(uint16_t base_port);
private:
    void read_commands(); 
    void parse_command(char * data, int size);
    tls::certificate m_ca_certificate;
    tls::certificate m_own_certificate;
    tls::private_key m_own_private_key;

    tls::standard_entropy m_entropy;
    tls::counter_deterministic_random_generator m_random;
    tls::standard_timer m_timer;

    tls::socket_input_output m_command_socket;
    tls::configuration m_command_configuration;
    tls::ssl m_command_ssl;

    /*socket_input_output m_data_socket;
    configuration m_data_configuration;*/
        

};
