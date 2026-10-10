#pragma once

#include <stdint.h>

#define DX_VERSION_MAJOR 0
#define DX_VERSION_MINOR 7

#ifdef _WIN32
#ifdef _MSC_VER
#if _MSC_VER < 1944
#error "MSVC version 14.43 (Visual Studio 2022 17.13) or greater is required to build on Windows."
#endif // _MSC_VER < 1944
#else
#error "MSVC version 14.43 (Visual Studio 2022 17.13) or greater is required to build on Windows."
#endif // _MSC_VER
#endif // _WIN32

namespace ExtenDeckSDK {

struct DxClient;
struct DxProfile;
struct DxEventBus;
struct DxSubscriptionToken;
struct DxLocalEventBus;
struct DxThreadContext;
struct DxEventDescriptor;
struct DxEventBase;
struct DxPluginBase;

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
    const char* type_name;
    uint64_t typenameNameLen;
    const char* display_name;
    uint64_t displayNameLen;
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

struct DxEventBase {
    struct DxEventDescriptor* const descriptor;
};

typedef void(*EventCallback)(const struct DxEventBase*, void*);

enum TextureStatus {
    DxTSUninitialized = -2,
    DxTSFailure = -1,
    DxTSSuccess = 0,
    DxTSReadyToBind = 1
};

struct DxTexture {
    const uint64_t textureID;
    enum TextureStatus status;
};

struct Message {
    const char* msg;
    uint64_t msg_len;
    const char* type_name;
    uint64_t typename_len;
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
    int(*send)(struct Client* client, struct Message msg, struct DxPluginBase* sender);
};

struct ProfileAPI {
    const char*(*configDir)(struct Profile* profile);
    struct Client*(*getParentClient)(struct Profile* client);
};

struct EventAPI {
    struct EventBus* bus;

    struct DxSubscriptionToken*(*subscribe)(struct DxEventBus* bus, struct DxEventDescriptor* type,
                                          EventCallback callback, void* ctx,
                                          uint64_t callbackExecutionPolicy);
    void(*post)(struct DxEventBus* bus, struct DxEventBase* event);
    void(*unsubscribe)(struct DxEventBus* bus, struct DxSubscriptionToken* token);
    struct DxEventDescriptor*(*registerType)(struct DxEventBus* bus, const char* type_name, uint64_t typename_len, uint64_t size, uint64_t execution_policy, void(*constructor)(struct DxEventBase*), void(*destructor)(struct DxEventBase*));
    struct DxEventDescriptor*(*registerPrivateType)(struct DxEventBus* bus, const char* type_name, uint64_t typename_len, uint64_t size, uint64_t execution_policy, struct DxPluginBase* plugin, void(*constructor)(struct DxEventBase*), void(*destructor)(struct DxEventBase*));
    void(*unregisterType)(struct DxEventBus* bus, struct DxEventDescriptor*, struct DxPluginBase* plugin);
    struct DxEventDescriptor*(*findDescriptorByTypename)(struct DxEventBus* bus, const char* type_name, uint64_t typename_len);
    struct DxEventDescriptor*(*findPrivateDescriptorByTypename)(struct DxEventBus* bus, const char* type_name, uint64_t typename_len, struct DxPluginBase* plugin);
    struct DxEventBase*(*create)(struct DxEventBus* bus, struct DxEventDescriptor*);
    void(*retain)(struct DxEventBus* bus, struct DxEventBase* event);
    void(*release)(struct DxEventBus* bus, struct DxEventBase* event);
};

struct GraphicsAPI {
    struct DxTexture*(*createTextureFromFile)(const char* path, uint64_t path_len);
    struct DxTexture*(*createTextureFromMemory)(const uint8_t* data, uint64_t len);
    int(*writeTextureAsPNG)(struct DxTexture* texture, const char* outputPath, uint64_t outputPathLen);
    int(*getWidth)(struct DxTexture* texture);
    int(*getHeight)(struct DxTexture* texture);
    void(*destroyTexture)(struct DxTexture* texture);
    void(*overlayTexture)(struct DxTexture* texture, int w, int h);
};

struct DxAPI {
    uint64_t(*generateUUID)();
    const uint64_t* const status;
    const uint64_t* const mode;
    const uint64_t* const tasks;
    const struct ClientAPI* client;
    const struct ProfileAPI* profile;
    const struct EventAPI* event;
    const struct GraphicsAPI* graphics;
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
    struct ViewBase*(*createView)(struct DxPluginBase* parent_plugin, struct DxProfile* parent_profile, uint64_t runtimeID);
    void(*destroyView)(struct ViewBase* view);
};

struct ExtensionInterface {
    const char* extended_plugin_typename;
};

struct DxPluginBase {
    struct DxObjectBase base;
    DxAPI* api;
    const enum PluginType type;
    const enum PluginFlags flags;
    const enum PluginSupportedParties supportedParties;
    const enum PluginRequiredParties requiredParties;
    const uint16_t versionMajor;
    const uint8_t versionMinor;
    const uint8_t patchVersion;

