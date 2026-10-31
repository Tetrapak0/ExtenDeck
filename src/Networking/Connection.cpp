#include "Networking/Connection.hpp"
#include "Networking/Networking.hpp"
#include "Networking/Message.hpp"

#include "Utilities/fnv1a_64.hpp"

#include <string.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace ExtenDeck::Networking {
int Connection::send(const char* data, uint64_t len) {
    return ::send(socket, data, len, 0);
}

int Connection::send(const ExtenDeckSDK::Message& msg, const ExtenDeckSDK::PluginBase* plugin) {
    if (!plugin)
        return NX_SOCKET_ERROR;
    MessageHeader header{htonll(msg.msg.length()),
                         htonll(Utilities::FNV1a_64::hash64(plugin->descriptor->base.type_name)),
                         htonll(Utilities::FNV1a_64::hash64(msg.type_name))};
    vector<uint8_t> message(sizeof(MessageHeader)+msg.msg.length(), 0);

    memcpy(message.data(), &header, sizeof(MessageHeader));
    memcpy(message.data()+sizeof(MessageHeader), msg.msg.data(), msg.msg.length());

    return send((const char*)message.data(), message.size());
}

bool Connection::isConnected() const {
    return socket != NX_INVALID_SOCKET;
}
}
