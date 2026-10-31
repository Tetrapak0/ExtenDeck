#include "Config/Deckastore.hpp"
#include "GUI/Texture.hpp"
#include "Event/Event.hpp"

#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>
#include <stb/stb_image_write.h>

#include <cmath>
#include <print>
#include <thread>

using std::string;
using std::thread;

namespace ExtenDeck::GUI {
Texture::Texture(const fs::path& path) {
    create_from_file(path.string());
}

Texture::Texture(const uint8_t* data, unsigned int size) {
    create_from_memory(data, size);
}
// This is allowed to fail. The VRAM usage will just be unnecessarily higher.
void Texture::_shrink_to_fit() {
    if (h > 512) {
        int max_h = 512;
        int max_w = std::round((float)w * ((float)max_h / (float)h));

        std::println("New width: {}", max_w);
        std::println("Resized image requires {} bytes.", max_w * max_h * 4);

        uint8_t* resized = new uint8_t[max_w * max_h * 4];

        int stride     = w * 4;
        int stride_new = max_w * 4;

        if (!stbir_resize_uint8_linear(data, w, h, stride, resized, max_w, max_h,
                                       stride_new, static_cast<stbir_pixel_layout>(4))) {
            fprintf(stderr, "Image resize failed.\n");
            status = TextureStatus::Failure;
            delete[] resized;
            return;
        }

        this->w = max_w;
        this->h = max_h;

        stbi_image_free(data);
        data = resized;
    }
}
vint Texture::_load_from_file(const string& filename) {
    int channels;
    data = stbi_load(filename.c_str(), &w, &h, &channels, 4);
    if (!data) {
        fprintf(stderr, "Failed to load image %s\n", stbi_failure_reason());
        fprintf(stderr, "filename %s\n", filename.c_str());
        status = TextureStatus::Failure;
        return;
    }
    std::println("Texture requires {} bytes.", 4 * w * h);
}

vint Texture::_load_from_memory(const uint8_t* raw, uint64_t size) {
    data = stbi_load_from_memory(raw, size, &w, &h, nullptr, 4);
    if (!data) {
        fprintf(stderr, "Failed to load image %s\n", stbi_failure_reason());
        this->status = TextureStatus::Failure;
        return;
    }
    std::println("Texture requires {} bytes.", 4 * w * h);
}

vint Texture::finalize_creation() {
    if (status == TextureStatus::Failure) {
        this->destroy();
        status = TextureStatus::Failure;
        return;
    }

    this->_shrink_to_fit();

    status = TextureStatus::ReadyToBind;

    Deckastore& ds = Deckastore::get();
    Event::ETextureReadyToBind* event = (Event::ETextureReadyToBind*)ds.eventBus.registry.createEvent(
                                            ds.eventBus.registry.findPrivateDescriptorByName(
                                                "org.PoS.ExtenDeck.events.TextureReadyToBind", ds.applicationPlugin->plugin
                                            )
                                        );
    event->texture = this;
    ds.eventBus.postEvent(event);
    ds.eventBus.registry.releaseEvent(event);
}

vint Texture::create_from_file(const string& filename) {
    if (texture_id)
        this->destroy();
    // TODO: Look at async, future and promise
    thread generator([this, filename] {
        _load_from_file(filename);
        finalize_creation();
    });
    generator.detach();
}

vint Texture::create_from_memory(const uint8_t* data, uint64_t size) {
    if (texture_id)
        this->destroy();
    thread generator([this, data, size] {
        _load_from_memory(data, size);
        finalize_creation();
    });
    generator.detach();
}

int Texture::bind_to_context() {
    glGenTextures(1, &texture_id);
    if (texture_id) {
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        } else {
            this->destroy();
            status = TextureStatus::Failure;
            return -1;
        }

        status = TextureStatus::Success;
    } else {
        status = TextureStatus::Failure;
    }

    this->garbage_collect();
    return 0;
}

int Texture::write_png(fs::path path) {
    std::unique_lock lock(this->lock);
    if (path.empty())
        return -1;
    while (status == TextureStatus::ReadyToBind);
    if (status == TextureStatus::Success) {
        uint8_t* texture_data = new uint8_t[w*h*4];
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
        stbi_write_png(path.string().c_str(), w, h, 4, texture_data, 4*w);
        delete[] texture_data;
        return 0;
    }
    return -1;
}

void Texture::garbage_collect() {
    if (data) {
        stbi_image_free(data);
        data = nullptr;
    }
}

const GLuint* Texture::get() const {
    return &texture_id;
}

GLuint Texture::getTextureID() const {
    return texture_id;
}

int Texture::get_width() const {
    return w;
}

int Texture::get_height() const {
    return h;
}

bool Texture::is_init() const {
    return status == TextureStatus::Success;
}

bool Texture::ready_to_bind() const {
    return status == TextureStatus::ReadyToBind;
}

bool Texture::is_null() const {
    return !texture_id;
}

void Texture::destroy() {
    if (texture_id) {
        glDeleteTextures(1, &texture_id);
        texture_id = NULL;
    }
    this->garbage_collect();
    w = 0;
    h = 0;
    status = TextureStatus::Uninitialized;
}
}
