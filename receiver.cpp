#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstdint>

int main(void) {
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    int opt = 1;
    setsockopt(in_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47002);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(in_fd, (struct sockaddr *)&in_addr, sizeof in_addr) < 0) {
        perror("bind 47002");
        return 1;
    }

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in player = {0};
    player.sin_family = AF_INET;
    player.sin_port = htons(47020);
    player.sin_addr.s_addr = inet_addr("127.0.0.1");

    unsigned char buf[2048];
    std::vector<bool> sent(1000000, false);

    for (;;) {
        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);
        ssize_t n = recvfrom(in_fd, buf, sizeof buf, 0, (struct sockaddr *)&src_addr, &addr_len);
        if (n <= 0) continue;
        if (n < 165) continue;

        uint32_t seq;
        memcpy(&seq, buf, 4);
        seq = ntohl(seq);

        uint8_t has_prev = buf[4];

        if (seq < sent.size()) {
            if (!sent[seq]) {
                unsigned char play_buf[164];
                uint32_t net_seq = htonl(seq);
                memcpy(play_buf, &net_seq, 4);
                memcpy(play_buf + 4, buf + 5, 160);
                sendto(out_fd, play_buf, 164, 0, (struct sockaddr *)&player, sizeof player);
                sent[seq] = true;
            }
        }

        if (has_prev == 1 && seq > 0 && n >= 325) {
            uint32_t prev_seq = seq - 1;
            if (prev_seq < sent.size()) {
                if (!sent[prev_seq]) {
                    unsigned char play_buf[164];
                    uint32_t net_seq = htonl(prev_seq);
                    memcpy(play_buf, &net_seq, 4);
                    memcpy(play_buf + 4, buf + 165, 160);
                    sendto(out_fd, play_buf, 164, 0, (struct sockaddr *)&player, sizeof player);
                    sent[prev_seq] = true;
                }
            }
        }
    }

    return 0;
}
