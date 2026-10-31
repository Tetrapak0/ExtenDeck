#include "../../include/Event/Event.hpp"
#include "Event/Event.hpp"
#include "Utilities/fnv1a_64.hpp"

#include <print>
#include <ranges>

namespace ExtenDeck::Event {
void HandlerList::addHandler(const Handler& handler) {
    std::unique_lock lock(mutex);
    auto current = std::make_shared<std::vector<Handler>>(*handlers.load());
    current->emplace_back(handler);
    handlers.store(current);
}
void HandlerList::removeHandler(uint64_t typeID)  {
    std::unique_lock lock(mutex);
    auto current = std::make_shared<std::vector<Handler>>(*handlers.load());
    std::erase_if(*current, [typeID](const Handler& handler) {
        return typeID == handler.typeID;
    });
    handlers.store(current);
}
void HandlerList::dispatch(ExtenDeckSDK::EventBase* event) {
    for (auto& handler : *handlers.load())
        handler.callback(event, handler.ctx);
}
bool HandlerList::hasHandlers() {
    return !handlers.load()->empty();
}


void ThreadContext::enqueueEvent(ExtenDeckSDK::EventBase* event) {
    std::unique_lock lock(mutex);
    if (parentBus.registry.isEventValid(event)) {
        parentBus.registry.retainEvent(event);
        queue.push_back(event);
    }
}
void ThreadContext::dispatchEvents() {
    std::deque<ExtenDeckSDK::EventBase*> _queue;
    {
        std::unique_lock lock(mutex);
        if (queue.empty())
            return;
        _queue.swap(queue);
    }
    for (auto& event : _queue) {
        EventDescriptor* descriptor = reinterpret_cast<EventDescriptor*>(event->descriptor);
        if (handlerLists.contains(descriptor->typename_fnv1a64))
            handlerLists[descriptor->typename_fnv1a64]->dispatch(event);
        else
            std::println("ThreadContext::DispatchEvents: no HandlerList found for event type {}!", descriptor->type_name);
        parentBus.registry.releaseEvent(event);
    }
}


void EventControlBlock::retain() {
    std::unique_lock _lock(lock);
    ++refcount;
}
void EventControlBlock::release() {
    std::unique_lock _lock(lock);
    --refcount;
}

string EventRegistry::createPrivateEventName(string type_name, ExtenDeckSDK::PluginBase* plugin) {
    return type_name + string(plugin->descriptor->base.display_name) + string(plugin->descriptor->base.type_name);
}
EventDescriptor* EventRegistry::registerEventType(string type_name, uint64_t size, ExtenDeckSDK::CallbackExecutionPolicy execution_policy, void(*constructor)(ExtenDeckSDK::EventBase* event), void(*destructor)(ExtenDeckSDK::EventBase* event)) {
    std::unique_lock _lock(lock);
    if (!constructor)
        return nullptr;
    if (m_eventDescriptors.contains(type_name) && !m_eventDescriptors.at(type_name).isPrivate) {
        m_eventDescriptors.at(type_name).registrations++;
        return &m_eventDescriptors.at(type_name);
    }
    m_eventDescriptors.emplace(type_name, type_name);
    EventDescriptor* descriptor = &m_eventDescriptors.at(type_name);
    descriptor->size = size;
    descriptor->create = constructor;
    descriptor->destroy = destructor;
    descriptor->allowed_dispatchers = execution_policy;
    //std::println("{}: {}", type_name, (intptr_t)descriptor);
    return descriptor;
}
EventDescriptor* EventRegistry::registerPrivateEventType(string type_name, uint64_t size, ExtenDeckSDK::CallbackExecutionPolicy execution_policy, ExtenDeckSDK::PluginBase* registrator, void(*constructor)(ExtenDeckSDK::EventBase* event), void(*destructor)(ExtenDeckSDK::EventBase* event)) {
    std::unique_lock _lock(lock);
    if (!constructor)
        return nullptr;
    if (registrator)
        type_name = createPrivateEventName(type_name, registrator);
    if (m_eventDescriptors.contains(type_name)) {
        m_eventDescriptors.at(type_name).registrations++;
        return &m_eventDescriptors.at(type_name);
    }
    m_eventDescriptors.emplace(type_name, type_name);
    EventDescriptor* descriptor = &m_eventDescriptors.at(type_name);
    descriptor->size = size;
    descriptor->create = constructor;
    descriptor->destroy = destructor;
    descriptor->allowed_dispatchers = execution_policy;
    descriptor->registrator = registrator;
    descriptor->isPrivate = true;
    return descriptor;
}

EventDescriptor* EventRegistry::findDescriptorByName(string type_name) {
    std::unique_lock _lock(lock);
    if (m_eventDescriptors.contains(type_name) && !m_eventDescriptors.at(type_name).isPrivate)
        return &m_eventDescriptors.at(type_name);
    std::println("No event descriptor found of type \"{}\"", type_name);
    return nullptr;
}
EventDescriptor* EventRegistry::findPrivateDescriptorByName(string type_name, ExtenDeckSDK::PluginBase* plugin) {
    std::unique_lock _lock(lock);
    if (!plugin)
        return nullptr;
    type_name = createPrivateEventName(type_name, plugin);
    if (m_eventDescriptors.contains(type_name))
        return m_eventDescriptors.at(type_name).registrator == plugin ? &m_eventDescriptors.at(type_name) : nullptr;
    std::println("No private event descriptor found of type \"{}\"", type_name);
    return nullptr;
}
void EventRegistry::unregisterEventType(EventDescriptor* descriptor, ExtenDeckSDK::PluginBase* plugin) {
    std::unique_lock _lock(lock);
    if (!descriptor || !plugin)
        return;
    if (!m_eventDescriptors.contains(descriptor->type_name))
        return;
    if (descriptor->registrator != plugin)
        return;
    --m_eventDescriptors.at(descriptor->type_name).registrations;
    if (!m_eventDescriptors.at(descriptor->type_name).registrations)
        m_eventDescriptors.erase(descriptor->type_name);
}
bool EventRegistry::isEventValid(ExtenDeckSDK::EventBase* event) {
    return m_eventControlBlocks.contains(event);
}
ExtenDeckSDK::EventBase* EventRegistry::createEvent(EventDescriptor* descriptor) {
    std::unique_lock _lock(lock);
    if (!descriptor)
        return nullptr;
    ExtenDeckSDK::EventBase* base = new (operator new(descriptor->size))
        ExtenDeckSDK::EventBase{(ExtenDeckSDK::EventDescriptor*)descriptor};
    if (descriptor->create)
        descriptor->create(base);
    base->descriptor = (ExtenDeckSDK::EventDescriptor*)descriptor;
    return m_eventControlBlocks.try_emplace(base, descriptor, 1).first->first;
}
ExtenDeckSDK::EventBase* EventRegistry::createEvent(string Typename) {
    return createEvent(findDescriptorByName(Typename));
}
ExtenDeckSDK::EventBase* EventRegistry::createEvent(string Typename, ExtenDeckSDK::PluginBase* plugin) {
    return createEvent(findPrivateDescriptorByName(Typename, plugin));
}

void EventRegistry::retainEvent(ExtenDeckSDK::EventBase* event) {
    std::unique_lock _lock(lock);
    if (isEventValid(event))
        m_eventControlBlocks[event].retain();
}
void EventRegistry::releaseEvent(ExtenDeckSDK::EventBase* event) {
    std::unique_lock _lock(lock);
    if (!isEventValid(event))
        return;
    EventControlBlock& ctrlBlock = m_eventControlBlocks[event];
    ctrlBlock.release();
    if (!ctrlBlock.refcount) {
        if (ctrlBlock.descriptor->destroy)
            ctrlBlock.descriptor->destroy(event);
        operator delete(event);
        m_eventControlBlocks.erase(event);
    }
}


std::unique_ptr<EventBus::SubscriptionToken> EventBus::subscribeToEvent(EventDescriptor* descriptor, ExtenDeckSDK::EventCallback callback, void* ctx, ExtenDeckSDK::CallbackExecutionPolicy executionPolicy) {
    if (!descriptor || !callback)
        return nullptr;
    if (descriptor->allowed_dispatchers && !(descriptor->allowed_dispatchers & executionPolicy))
        return nullptr;

    std::thread::id handlerThreadID{};
    std::shared_ptr<ThreadContext> threadContext{};

    std::unique_lock lock(m_mutex);

    switch (executionPolicy) {
        case ExtenDeckSDK::DxECEPImmediate: {
            threadContext = this->m_immediateContext;
            break;
        } case ExtenDeckSDK::DxECEPWorkerThread: {
            handlerThreadID = this->m_workerThread.get_id();
            threadContext = this->m_workerContext;
            break;
        } case ExtenDeckSDK::DxECEPSubscriberThread: {
            handlerThreadID = std::this_thread::get_id();
            if (!m_threadContexts.contains(handlerThreadID))
                return nullptr;
            threadContext = m_threadContexts[handlerThreadID];
            break;
        } default: {
            return nullptr;
        }
    }
    {
        std::unique_lock lock(threadContext->mutex);
        std::unique_ptr<HandlerList>& list = threadContext->handlerLists[descriptor->typename_fnv1a64];
        if (!list)
            list = std::make_unique<HandlerList>(descriptor->typename_fnv1a64);
        list->addHandler(Handler(handlerThreadID, executionPolicy, ++m_nextID, callback, ctx));
    }

    std::vector<std::shared_ptr<ThreadContext>>& interestedThreads = m_threadsInterestedInEvent[descriptor->typename_fnv1a64];
    if (std::find(interestedThreads.begin(), interestedThreads.end(), threadContext) == interestedThreads.end())
        interestedThreads.push_back(threadContext);

    return std::make_unique<SubscriptionToken>(
        descriptor->typename_fnv1a64,
        m_nextID,
        executionPolicy,
        handlerThreadID,
        *this
    );
}

void EventBus::unsubscribeFromEvent(const SubscriptionToken& token) {
    if (&token.parentBus != this)
        return;
    std::unique_lock lock(m_mutex);
    std::shared_ptr<ThreadContext> ctx = nullptr;
    switch (token.executionPolicy) {
        case ExtenDeckSDK::DxECEPImmediate: {
            ctx = m_immediateContext;
            break;
        } case ExtenDeckSDK::DxECEPWorkerThread: {
            ctx = m_workerContext;
            break;
        } case ExtenDeckSDK::DxECEPSubscriberThread: {
            if (!m_threadContexts.contains(token.threadID))
                return;
            ctx = m_threadContexts[token.threadID];
            break;
        } default: {
            return;
        }
    }
    std::unique_lock ctx_lock(ctx->mutex);
    if (!ctx->handlerLists.contains(token.typeID))
        return;

    ctx->handlerLists[token.typeID]->removeHandler(token.id);

    if (!ctx->handlerLists[token.typeID]->hasHandlers()) {
        ctx->handlerLists.erase(token.typeID);
        if (m_threadsInterestedInEvent.contains(token.typeID)) {
            std::erase_if(m_threadsInterestedInEvent[token.typeID], [ctx](const std::shared_ptr<ThreadContext>& threadContext) {
                return threadContext == ctx;
            });
        }
    }
}

int EventBus::postEvent(ExtenDeckSDK::EventBase* event) {
    if (!registry.isEventValid(event))
        return -1;
    if (!event->descriptor)
        return -1;
    EventDescriptor* descriptor = reinterpret_cast<EventDescriptor*>(event->descriptor);
    if (!m_threadsInterestedInEvent.contains(Utilities::FNV1a_64::hash64(descriptor->type_name)))
        return 0;
    std::println("Event of type {} posted.", descriptor->type_name);
    std::unique_lock global_lock(m_mutex);
    std::vector<std::shared_ptr<ThreadContext>>& threadsInterestedInEvent = m_threadsInterestedInEvent[Utilities::FNV1a_64::hash64(descriptor->type_name)];
    for (auto& threadContext : threadsInterestedInEvent) {
        threadContext->enqueueEvent(event);
        if (threadContext == m_workerContext)
            m_workerCV.notify_all();
        if (threadContext == m_immediateContext)
            threadContext->dispatchEvents();
    }
    return 0;
}

std::shared_ptr<ThreadContext> EventBus::registerThreadAsEventEndpoint() {
    std::thread::id threadID = std::this_thread::get_id();
    std::lock_guard lock(m_mutex);
    if (m_threadContexts.contains(threadID))
        return m_threadContexts[threadID];
    return m_threadContexts[threadID] = std::make_shared<ThreadContext>(*this);
}
void EventBus::unregisterThreadAsEventEndpoint() {
    std::thread::id threadID = std::this_thread::get_id();
    std::lock_guard lock(m_mutex);
    if (!m_threadContexts.contains(threadID))
        return;
    std::shared_ptr<ThreadContext> ctx = m_threadContexts[threadID];
    for (auto& type : ctx->handlerLists | std::views::keys) {
        if (!m_threadsInterestedInEvent.contains(type))
            continue;
        auto& threadList = m_threadsInterestedInEvent[type];

        std::erase_if(threadList, [ctx](const std::shared_ptr<ThreadContext>& tmp){
            return ctx == tmp;
        });
    }
    m_threadContexts.erase(threadID);
}

void EventBus::handlerWorker() {
    while (true) {
        {
            std::unique_lock lock(m_workerContext->mutex);

            m_workerCV.wait(lock, [&] {
                return !m_running || !m_workerContext->queue.empty();
            });

            if (!m_running && m_workerContext->queue.empty())
                return;
        }
        m_workerContext->dispatchEvents();
    }
}
}
