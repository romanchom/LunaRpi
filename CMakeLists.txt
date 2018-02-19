cmake_minimum_required(VERSION 3.7)

set (CMAKE_CXX_STANDARD 14)

project(LunaRpi)

add_subdirectory(DTLS-server)

add_executable(${PROJECT_NAME}
    main.cpp
    discovery_responder.cpp
    luna_connection.cpp
)

target_link_libraries(${PROJECT_NAME}
    tls
)
