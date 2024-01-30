#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <iostream>

void sendWithLength(int sock, char *buf, size_t len) {
    // Send a message prepended by a length
    char temp[256]{};
    std::memcpy(temp + 1, buf, len);
    temp[0] = static_cast<uint8_t>(len);

    int bytesSent = write(sock, temp, len + 1);
    if (bytesSent == -1) {
        if (errno == EWOULDBLOCK) {
            bytesSent = 0;
        } else {
            throw std::runtime_error("uihadsygyads");
        }
    }
    std::cout<<bytesSent<<std::endl;
    while (bytesSent < len + 1) {
        int newBytes = write(sock, temp + bytesSent, len + 1 - bytesSent);
        if (newBytes == -1) {
            if (errno == EWOULDBLOCK)
                continue;
            throw std::runtime_error("uihadsygyads");
        }
        bytesSent += newBytes;
        std::cout << bytesSent << std::endl;
    }
}

int receive(int sock, char *buf) {
    // Send a message prepended by a length

        int bytesRead = read(sock, buf, 1);
        if (bytesRead == -1) {
            if (errno == EWOULDBLOCK) {
                return 0;
            } else
                return -1;
        }


    size_t msgSize = static_cast<size_t>(buf[0]);
    std::cout << " i am receiving and the expected size is: " << msgSize << std::endl;
    while (bytesRead < msgSize + 1) {
        int newBytes = read(sock, buf + bytesRead, msgSize + 1 - bytesRead);
        if (newBytes == -1) {
            if (errno == EWOULDBLOCK)
                continue;
            return -1;
        }
        bytesRead += newBytes;
    }

    std::memmove(buf, buf+1,msgSize);
    return bytesRead;
}