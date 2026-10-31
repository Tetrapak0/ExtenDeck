#include "NexusSockets.h"
#include "stdint.h"

unsigned long long recvn(socket_t socket, void* out, unsigned long long len, int flags) {
    uint64_t received = 0;
    while (received < len) {
        uint64_t res = recv(socket, (char*)out+received, len-received, flags);
        if (res < 1) {
            return res;
        }
        received += res;
    }
    return received;
}