    struct HelperInterface* helper_interface;
    struct ExtensionInterface* extension_interface;
    struct ViewInterface* view_interface;

    void(*onMessage)(struct DxPluginBase* plugin, struct DxClient* sender, uint64_t messageTypeHash, const char* msg, uint64_t data);
};

struct ViewBase {
    struct DxObjectBase base;
    uint64_t runtimeID;
    struct DxProfile* parent_profile;
    struct DxPluginBase* parent_plugin;
    void(*draw)(struct ViewBase* context);
    void(*drawProperties)(struct ViewBase* context);
    void(*propertiesApply)(struct ViewBase* context);
    void(*propertiesCancel)(struct ViewBase* context);
};

// Allowed handlers: DxECEPImmediate
struct EClientDisconnected {
    struct DxEventBase base;
    uint64_t clientID;
};

struct EClientDisconnectIssued {
    struct DxEventBase base;
    uint64_t clientID;
};

struct DxPluginBase* createPlugin(struct DxAPI* api);
void destroyPlugin(struct DxPluginBase* plugin);

#include <cassert>
#include <filesystem>
#include <functional>
#include <string>
struct Texture;
class API {
    static DxAPI* dxapi;
public:
    static Texture* createTextureFromFile(const std::filesystem::path& path) {
        assert(dxapi);
        return (Texture*)dxapi->graphics->createTextureFromFile(path.string().c_str(), path.string().length());
    }
    static Texture* createTextureFromMemory(const uint8_t* data, uint64_t size) {
        assert(dxapi);
        return (Texture*)dxapi->graphics->createTextureFromMemory(data, size);
    }
    static uint64_t generateUUID() {
        assert(dxapi);
        return dxapi->generateUUID();
    }
    static const DxStatuses& getStatus() {
        assert(dxapi);
        return (const DxStatuses&)*dxapi->status;
    }
    static DxModes getMode() {
        assert(dxapi);
        return (DxModes)*dxapi->mode;
    }
    static const DxTasks& getTasks() {
        assert(dxapi);
        return (const DxTasks&)*dxapi->tasks;
    }
    static void initAPI(DxAPI* api) {
        if (api)
            dxapi = api;
    }
    static DxAPI* getAPI() {
        return dxapi;
    }
};
    struct Profile;
    struct Client {
        static Client* getCurrentClient() {
            assert(API::getAPI());
            return (Client*)API::getAPI()->client->current();
        }
        Profile* getCurrentProfile() {
            assert(API::getAPI());
            return (Profile*)API::getAPI()->client->currentProfile((DxClient*)this);
        }
        static Client* getClientByUUID(uint64_t uuid) {
            assert(API::getAPI());
            return (Client*)API::getAPI()->client->byUUID(uuid);
        }
        uint64_t getUUID() {
            assert(API::getAPI());
            return API::getAPI()->client->getUUID((DxClient*)this);
        }
        std::string getNickname() {
            assert(API::getAPI());
            return API::getAPI()->client->getNickname((DxClient*)this);
        }
        bool isConnected() {
            assert(API::getAPI());
            return API::getAPI()->client->isConnected((DxClient*)this);
        }
        int send(Message msg, DxPluginBase* sender) {
            assert(API::getAPI());
            return API::getAPI()->client->send((DxClient*)this, msg, sender);
        }
        ~Client() = delete;
    };
    struct Profile {
        std::filesystem::path getConfigDir() {
            assert(API::getAPI());
            return API::getAPI()->profile->configDir((DxProfile*)this);
        }
        Client* getParentClient() {
            assert(API::getAPI());
            return (Client*)API::getAPI()->profile->getParentClient((DxProfile*)this);
        }
        ~Profile() = delete;
    };
    struct EventBus {
        using EventTypeConstructor = void(*)(DxEventBase*);
        using EventTypeDestructor = EventTypeConstructor;
        const std::function<void(DxSubscriptionToken*)> eventSubscriptionTokenDeleter = [this](DxSubscriptionToken* token) {
            assert(API::getAPI());
            API::getAPI()->event->unsubscribe((DxEventBus*)this, token);
        };
        using upDxSubTok = std::unique_ptr<DxSubscriptionToken, std::function<void(DxSubscriptionToken*)>>;
        upDxSubTok subscribeToEvent(DxEventDescriptor* type, EventCallback callback, void* ctx, uint64_t callbackExecutionPolicy) {
            assert(API::getAPI());
            return upDxSubTok(API::getAPI()->event->subscribe((DxEventBus*)this, type, callback, ctx, callbackExecutionPolicy),
                              eventSubscriptionTokenDeleter);
        }
        void postEvent(DxEventBase* event) {
            assert(API::getAPI());
            API::getAPI()->event->post((DxEventBus*)this, event);
        }
        void unsubscribeFromEvent(upDxSubTok& token) {
            if (!token)
                return;
            token.reset();
        }
        DxEventDescriptor* registerEventType(std::string type_name, uint64_t size, uint64_t execution_policy, EventTypeConstructor constructor, EventTypeDestructor destructor) {
            assert(API::getAPI());
            return API::getAPI()->event->registerType((DxEventBus*)this, type_name.c_str(), type_name.length(), size, execution_policy, constructor, destructor);
        }
        DxEventDescriptor* registerPrivateEventType(std::string type_name, uint64_t size, uint64_t execution_policy, DxPluginBase* owner, EventTypeConstructor constructor, EventTypeDestructor destructor) {
            assert(API::getAPI());
            return API::getAPI()->event->registerPrivateType((DxEventBus*)this, type_name.c_str(), type_name.length(), size, execution_policy, owner, constructor, destructor);
        }
        void unregisterType(DxEventDescriptor* descriptor, DxPluginBase* caller) {
            assert(API::getAPI());
            return API::getAPI()->event->unregisterType((DxEventBus*)this, descriptor, caller);
        }
        DxEventDescriptor* findDescriptorByTypename(std::string type_name) {
            assert(API::getAPI());
            return API::getAPI()->event->findDescriptorByTypename((DxEventBus*)this, type_name.c_str(), type_name.length());
        }
        DxEventDescriptor* findPrivateDescriptorByTypename(std::string type_name, DxPluginBase* caller) {
            assert(API::getAPI());
            return API::getAPI()->event->findPrivateDescriptorByTypename((DxEventBus*)this, type_name.c_str(), type_name.length(), caller);
        }
        DxEventBase* createEvent(DxEventDescriptor* descriptor) {
            assert(API::getAPI());
            return API::getAPI()->event->create((DxEventBus*)this, descriptor);
        }
        void retainEvent(DxEventBase* event) {
            assert(API::getAPI());
            API::getAPI()->event->retain((DxEventBus*)this, event);
        }
        void releaseEvent(DxEventBase* event) {
            assert(API::getAPI());
            API::getAPI()->event->release((DxEventBus*)this, event);
        }
    };
    struct Event {
        static EventBus* getDefaultEventBus() {
            assert(API::getAPI());
            return (EventBus*)API::getAPI()->event->bus;
        }
    };
    using TextureDeleterFun = std::function<void(Texture*)>;
    const TextureDeleterFun deleter = [](Texture* texture) {
        assert(API::getAPI());
        API::getAPI()->graphics->destroyTexture((DxTexture*)texture);
    };
    struct Texture {
        const uint64_t textureID;
        TextureStatus status;
        int writeTextureAsPNG(const std::filesystem::path& path) {
            assert(API::getAPI());
            return API::getAPI()->graphics->writeTextureAsPNG((DxTexture*)this, path.string().c_str(), path.string().length());
        }
        int getHeight() {
            assert(API::getAPI());
            return API::getAPI()->graphics->getHeight((DxTexture*)this);
        }
        int getWidth() {
            assert(API::getAPI());
            return API::getAPI()->graphics->getWidth((DxTexture*)this);
        }
        void destroyTexture() {
            assert(API::getAPI());
            return API::getAPI()->graphics->destroyTexture((DxTexture*)this);
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
