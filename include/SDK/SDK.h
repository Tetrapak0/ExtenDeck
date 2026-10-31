#pragma once

#include <stdint.h>
#include <cassert>
#include <filesystem>
#include <functional>

#define DX_VERSION_MAJOR 0
#define DX_VERSION_MINOR 7

#ifdef _WIN32
#ifdef _MSC_VER
#if _MSC_VER < 1944
#error "MSVC version 14.43 (Visual Studio 2022 17.13) or greater is required to build on Windows."
#endif // _MSC_VER < 1944
#else // _MSC_VER
#error "MSVC version 14.43 (Visual Studio 2022 17.13) or greater is required to build on Windows."
#endif // _MSC_VER
#endif // _WIN32

#define symGetPluginDescriptor "getPluginDescriptor"

#ifdef _WIN32
#define ExDK_PLUGIN_EXPORT __declspec(dllexport)
#define ExDK_PLUGIN_CALL __cdecl
#else
#define ExDK_PLUGIN_EXPORT __attribute__((visibility("default")))
#define ExDK_PLUGIN_CALL
#endif

namespace ExtenDeckSDK {
extern "C" {
struct SubscriptionToken;
struct EventDescriptor;

enum DxStatuses {
    DxStatRunning     = 0,
    DxStatDone        = 1,
    DxStatExiting     = 2,
    DxStatRestart     = 4,
    DxStatRestarting  = 8,
};

enum DxModes {
    DxModeServer      = 0,
    DxModeClient      = 1,
    DxModeUndefined   = 0xFFFF
};

enum DxTasks {
    DxTaskNone            = 0,
    DxTaskSingleInstance  = 1,
    DxTaskServer          = 2,
    DxTaskClient          = 2,
    DxTaskDiscovery       = 4,
    DxTaskSetup           = 8,
};

struct DxObjectBase {
    std::string_view type_name;
    std::string_view display_name;
};

inline short dx_get_version_major() {
    return DX_VERSION_MAJOR;
}
inline short dx_get_version_minor() {
    return DX_VERSION_MINOR;
}

enum CallbackExecutionPolicy {
    DxECEPAll = 0,
    DxECEPImmediate = 1,
    DxECEPSubscriberThread = 2,
    DxECEPWorkerThread = 4,
};

struct EventBase {
    EventDescriptor* descriptor;
};

typedef void(*EventCallback)(const struct EventBase*, void*);

enum TextureStatus {
    DxTSUninitialized = -2,
    DxTSFailure = -1,
    DxTSSuccess = 0,
    DxTSReadyToBind = 1
};

struct Message {
    std::string msg;
    std::string_view type_name;
};

struct Client;
struct Profile;
struct EventBus;
struct Texture;

struct ClientAPI {
    struct Client*(*current)(void);
    struct Profile*(*currentProfile)(struct Client* client);
    struct Client*(*byUUID)(uint64_t uuid);
    uint64_t(*getUUID)(struct Client* client);
    const char*(*getNickname)(struct Client* client);
    bool(*isConnected)(struct Client* client);
    int(*send)(struct Client* client, struct Message msg, struct PluginBase* sender);
};

struct ProfileAPI {
    const char*(*configDir)(struct Profile* profile); // TODO: Make this a variable instead so i dont have to free it each time
    struct Client*(*getParentClient)(struct Profile* client);
};

struct EventAPI {
    struct EventBus* bus;

    struct SubscriptionToken*(*subscribe)(struct EventBus* bus, struct EventDescriptor* type,
                                          EventCallback callback, void* ctx,
                                          uint64_t callbackExecutionPolicy);
    void(*post)(struct EventBus* bus, struct EventBase* event);
    void(*unsubscribe)(struct EventBus* bus, struct SubscriptionToken* token);
    struct EventDescriptor*(*registerType)(struct EventBus* bus, const char* type_name, uint64_t typename_len, uint64_t size, uint64_t execution_policy, void(*constructor)(struct EventBase*), void(*destructor)(struct EventBase*));
    struct EventDescriptor*(*registerPrivateType)(struct EventBus* bus, const char* type_name, uint64_t typename_len, uint64_t size, uint64_t execution_policy, struct PluginBase* plugin, void(*constructor)(struct EventBase*), void(*destructor)(struct EventBase*));
    void(*unregisterType)(struct EventBus* bus, struct EventDescriptor*, struct PluginBase* plugin);
    struct EventDescriptor*(*findDescriptorByTypename)(struct EventBus* bus, const char* type_name, uint64_t typename_len);
    struct EventDescriptor*(*findPrivateDescriptorByTypename)(struct EventBus* bus, const char* type_name, uint64_t typename_len, struct PluginBase* plugin);
    struct EventBase*(*create)(struct EventBus* bus, struct EventDescriptor*);
    void(*retain)(struct EventBus* bus, struct EventBase* event);
    void(*release)(struct EventBus* bus, struct EventBase* event);
};
inline const char* eventClientConnectedTypename = "org.PoS.ExtenDeck.events.ClientConnected";
inline const char* eventClientDisconnectedTypename = "org.PoS.ExtenDeck.events.ClientDisconnected";
inline const char* eventClientDisconnectIssuedTypename = "org.PoS.ExtenDeck.events.ClientDisconnectIssued";
inline const char* eventServerConnectedTypename = "org.PoS.ExtenDeck.events.ServerConnected";
inline const char* eventServerDisconnectedTypename = "org.PoS.ExtenDeck.events.ServerDisconnected";

struct GraphicsAPI {
    struct Texture*(*createTextureFromFile)(const char* path, uint64_t path_len);
    struct Texture*(*createTextureFromMemory)(const uint8_t* data, uint64_t len);
    int(*writeTextureAsPNG)(struct Texture* texture, const char* outputPath, uint64_t outputPathLen);
    int(*getWidth)(struct Texture* texture);
    int(*getHeight)(struct Texture* texture);
    void(*destroyTexture)(struct Texture* texture);
    void(*overlayTexture)(struct Texture* texture, int w, int h);
    unsigned int(*getTextureID)(struct Texture* texture);
    bool(*isTextureInit)(struct Texture* texture);
    bool(*isTextureReadyToBind)(struct Texture* texture);
    bool(*isTextureNull)(struct Texture* texture);
};

struct DxFunctions {
    uint64_t(*generateUUID)();
    void(*freeString)(const char*);
};
struct DxDataMembers {
    const uint64_t* const mode;
    const uint64_t* const status;
    const uint64_t* const tasks;
};
struct DxAPI {
    const struct ClientAPI* client;
    const struct ProfileAPI* profile;
    const struct EventAPI* event;
    const struct GraphicsAPI* graphics;
    const DxFunctions* generalFunctions;
    const DxDataMembers* dataMembers;
};

enum PluginType {
    DxPTHelper = 1, // e.g. CPU monitor
    DxPTView = 2, // e.g. ShortcutView
    DxPTExtensionOf = 4, // e.g. Item for ShortcutView
    DxPTOverlay = 8, // e.g. Notification toasts
};

enum PluginFlags {
    DxPFNone = 0,
    DxPFExtensible = 1,
};

enum PluginSupportedParties {
    DxPSPClient = 1,
    DxPSPServer = 2,
    DxPSPAll = 3
};

enum PluginRequiredParties {
    DxPRPClient = 1,
    DxPRPServer = 2,
    DxPRPAll = 3
};

struct HelperInterface {
    void* reserved;
    void* reserved2;
    void* reserved3;
    void* reserved4;
    void* reserved5;
};

struct ViewInterface {
    struct DxObjectBase view_meta;
    struct ViewBase*(*createView)(struct PluginBase* parent_plugin, struct Profile* parent_profile, uint64_t runtimeID);
    void(*destroyView)(struct ViewBase* view);
};

struct ExtensionInterface {
    const char* extended_plugin_typename;
};

struct PluginBase;

struct PluginDescriptor {
    struct DxObjectBase base;
    struct PluginBase*(*create)(struct DxAPI* api);
    void(*destroy)(struct PluginBase* plugin);
    const enum PluginType type;
    const enum PluginFlags flags;
    const enum PluginSupportedParties supportedParties;
    const enum PluginRequiredParties requiredParties;
    const uint16_t versionMajor;
    const uint8_t versionMinor;
    const uint8_t patchVersion;
};

struct PluginBase {
    const struct PluginDescriptor* const descriptor;
    struct DxAPI* api;

    struct HelperInterface* helper_interface;
    struct ExtensionInterface* extension_interface;
    struct ViewInterface* view_interface;

    void(*onMessage)(struct PluginBase* plugin, struct Client* sender, uint64_t messageTypeHash, const char* msg, uint64_t data);
};

//ExDK_PLUGIN_EXPORT const PluginDescriptor* ExDK_PLUGIN_CALL getPluginDescriptor(void);

struct ViewBase {
    struct DxObjectBase base;
    uint64_t runtimeID;
    struct Profile* parent_profile;
    struct PluginBase* parent_plugin;
    void(*draw)(struct ViewBase* context);
    void(*drawProperties)(struct ViewBase* context);
    void(*propertiesApply)(struct ViewBase* context);
    void(*propertiesCancel)(struct ViewBase* context);
};

// Allowed handlers: DxECEPImmediate
struct EClientDisconnected {
    struct EventBase base;
    uint64_t clientID;
};

struct EClientDisconnectIssued {
    struct EventBase base;
    uint64_t clientID;
};

struct PluginBase* createPlugin(struct DxAPI* api);
void destroyPlugin(struct PluginBase* plugin);
}

class API {
    inline static DxAPI* dxAPI = nullptr;
public:
    static void init(DxAPI* api) {
        dxAPI = api;
    }
    static DxAPI* getAPI() {
        return dxAPI;
    }
};
inline Texture* createTextureFromFile(const std::filesystem::path& path) {
    assert(API::getAPI());
    return API::getAPI()->graphics->createTextureFromFile(path.string().c_str(), path.string().length());
}
inline Texture* createTextureFromMemory(const uint8_t* data, uint64_t size) {
    assert(API::getAPI());
    return API::getAPI()->graphics->createTextureFromMemory(data, size);
}
inline void overlayTexture(Texture* texture, int w, int h) {
    assert(API::getAPI());
    return API::getAPI()->graphics->overlayTexture(texture, w, h);
}
inline uint64_t generateUUID() {
    assert(API::getAPI());
    return API::getAPI()->generalFunctions->generateUUID();
}
inline const DxStatuses& getStatus() {
    assert(API::getAPI());
    return (const DxStatuses&)*API::getAPI()->dataMembers->status;
}
inline DxModes getMode() {
    assert(API::getAPI());
    return (DxModes)*API::getAPI()->dataMembers->mode;
}
inline const DxTasks& getTasks() {
    assert(API::getAPI());
    return (const DxTasks&)*API::getAPI()->dataMembers->tasks;
}
struct Client {
    static Client* getCurrentClient() {
        assert(API::getAPI());
        return API::getAPI()->client->current();
    }
    Profile* getCurrentProfile() {
        assert(API::getAPI());
        return API::getAPI()->client->currentProfile(this);
    }
    static Client* getClientByUUID(uint64_t uuid) {
        assert(API::getAPI());
        return API::getAPI()->client->byUUID(uuid);
    }
    uint64_t getUUID() {
        assert(API::getAPI());
        return API::getAPI()->client->getUUID(this);
    }
    std::string getNickname() {
        assert(API::getAPI());
        return API::getAPI()->client->getNickname(this);
    }
    bool isConnected() {
        assert(API::getAPI());
        return API::getAPI()->client->isConnected(this);
    }
    int send(Message msg, PluginBase* sender) {
        assert(API::getAPI());
        return API::getAPI()->client->send(this, msg, sender);
    }
    ~Client() = delete;
};
struct Profile {
    std::filesystem::path getConfigDir() {
        assert(API::getAPI());
        const char* buf = API::getAPI()->profile->configDir(this);
        std::filesystem::path path = buf;
        API::getAPI()->generalFunctions->freeString(buf);
        return path;
    }
    Client* getParentClient() {
        assert(API::getAPI());
        return API::getAPI()->profile->getParentClient(this);
    }
    ~Profile() = delete;
};
struct EventBus {
    using EventTypeConstructor = void(*)(EventBase*);
    using EventTypeDestructor = EventTypeConstructor;
    using upDxSubTok = std::unique_ptr<SubscriptionToken, std::function<void(SubscriptionToken*)>>;
    upDxSubTok subscribeToEvent(EventDescriptor* type, EventCallback callback, void* ctx, uint64_t callbackExecutionPolicy) {
        assert(API::getAPI());
        static const std::function<void(SubscriptionToken*)> eventSubscriptionTokenDeleter = [this](SubscriptionToken* token) {
            assert(API::getAPI());
            API::getAPI()->event->unsubscribe(this, token);
        };
        return upDxSubTok(API::getAPI()->event->subscribe(this, type, callback, ctx, callbackExecutionPolicy),
                          eventSubscriptionTokenDeleter);
    }
    void postEvent(EventBase* event) {
        assert(API::getAPI());
        API::getAPI()->event->post(this, event);
    }
    void unsubscribeFromEvent(upDxSubTok& token) {
        if (!token)
            return;
        token.reset();
    }
    EventDescriptor* registerEventType(std::string type_name, uint64_t size, uint64_t execution_policy, EventTypeConstructor constructor, EventTypeDestructor destructor) {
        assert(API::getAPI());
        return API::getAPI()->event->registerType(this, type_name.c_str(), type_name.length(), size, execution_policy, constructor, destructor);
    }
    EventDescriptor* registerPrivateEventType(std::string type_name, uint64_t size, uint64_t execution_policy, PluginBase* owner, EventTypeConstructor constructor, EventTypeDestructor destructor) {
        assert(API::getAPI());
        return API::getAPI()->event->registerPrivateType(this, type_name.c_str(), type_name.length(), size, execution_policy, owner, constructor, destructor);
    }
    void unregisterType(EventDescriptor* descriptor, PluginBase* caller) {
        assert(API::getAPI());
        return API::getAPI()->event->unregisterType(this, descriptor, caller);
    }
    EventDescriptor* findDescriptorByTypename(std::string type_name) {
        assert(API::getAPI());
        return API::getAPI()->event->findDescriptorByTypename(this, type_name.c_str(), type_name.length());
    }
    EventDescriptor* findPrivateDescriptorByTypename(std::string type_name, PluginBase* caller) {
        assert(API::getAPI());
        return API::getAPI()->event->findPrivateDescriptorByTypename(this, type_name.c_str(), type_name.length(), caller);
    }
    EventBase* createEvent(EventDescriptor* descriptor) {
        assert(API::getAPI());
        return API::getAPI()->event->create(this, descriptor);
    }
    void retainEvent(EventBase* event) {
        assert(API::getAPI());
        API::getAPI()->event->retain(this, event);
    }
    void releaseEvent(EventBase* event) {
        assert(API::getAPI());
        API::getAPI()->event->release(this, event);
    }
};
struct Event {
    static EventBus* getDefaultEventBus() {
        assert(API::getAPI());
        return API::getAPI()->event->bus;
    }
    static std::string_view eventClientConnectedTypename;
    static std::string_view eventClientDisconnectedTypename;
    static std::string_view eventClientDisconnectIssuedTypename;
    static std::string_view eventServerConnectedTypename;
    static std::string_view eventServerDisconnectedTypename;
};
inline std::string_view Event::eventClientConnectedTypename = ExtenDeckSDK::eventClientConnectedTypename;
inline std::string_view Event::eventClientDisconnectedTypename = ExtenDeckSDK::eventClientDisconnectedTypename;
inline std::string_view Event::eventClientDisconnectIssuedTypename = ExtenDeckSDK::eventClientDisconnectIssuedTypename;
inline std::string_view Event::eventServerConnectedTypename = ExtenDeckSDK::eventServerConnectedTypename;
inline std::string_view Event::eventServerDisconnectedTypename = ExtenDeckSDK::eventServerDisconnectedTypename;

using TextureDeleterFun = std::function<void(Texture*)>;
const TextureDeleterFun deleter = [](Texture* texture) {
    assert(API::getAPI());
    API::getAPI()->graphics->destroyTexture(texture);
};
struct Texture {
    int writeTextureAsPNG(const std::filesystem::path& path) {
        assert(API::getAPI());
        return API::getAPI()->graphics->writeTextureAsPNG(this, path.string().c_str(), path.string().length());
    }
    int getHeight() {
        assert(API::getAPI());
        return API::getAPI()->graphics->getHeight(this);
    }
    int getWidth() {
        assert(API::getAPI());
        return API::getAPI()->graphics->getWidth(this);
    }
    void destroy() {
        assert(API::getAPI());
        return API::getAPI()->graphics->destroyTexture(this);
    }
    unsigned int getTextureID() {
        assert(API::getAPI());
        return API::getAPI()->graphics->getTextureID(this);
    }
    bool isInit() {
        assert(API::getAPI());
        return API::getAPI()->graphics->isTextureInit(this);
    }
    bool readyToBind() {
        assert(API::getAPI());
        return API::getAPI()->graphics->isTextureReadyToBind(this);
    }
    bool isNull() {
        assert(API::getAPI());
        return API::getAPI()->graphics->isTextureNull(this);
    }
    std::unique_ptr<Texture, TextureDeleterFun> toUniquePtr() {
        return std::unique_ptr<Texture, TextureDeleterFun>(this, deleter);
    }
    std::shared_ptr<Texture> toSharedPtr() {
        return std::shared_ptr<Texture>(this, deleter);
    }

    ~Texture() = delete;
};
}
