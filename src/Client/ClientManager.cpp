#include "Client/Client.hpp"
#include "Config/Deckastore.hpp"

namespace ExtenDeck::Client {
Client* ClientManager::retrieve_client(uint64_t uuid) {
    if (!clients.contains(uuid))
        return nullptr;
    return &clients.at(uuid);
}

Client* ClientManager::insert_client(uint64_t uuid, socket_t socket) {
    std::unique_lock lock(mutex);
    if (!client_exists(uuid)) {
        auto [it, emplaced] = clients.try_emplace(uuid, uuid, socket);
        Deckastore& dxstore = Deckastore::get();
        Event::EventDescriptor* descriptor = dxstore.eventBus.registry.findPrivateDescriptorByName("org.PoS.ExtenDeck.events.ClientConnected", dxstore.applicationPlugin->plugin);
        Event::EClientConnected* event = reinterpret_cast<Event::EClientConnected*>(dxstore.eventBus.registry.createEvent(descriptor));
        event->clientID = uuid;
        dxstore.eventBus.postEvent(event);
        dxstore.eventBus.registry.releaseEvent(event);
        return &it->second;
    }
    return nullptr;
}

bool ClientManager::client_exists(uint64_t uuid) const {
    return clients.contains(uuid);
}

void ClientManager::disconnect_client(uint64_t uuid, ClientDisconnectReason reason) {
    std::unique_lock lock(mutex);
    Deckastore& dxstore = Deckastore::get();
    if (!clients.contains(uuid))
        return;
    if (reason == ClientDisconnectReason::ConnectionClosed) {
        Event::EClientDisconnected* event = reinterpret_cast<Event::EClientDisconnected*>(dxstore.eventBus.registry.createEvent(dxstore.eventBus.registry.findDescriptorByName("org.PoS.ExtenDeck.events.ClientDisconnected")));
        event->clientID = uuid;
        dxstore.eventBus.postEvent(event);
        dxstore.eventBus.registry.releaseEvent(event);
    } else {
        Event::EClientDisconnectIssued* event = reinterpret_cast<Event::EClientDisconnectIssued*>(dxstore.eventBus.registry.createEvent(dxstore.eventBus.registry.findDescriptorByName("org.PoS.ExtenDeck.events.ClientDisconnectIssued")));
        event->clientID = uuid;
        dxstore.eventBus.postEvent(event);
        dxstore.eventBus.registry.releaseEvent(event);
    }
    clients.erase(uuid);
}

void ClientManager::request_disconnect(uint64_t uuid) {
    std::unique_lock lock(mutex);
    if (clients.contains(uuid))
        clients.at(uuid).disconnectReason = ClientDisconnectReason::DisconnectIssued;
}

uint64_t ClientManager::getNumOfClients() const {
    return clients.size();
}

const unordered_map<uint64_t, Client>& ClientManager::getClientsMap() {
    return clients;
}
}
