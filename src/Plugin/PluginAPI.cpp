#include "Client/Client.hpp"
#include "Config/Deckastore.hpp"
#include "Plugin/PluginAPI.h"

namespace ExtenDeckSDK::impl {
void freeString(const char* buf) {
    delete[] buf;
}

Client* current() {
    return reinterpret_cast<Client*>(ExtenDeck::Deckastore::get().retrieve_current_client());
}
Client* byUUID(uint64_t uuid) {
    return reinterpret_cast<Client*>(ExtenDeck::Deckastore::get().clientManager.retrieve_client(uuid));
}
uint64_t getUUID(Client* client) {
    if (!client)
        return DX_INVALID_UUID;
    return reinterpret_cast<ExtenDeck::Client::Client*>(client)->get_uuid();
}
const char* getNickname(Client* client) {
    if (!client)
        return nullptr;
    return reinterpret_cast<ExtenDeck::Client::Client*>(client)->get_nickname().c_str();
}
bool isConnected(Client* client) {
    if (!client)
        return false;
    return reinterpret_cast<ExtenDeck::Client::Client*>(client)->isConnected();
}
int send(Client* client, Message msg, PluginBase* sender) {
    if (!client || !sender)
        return NX_SOCKET_ERROR;
    return reinterpret_cast<ExtenDeck::Client::Client*>(client)->connection.send(msg, sender);
}


Profile* currentProfile(Client* client) {
    if (!client)
        return nullptr;
    return reinterpret_cast<Profile*>(&reinterpret_cast<ExtenDeck::Client::Client*>(client)->getCurrentProfile());
}
const char* configDir(Profile* profile) {
    if (!profile)
        return nullptr;
    std::string path = reinterpret_cast<ExtenDeck::Profile*>(profile)->get_profile_dir().string();
    char* buf = new char[path.length()+1]{};
    memcpy(buf, path.c_str(), path.length());
    buf[path.length()+1] = '\0';
    return buf;
}
Client* getParentClient(Profile* profile) {
    if (!profile)
        return nullptr;
    return reinterpret_cast<Client*>(&reinterpret_cast<ExtenDeck::Profile*>(profile)->parent);
}

SubscriptionToken* subscribeToEvent(EventBus* bus, EventDescriptor* descriptor, EventCallback callback,
                                      void* ctx, uint64_t callbackExecutionPolicy) {
    if (!bus || !descriptor || !callback)
        return nullptr;
    return reinterpret_cast<SubscriptionToken*>(reinterpret_cast<ExtenDeck::Event::EventBus*>(bus)
            ->subscribeToEvent(reinterpret_cast<ExtenDeck::Event::EventDescriptor*>(descriptor), callback, ctx,
                                (CallbackExecutionPolicy)callbackExecutionPolicy).release());
}
void postEvent(EventBus* bus, EventBase* event) {
    if (!bus || !event)
        return;
    reinterpret_cast<ExtenDeck::Event::EventBus*>(bus)->postEvent(event);
}
void unsubscribeFromEvent(EventBus* bus, SubscriptionToken* token) {
    if (!bus || !token)
        return;
    ((ExtenDeck::Event::EventBus*)bus)->unsubscribeFromEvent(*(ExtenDeck::Event::EventBus::SubscriptionToken*)token);
    delete (ExtenDeck::Event::EventBus::SubscriptionToken*)token;
}
EventDescriptor* registerEventType(EventBus* bus, const char* type_name, uint64_t typename_len,
                                        uint64_t size, uint64_t execution_policy,
                                        void(*constructor)(EventBase*), void(*destructor)(EventBase*)) {
    if (!bus || !type_name || !typename_len || !size)
        return nullptr;
    return reinterpret_cast<EventDescriptor*>(reinterpret_cast<ExtenDeck::Event::EventBus*>(bus)
            ->registry.registerEventType(string(type_name, typename_len), size, 
                                        (CallbackExecutionPolicy)execution_policy,
                                        constructor, destructor));
}
EventDescriptor* registerPrivateEventType(EventBus* bus, const char* type_name, uint64_t typename_len,
                                                uint64_t size, uint64_t execution_policy, PluginBase* plugin, void(*constructor)(EventBase*),
                                                void(*destructor)(EventBase*)) {
    if (!bus || !type_name || !typename_len || !size || !plugin)
        return nullptr;
    return reinterpret_cast<EventDescriptor*>(reinterpret_cast<ExtenDeck::Event::EventBus*>(bus)
            ->registry.registerPrivateEventType(string(type_name, typename_len), size, 
                                                (CallbackExecutionPolicy)execution_policy, plugin,
                                                constructor, destructor));
}
void unregisterEventType(EventBus* bus, EventDescriptor* descriptor, PluginBase* plugin) {
    if (!bus || !descriptor || !plugin)
        return;
    reinterpret_cast<ExtenDeck::Event::EventBus*>(bus)->registry.unregisterEventType(reinterpret_cast<ExtenDeck::Event::EventDescriptor*>(descriptor), plugin);
}
EventDescriptor* findEventDescriptorByTypename(EventBus* bus, const char* type_name, uint64_t typename_len) {
    if (!bus || !type_name || !typename_len)
        return nullptr;
    return reinterpret_cast<EventDescriptor*>(reinterpret_cast<ExtenDeck::Event::EventBus*>(bus)
            ->registry.findDescriptorByName(string(type_name, typename_len)));
}
struct EventDescriptor* findPrivateEventDescriptorByTypename(EventBus* bus, const char* type_name,
                                                            uint64_t typename_len, PluginBase* plugin) {
    if (!bus || !type_name || !typename_len || !plugin)
        return nullptr;
    return reinterpret_cast<EventDescriptor*>(reinterpret_cast<ExtenDeck::Event::EventBus*>(bus)
            ->registry.findPrivateDescriptorByName(ExtenDeck::Event::EventRegistry::createPrivateEventName(string(type_name, typename_len),
                                                                                                plugin), plugin));
}
EventBase* createEvent(EventBus* bus, EventDescriptor* descriptor) {
    if (!bus || !descriptor)
        return nullptr;
    return reinterpret_cast<ExtenDeck::Event::EventBus*>(bus)->registry.createEvent(reinterpret_cast<ExtenDeck::Event::EventDescriptor*>(descriptor));
}
void retainEvent(EventBus* bus, EventBase* event) {
    if (!bus || !event)
        return;
    reinterpret_cast<ExtenDeck::Event::EventBus*>(bus)->registry.retainEvent(event);
}
void releaseEvent(EventBus* bus, EventBase* event) {
    if (!bus || !event)
        return;
    reinterpret_cast<ExtenDeck::Event::EventBus*>(bus)->registry.releaseEvent(event);
}


Texture* createTextureFromFile(const char* path, uint64_t path_len) {
    if (!path || !path_len)
        return nullptr;
    Texture* tmp = reinterpret_cast<Texture*>(new ExtenDeck::GUI::Texture(path));
    return tmp;
}
Texture* createTextureFromMemory(const uint8_t* data, uint64_t len) {
    if (!data || !len)
        return nullptr;
    return reinterpret_cast<Texture*>(new ExtenDeck::GUI::Texture(data, len));
}
int writeTextureAsPNG(Texture* texture, const char* outputPath, uint64_t outputPathLen) {
    if (!texture || !outputPath || !outputPathLen)
        return -1;
    return reinterpret_cast<ExtenDeck::GUI::Texture*>(texture)->write_png(outputPath);
}
int getWidth(Texture* texture) {
    if (!texture)
        return -1;
    return reinterpret_cast<ExtenDeck::GUI::Texture*>(texture)->get_width();
}
int getHeight(Texture* texture) {
    if (!texture)
        return -1;
    return reinterpret_cast<ExtenDeck::GUI::Texture*>(texture)->get_height();
}
void destroyTexture(Texture* texture) {
    if (texture)
        delete (ExtenDeck::GUI::Texture*)texture;
}
void overlayTexture(Texture* texture, int w, int h) {
    ExtenDeck::GUI::overlay_texture(*reinterpret_cast<ExtenDeck::GUI::Texture*>(texture), ImVec2(w,h));
}
unsigned int getTextureID(struct Texture* texture) {
    return ((ExtenDeck::GUI::Texture*)texture)->getTextureID();
}
bool isTextureInit(struct Texture* texture) {
    return ((ExtenDeck::GUI::Texture*)texture)->is_init();
}
bool isTextureReadyToBind(struct Texture* texture) {
    return ((ExtenDeck::GUI::Texture*)texture)->ready_to_bind();
}
bool isTextureNull(struct Texture* texture) {
    return ((ExtenDeck::GUI::Texture*)texture)->is_null();
}
}
