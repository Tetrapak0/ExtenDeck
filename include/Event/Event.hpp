#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Client/DiscoveryListenerService.hpp"
#include "GUI/Texture.hpp"
#include "Plugin/Plugin.hpp"
#include "SDK/SDK.h"
#include "Utilities/fnv1a_64.hpp"

namespace ExtenDeck::Event {
class EventBus;

struct Handler {
    const std::thread::id threadID;
    const ExtenDeckSDK::CallbackExecutionPolicy executionPolicy;
    const uint64_t typeID;
    const ExtenDeckSDK::EventCallback callback;
    void* ctx;
    Handler() = delete;
    Handler(std::thread::id threadID, ExtenDeckSDK::CallbackExecutionPolicy executionPolicy,
            uint64_t typeID, ExtenDeckSDK::EventCallback callback, void* ctx)
    : threadID(threadID), executionPolicy(executionPolicy), typeID(typeID), callback(callback), ctx(ctx) {}
    Handler operator=(const Handler& other) const {
        return Handler{other.threadID, other.executionPolicy, other.typeID, other.callback, other.ctx};
    }
};

class HandlerList {
    std::atomic<std::shared_ptr<std::vector<Handler>>> handlers = std::make_shared<std::vector<Handler>>();
    std::mutex mutex;
public:
    const uint64_t typeID;
    
    void addHandler(const Handler& handler);
    void removeHandler(uint64_t id);
    void dispatch(ExtenDeckSDK::EventBase* base);
    bool hasHandlers();

    HandlerList(uint64_t typeID) : typeID(typeID) {}
};

struct ThreadContext {
    std::mutex mutex;
    EventBus& parentBus;
    std::deque<ExtenDeckSDK::EventBase*> queue;
    std::unordered_map<uint64_t, std::unique_ptr<HandlerList>> handlerLists;
    void enqueueEvent(ExtenDeckSDK::EventBase* event);
    void dispatchEvents();
    ThreadContext(EventBus& parentBus) : parentBus(parentBus) {}
};

struct EventDescriptor {
    const string type_name;
    const uint64_t typename_fnv1a64;
    uint64_t size = 0;
    void(*create)(ExtenDeckSDK::EventBase* event) = nullptr;
    void(*destroy)(ExtenDeckSDK::EventBase* event) = nullptr;
    ExtenDeckSDK::CallbackExecutionPolicy allowed_dispatchers;
    ExtenDeckSDK::PluginBase* registrator = nullptr;
    bool isPrivate = false;
    uint64_t registrations = 1;
    EventDescriptor(string type_name) : type_name(type_name), typename_fnv1a64(Utilities::FNV1a_64::hash64(type_name)) {} // TODO:
};

struct EventControlBlock {
    struct EventDescriptor* descriptor;
    uint64_t refcount = 1;
    std::mutex lock;
    void retain();
    void release();
};

class EventRegistry {
    std::unordered_map<string, EventDescriptor> m_eventDescriptors;
    std::unordered_map<ExtenDeckSDK::EventBase*, EventControlBlock> m_eventControlBlocks;
    std::mutex lock;
public:
    static string createPrivateEventName(string type_name, ExtenDeckSDK::PluginBase* plugin);
    EventDescriptor* registerEventType(string type_name, uint64_t size, ExtenDeckSDK::CallbackExecutionPolicy execution_policy, void(*constructor)(ExtenDeckSDK::EventBase* event), void(*destructor)(ExtenDeckSDK::EventBase* event) = nullptr);
    EventDescriptor* registerPrivateEventType(string type_name, uint64_t size, ExtenDeckSDK::CallbackExecutionPolicy execution_policy, ExtenDeckSDK::PluginBase* registrator, void(*constructor)(ExtenDeckSDK::EventBase* event), void(*destructor)(ExtenDeckSDK::EventBase* event) = nullptr);
    EventDescriptor* findDescriptorByName(string type_name);
    EventDescriptor* findPrivateDescriptorByName(string type_name, ExtenDeckSDK::PluginBase* plugin);
    void unregisterEventType(EventDescriptor* descriptor, ExtenDeckSDK::PluginBase* plugin);
    bool isEventValid(ExtenDeckSDK::EventBase* event);
    ExtenDeckSDK::EventBase* createEvent(EventDescriptor* descriptor);
    ExtenDeckSDK::EventBase* createEvent(string Typename);
    ExtenDeckSDK::EventBase* createEvent(string Typename, ExtenDeckSDK::PluginBase* plugin);
    void retainEvent(ExtenDeckSDK::EventBase* event);
    void releaseEvent(ExtenDeckSDK::EventBase* event);
};

// TODO: EventBus registry because because

class EventBus final {
public:
    struct SubscriptionToken {
        const uint64_t typeID;
        const uint64_t id;
        const ExtenDeckSDK::CallbackExecutionPolicy executionPolicy;
        const std::thread::id threadID;
        EventBus& parentBus;
        SubscriptionToken(uint64_t type, uint64_t id, ExtenDeckSDK::CallbackExecutionPolicy executionPolicy, std::thread::id threadID, EventBus& bus) : typeID(type), id(id), executionPolicy(executionPolicy), threadID(threadID), parentBus(bus) {}
        ~SubscriptionToken() {
            parentBus.unsubscribeFromEvent(*this);
        }
    };

