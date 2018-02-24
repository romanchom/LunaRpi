#include <cstdio>

#include "discovery_responder.hpp"
#include "luna_connection.hpp"

int main(int, char **) {
    discovery_responder responder(9510, "Luna Rpi v2.0");
    luna_connection connection("./certs");
    connection.listen(9510);

    char buff[1024];
    scanf("%s", buff);
    return 0;
}