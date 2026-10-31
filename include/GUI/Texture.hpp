#pragma once

#include <filesystem>
#include <mutex>
#include <string>

using std::string;

namespace fs = std::filesystem;

namespace ExtenDeck::GUI {
// Function returns void but sets the internal status variable
typedef void vint;
enum class TextureStatus {
    Uninitialized = -2,
    Failure = -1,
    Success = 0,
    ReadyToBind = 1
};
struct Texture {
private:
    unsigned int        texture_id = 0;
    TextureStatus       status = TextureStatus::Uninitialized;
    std::mutex          lock;
    int                 w = 0;
    int                 h = 0;
    uint8_t*            data = nullptr;
    void                _shrink_to_fit();
    vint                _load_from_file(const string& filename);
    vint                _load_from_memory(const uint8_t* raw, uint64_t size);
    vint                finalize_creation();
public:
    vint                create_from_file(const string& filename);
    vint                create_from_memory(const uint8_t* data, uint64_t size);
    int                 bind_to_context();
    int                 write_png(fs::path path);
    void                garbage_collect();
    const unsigned int* get() const;
    unsigned int        getTextureID() const;
    int                 get_width() const;
    int                 get_height() const;
    bool                is_init() const;
    bool                ready_to_bind() const;
    bool                is_null() const;
    void                destroy();

    Texture() = default;
    Texture(const fs::path& path);
    Texture(const uint8_t* data, unsigned int size);

    bool operator==(const Texture& other) const {
        return this->texture_id == other.texture_id;
    }
    bool operator!=(const Texture& other) const {
        return this->texture_id != other.texture_id;
    }
    ~Texture() {
        this->destroy();
    }
};
}