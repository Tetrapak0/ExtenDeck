#include "ShortcutView/Item/Item.hpp"
#include "ShortcutView/Item/ExecutableItem.hpp"
#include "ShortcutView/ShortcutView.hpp"
#include "ShortcutView/Item/FolderItem.hpp"

#include <imgui/imgui_stdlib.h>

#include "Utilities/FileDialog.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

namespace ShortcutView::Item {
ExecutableItem::ExecutableItem(json* config, ShortcutView& parentView, FolderItem* parent) : Item(config, parentView, parent) {
    if (config) {
        if (config->contains("command") && (*config)["command"].is_string()) {
            this->command = (*config)["command"].get<string>();
            this->m_command = command;
        }
        if (config->contains("args") && (*config)["args"].is_string()) {
            this->args = (*config)["args"].get<string>();
            this->m_args = args;
        }
        if (config->contains("cwd") && (*config)["cwd"].is_string()) {
            this->cwd = (*config)["cwd"].get<string>();
            this->m_cwd = cwd;
        }
        if (config->contains("admin") && (*config)["admin"].is_boolean()) {
            this->admin = (*config)["admin"].get<bool>();
            this->m_admin = admin;
        }
        if (config->contains("console") && (*config)["console"].is_boolean()) {
            this->console = (*config)["console"].get<bool>();
            this->m_console = console;
        }
        if (ExtenDeckSDK::getMode() == ExtenDeckSDK::DxModeServer)
            args_to_argv();
    }
}

void ExecutableItem::drawProperties() {
    ImGui::BeginDisabled();
    ImGui::TextWrapped("(i) Please do not use environment variables. Write your full user path (if applicable), not ~/");
    ImGui::EndDisabled();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Path: ");
    ImGui::SameLine();
    ImGui::InputText("##cmd", &this->m_command);
    ImGui::SameLine();
    if (ImGui::Button("Browse")) {
        string res = ExtenDeck::Utilities::nfd_open_exe();
        if (!res.empty())
            m_command = res;
    }
    ImGui::BeginDisabled();
#ifdef _WIN32
    ImGui::TextWrapped(R"((i) If you cannot see some shortcuts, try looking at the public desktop or select the "Filesystem / URL" type in the "General" tab.)");
#else
    ImGui::TextWrapped(R"((i) If you cannot see some shortcuts, try selecting the "Filesystem / URL" type in the "General" tab.)");
#endif
    ImGui::EndDisabled();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Arguments: ");
    ImGui::SameLine();
    ImGui::InputText("##args", &this->m_args);
    ImGui::BeginDisabled();
    ImGui::TextWrapped("(i) Arguments that contain spaces (i.e. paths) should be put in double quotes");
    ImGui::EndDisabled();
#ifdef _WIN32
    ImGui::Checkbox("Run with administrator rights", &this->m_admin);
#endif
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Working directory: ");
    ImGui::SameLine();
    ImGui::InputText("##cwd", &this->m_cwd);
    ImGui::SameLine();
    ImGui::PushID(69);
    if (ImGui::Button("Browse")) {
        string res = ExtenDeck::Utilities::nfd_open_dir();
        if (!res.empty())
            m_cwd = res;
    }
    ImGui::PopID();
    ImGui::BeginDisabled();
    ImGui::TextWrapped("(i) If empty, ExtenDeck's current working directory will be used.");
    ImGui::EndDisabled();
#ifndef _WIN32
    ImGui::Checkbox("Show console", &this->m_console);
    // TODO: terminal selection
    // ImGui::BeginDisabled();
    // ImGui::TextWrapped("(i) This will use the first good terminal if one is not specified below:");
    // ImGui::EndDisabled();
#endif
}

bool ExecutableItem::isDisabled() {
    return m_command.empty();
}

void ExecutableItem::propertiesCancel() {
    m_command = command;
    m_args = args;
    m_cwd = cwd;
    m_admin = admin;
    m_console = console;
}

void ExecutableItem::properties_apply() {
    command = m_command;
    args = m_args;
    cwd = m_cwd;
    admin = m_admin;
    console = m_console;
    (*config)["command"] = command;
    (*config)["args"] = args;
    (*config)["cwd"] = cwd;
    (*config)["admin"] = admin;
    (*config)["console"] = console;
    args_to_argv();
}

void ExecutableItem::args_to_argv() {
#ifndef _WIN32
    argv.clear();
    strargv.clear();

    string current;
    bool in_sig_quote = false;
    bool in_double_quote = false;
    bool backslash = false;

    argv.push_back(command.data());
    for (size_t i = 0; i < args.length(); ++i) {
        const char c = args[i];

        if (backslash) {
            current += c;
            backslash = false;
        } else if (c == '\\') {
            backslash = true;
        } else if (c == '"' && !in_sig_quote && !backslash) {
            in_double_quote = !in_double_quote;
        } else if (c == '\'' && !in_double_quote && !backslash) {
            in_sig_quote = !in_sig_quote;
        } else if (std::isspace(c) && !in_sig_quote && !in_double_quote && !backslash) {
            if (!current.empty()) {
                this->strargv.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        this->strargv.push_back(current);
    }

    for (auto& arg : strargv) {
        argv.push_back(arg.data());
    }
#endif
}

void ExecutableItem::execute() {
#ifdef _WIN32
    // TODO: Add window show options (show, minimise, etc.)
    // TODO: Windows store apps
    if (admin)
        ShellExecuteA(nullptr, "runas", command.c_str(), args.c_str(), cwd.empty() ? nullptr : cwd.c_str(), SW_SHOW);
    else
        ShellExecuteA(nullptr, "open", command.c_str(), args.c_str(), cwd.empty() ? nullptr : cwd.c_str(), SW_SHOW);
#else
    // TODO: XDG Application menu support
    pid_t pid = fork();
    if (pid) {
        return;
    }

    setsid();

    int fd = open("/dev/null", O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > 2)
        close(fd);

    vector<char*> _argv;
    _argv.reserve(argv.size() + 4);
    /*
    if (this->console) {
        vector<string> terminals = {
            std::getenv("TERMINAL") ? std::getenv("TERMINAL") : "",
            "x-terminal-emulator", "mate-terminal", "gnome-terminal", "terminator",
            "xfce4-terminal", "urxvt", "rxvt", "termit", "Eterm", "aterm",
            "uxterm", "xterm", "roxterm", "termite", "lxterminal", "terminology",
            "st", "qterminal", "lilyterm", "tilix", "terminix", "konsole",
            "kitty", "guake", "tilda", "alacritty", "hyper", "wezterm", "rio"
        };

        for (const auto& terminal : terminals) {
            if (terminal.empty())
                continue;

            if (!system(string("command -v " + terminal + " > /dev/null 2>&1").c_str())) {
                _argv.push_back(vector<char*>::value_type(terminal.data()));
                _argv.push_back("-e");
                break;
            }
        }
    }
    // TODO: SUDO
    // we don't need administrative consoles
    if (this->admin) {
        if (command == this->command)
            command = "xdg-su";
        _argv.push_back("xdg-su");
    }
    */
    _argv.insert(_argv.end(), argv.begin(), argv.end());
    _argv.push_back(nullptr);

    if (!cwd.empty())
        chdir(cwd.c_str());
    execvp(_argv[0], _argv.data());

    _exit(-1);
#endif
}
}
