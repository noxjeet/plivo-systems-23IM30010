#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <array>
#include <vector>
#include <cstdint>

struct CircularBuffer {
    std::array<std::array<uint8_t, 160>, 2048> frames;
    std::array<bool, 2048> present;
};

int main(void) {
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    int opt = 1;
    setsockopt(in_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47010);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(in_fd, (struct sockaddr *)&in_addr, sizeof in_addr) < 0) {
        perror("bind 47010");
        return 1;
    }

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in relay = {0};
    relay.sin_family = AF_INET;
    relay.sin_port = htons(47001);
    relay.sin_addr.s_addr = inet_addr("127.0.0.1");

    unsigned char buf[2048];
    CircularBuffer buffer = {0};

    for (;;) {
        ssize_t n = recvfrom(in_fd, buf, sizeof buf, 0, NULL, NULL);
        if (n <= 0) continue;
        if (n < 164) continue;

        uint32_t seq;
        memcpy(&seq, buf, 4);
        seq = ntohl(seq);

        size_t index = seq % 2048;
        memcpy(buffer.frames[index].data(), buf + 4, 160);
        buffer.present[index] = true;

        unsigned char out_buf[1024];
        uint32_t net_seq = htonl(seq);
        memcpy(out_buf, &net_seq, 4);

        uint8_t has_prev = 0;
        if (seq > 0 && (seq % 20 != 0)) {
            size_t prev_index = (seq - 1) % 2048;
            if (buffer.present[prev_index]) {
                has_prev = 1;
            }
        }
        out_buf[4] = has_prev;

        memcpy(out_buf + 5, buffer.frames[index].data(), 160);
        size_t out_len = 165;

        if (has_prev) {
            size_t prev_index = (seq - 1) % 2048;
            memcpy(out_buf + 165, buffer.frames[prev_index].data(), 160);
            out_len = 325;
        }

        sendto(out_fd, out_buf, out_len, 0, (struct sockaddr *)&relay, sizeof relay);
    }
    return 0;
}