    EventRegistry& registry;

    std::unique_ptr<SubscriptionToken> subscribeToEvent(EventDescriptor* descriptor, ExtenDeckSDK::EventCallback callback, void* ctx, ExtenDeckSDK::CallbackExecutionPolicy executionPolicy);
    void unsubscribeFromEvent(const SubscriptionToken& token);

    int postEvent(ExtenDeckSDK::EventBase* event);

    std::shared_ptr<ThreadContext> registerThreadAsEventEndpoint();
    void unregisterThreadAsEventEndpoint();
private:
    std::shared_ptr<ThreadContext> m_workerContext = std::make_shared<ThreadContext>(*this);
    std::shared_ptr<ThreadContext> m_immediateContext = std::make_shared<ThreadContext>(*this);
    std::unordered_map<std::thread::id, std::shared_ptr<ThreadContext>> m_threadContexts;
    std::unordered_map<uint64_t, std::vector<std::shared_ptr<ThreadContext>>> m_threadsInterestedInEvent;
    uint64_t m_nextID = 0;

    bool m_running = true;
    std::condition_variable m_workerCV;
    std::mutex m_mutex;

    void handlerWorker();
    std::thread m_workerThread;
public:
    EventBus(EventRegistry& registry) : registry(registry) {
        m_workerThread = std::thread{[this] {
            this->handlerWorker();
        }};
    }
    ~EventBus() {
        m_running = false;
        m_workerCV.notify_one();
        m_workerThread.join();
    }
};

class ThreadLocalEventBus {
public:
    EventBus& eventBus;
private:
    std::shared_ptr<ThreadContext> threadContext;
public:
    int postEvent(ExtenDeckSDK::EventBase* event) {
        if (!eventBus.registry.isEventValid(event))
            return -1;
        std::lock_guard lock(threadContext->mutex);
        eventBus.registry.retainEvent(event);
        threadContext->queue.push_back(event);
        return 0;
    }
    void processEvents() const {
        threadContext->dispatchEvents();
    }
    ThreadLocalEventBus(EventBus& eventBus) : eventBus(eventBus), threadContext(eventBus.registerThreadAsEventEndpoint()) {}
    ~ThreadLocalEventBus() {
        eventBus.unregisterThreadAsEventEndpoint();
    }
};

struct EClientConnected final : ExtenDeckSDK::EventBase {
    uint64_t clientID;
};

struct EClientDisconnected final : ExtenDeckSDK::EventBase {
    uint64_t clientID;
};

struct EClientDisconnectIssued final : ExtenDeckSDK::EventBase {
    uint64_t clientID;
};

struct EServerConnected final : ExtenDeckSDK::EventBase {
    Networking::ServerInfo serverInfo;
};

struct EServerDisconnected final : ExtenDeckSDK::EventBase {
    Networking::ServerInfo serverInfo;
};

struct ETextureReadyToBind final : ExtenDeckSDK::EventBase {
    GUI::Texture* texture;
};

struct EServerFound final : ExtenDeckSDK::EventBase {
    Networking::ServerInfo serverInfo;
};

}
