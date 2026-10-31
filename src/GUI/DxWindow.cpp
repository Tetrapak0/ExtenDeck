#ifdef _WIN32
 #define WIN32_LEAN_AND_MEAN
 #define _CRT_SECURE_NO_WARNINGS 1

 #define GLFW_EXPOSE_NATIVE_WIN32
 #include <dwmapi.h>
#endif

#include "GUI/GUI.hpp"
#include "GUI/DxWindow.hpp"
#include "GUI/Texture.hpp"

#include "stb/stb_image.h"
#include "GLFW/include/glfw3native.h"

extern "C" {
    #include "../../resources/icon.h"
}

GLFWwindow* DxWindow::get() const {
    return window;
}

bool DxWindow::should_close() const {
    assert(window);
    return glfwWindowShouldClose(window);
}

void DxWindow::register_close_callback(const GLFWwindowclosefun callback) const {
    assert(window);
    glfwSetWindowCloseCallback(window, callback);
}
void DxWindow::register_resize_callback(const GLFWwindowsizefun callback) const {
    assert(window);
    glfwSetWindowSizeCallback(window, callback);
}
void DxWindow::register_move_callback(const GLFWwindowposfun callback) const {
    assert(window);
    glfwSetWindowPosCallback(window, callback);
}

void DxWindow::resize(const Vec2<int>& size) const {
    assert(window);
    glfwSetWindowSize(window, size.x, size.y);
}
const Vec2<int>& DxWindow::get_size() {
    assert(window);
    glfwGetWindowSize(window, &size.x, &size.y);
    return size;
}

int DxWindow::create(const char* title, const Vec2<int>& size, const int fullscreen,
                     const std::vector<std::pair<int, int>>& hints,
                     const Vec2<int>& min_size, const Vec2<int>& max_size) {
    assert(!window);
    if (!glfwInit()) {
        const char* glfwerr;
        int glfwcode = glfwGetError(&glfwerr);
        fprintf(stderr, "GLFW failed to initialize with code %d: %s\n", glfwcode, glfwerr);
        return -1;
    }
    bool auto_iconify = false;
    bool resizable = true;

    for (auto& [hint, value] : hints) {
        // all we realistically need
        assert(value == GLFW_FALSE || value == GLFW_TRUE);
        assert(hint == GLFW_RESIZABLE || hint == GLFW_VISIBLE || hint == GLFW_DECORATED ||
               hint == GLFW_FOCUSED || hint == GLFW_AUTO_ICONIFY || hint == GLFW_FLOATING ||
               hint == GLFW_MAXIMIZED || hint == GLFW_CENTER_CURSOR || hint == GLFW_FOCUS_ON_SHOW ||
               hint == GLFW_SCALE_TO_MONITOR || hint == GLFW_TRANSPARENT_FRAMEBUFFER ||
               hint == GLFW_SCALE_FRAMEBUFFER || hint == GLFW_DOUBLEBUFFER);

        if (hint == GLFW_AUTO_ICONIFY && value == GLFW_TRUE) {
            auto_iconify = true;
        } else if (hint == GLFW_RESIZABLE && value == GLFW_FALSE) {
            resizable = false;
        }
        glfwWindowHint(hint, value);
    }

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    // glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    if (!fullscreen) {
        window = glfwCreateWindow(size.x, size.y, title, nullptr, nullptr);
    } else if (fullscreen == -1) {
        // Windowed fullscreen
        if (!auto_iconify)
            glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        window = glfwCreateWindow(mode->width, mode->height, title, nullptr, nullptr);
    } else if (fullscreen == 1) {
        window = glfwCreateWindow(mode->width, mode->height, title, glfwGetPrimaryMonitor(), nullptr);
    }

    if (!window) {
        const char* glfwerr;
        const int glfwcode = glfwGetError(&glfwerr);
        fprintf(stderr, "Failed to create GLFW window with code %d: %s\n", glfwcode, glfwerr);
        return -1;
    }
    glfwMakeContextCurrent(window);
    // glEnable(0x8DB9); // GL_FRAMEBUFFER_SRGB
    glfwSwapInterval(1);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwShowWindow(window);
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
#endif
    if (resizable)
        glfwSetWindowSizeLimits(window, min_size.x, min_size.y, max_size.x, max_size.y);
    else
        glfwSetWindowSizeLimits(window, size.x, size.y, size.x, size.y);
    GLFWimage icon;
    int channels = 0;
    icon.pixels = stbi_load_from_memory(icon_png, icon_png_len, &icon.width, &icon.height, &channels, 4);
    if (icon.pixels) {
        glfwSetWindowIcon(this->window, 1, &icon);
        stbi_image_free(icon.pixels);
    }

    return 0;
}

void DxWindow::show(bool focus) {
    if (glfwGetWindowAttrib(this->window, GLFW_ICONIFIED))
        glfwRestoreWindow(this->window);
    glfwShowWindow(this->window);
    if (focus)
        glfwFocusWindow(this->window);
}

void DxWindow::hide() {
    glfwHideWindow(this->window);
}

void DxWindow::destroy() {
    glfwDestroyWindow(window);
    window = nullptr;
    glfwTerminate();
}
