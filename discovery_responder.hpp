#pragma once

#include <thread>
#include <atomic>

class discovery_responder {
public:
    discovery_responder(uint16_t listening_port, const std::string & response);
    ~discovery_responder();
private:
    void task(uint16_t listening_port, const std::string & response);

    std::atomic<bool> m_should_run;
    std::thread m_thread;
};
