#pragma once

#include "SDK/SDK.h"

#undef _UNICODE

namespace ExtenDeckSDK::impl {
void freeString(const char*);

// clientAPI
struct Client* current();
Profile* currentProfile(Client* client);
Client* byUUID(uint64_t uuid);
uint64_t getUUID(Client* client);
const char* getNickname(Client* client);
bool isConnected(Client* client);
int send(Client* client, Message msg, PluginBase* sender);

//profileAPI
const char* configDir(Profile* profile);
Client* getParentClient(Profile* profile);

//eventAPI
SubscriptionToken* subscribeToEvent(EventBus* bus, EventDescriptor* type, EventCallback callback, void* ctx, uint64_t callbackExecutionPolicy);
void postEvent(EventBus* bus, EventBase* event);
void unsubscribeFromEvent(EventBus* bus, SubscriptionToken* token);
EventDescriptor* registerEventType(EventBus* bus, const char* type_name, uint64_t typename_len, uint64_t size, uint64_t execution_policy, void(*constructor)(EventBase*), void(*destructor)(EventBase*));
EventDescriptor* registerPrivateEventType(EventBus* bus, const char* type_name, uint64_t typename_len, uint64_t size, uint64_t execution_policy, PluginBase* plugin, void(*constructor)(EventBase*), void(*destructor)(EventBase*));
void unregisterEventType(EventBus* bus, EventDescriptor* descriptor, PluginBase* plugin);
EventDescriptor* findEventDescriptorByTypename(EventBus* bus, const char* type_name, uint64_t typename_len);
EventDescriptor* findPrivateEventDescriptorByTypename(EventBus* bus, const char* type_name, uint64_t typename_len, PluginBase* plugin);
EventBase* createEvent(EventBus* bus, EventDescriptor* descriptor);
void retainEvent(EventBus* bus, EventBase* event);
void releaseEvent(EventBus* bus, EventBase* event);

//graphicsAPI
Texture* createTextureFromFile(const char* path, uint64_t path_len);
Texture* createTextureFromMemory(const uint8_t* data, uint64_t len);
int writeTextureAsPNG(Texture* texture, const char* outputPath, uint64_t outputPathLen);
int getWidth(Texture* texture);
int getHeight(Texture* texture);
void destroyTexture(Texture* texture);
void overlayTexture(Texture* texture, int w, int h);
unsigned int getTextureID(struct Texture* texture);
bool isTextureInit(struct Texture* texture);
bool isTextureReadyToBind(struct Texture* texture);
bool isTextureNull(struct Texture* texture);
}
