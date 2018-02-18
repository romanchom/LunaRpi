#include <cstdio>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


int main(int, char **) {
    uint16_t mPort = 9510;
    int mDiscoverySocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in6 bindAddress;
    bindAddress.sin6_family = AF_INET6;
    bindAddress.sin6_port = htons(mPort);
    bindAddress.sin6_addr  = IN6ADDR_ANY_INIT;

    auto error = bind(mDiscoverySocket, reinterpret_cast<sockaddr *>(&bindAddress), sizeof(sockaddr_in6));
    if (0 != error) {
        printf("%s%d", "Bind failed ", error);
    }

    while (true) {
        sockaddr_in6 fromAddress;
        auto addressSize = static_cast<socklen_t>(sizeof(sockaddr_in6));
        char buffer[1024];
        
        auto messageLength = recvfrom(mDiscoverySocket, buffer, sizeof(buffer), 0,
            reinterpret_cast<sockaddr *>(&fromAddress), &addressSize);
        if (messageLength < 0) {
            printf("%s%d", "Socket recvfrom error", messageLength);
            break;
        }

        printf("%s%s", "Got message: ", buffer);
        fflush(stdout);

        auto sentLength = sendto(mDiscoverySocket, buffer, messageLength, 0,
            reinterpret_cast<sockaddr *>(&fromAddress), addressSize);
        if (sentLength != messageLength) {
            printf("%s%d", "Socket sentto error", sentLength);
            break;
        }
    }
    close(mDiscoverySocket);
    return 0;
}