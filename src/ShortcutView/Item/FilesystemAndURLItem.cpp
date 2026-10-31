#include "ShortcutView/ShortcutView.hpp"
#include "ShortcutView/Item/Item.hpp"
#include "ShortcutView/Item/FilesystemAndURLItem.hpp"
#include "ShortcutView/Item/FolderItem.hpp"

#include "Utilities/FileDialog.hpp"

#include <imgui/imgui_stdlib.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

namespace ShortcutView::Item {
FilesystemAndURLItem::FilesystemAndURLItem(json* config, ShortcutView& parentView,
                                            FolderItem* parent) : Item(config, parentView, parent) {
    if (config) {
        if (config->contains("path") && (*config)["path"].is_string()) {
            path = (*config)["path"].get<string>();
            m_path = path;
        }
    }
}

void FilesystemAndURLItem::drawProperties() {
    ImGui::BeginDisabled();
    ImGui::TextWrapped("(i) Please do not use environment variables. Write your full user path (if applicable), not ~/");
    ImGui::EndDisabled();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Path: ");
    ImGui::SameLine();
    ImGui::InputText("##cmd", &m_path);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth()-ImGui::GetCursorPosX()-12);
    if (ImGui::BeginCombo("##browse", "Browse")) {
        if (ImGui::Selectable("File")) {
            string res = ExtenDeck::Utilities::nfd_open_file();
            if (!res.empty())
                m_path = res;
        }
        if (ImGui::Selectable("Directory")) {
            string res = ExtenDeck::Utilities::nfd_open_dir();
            if (!res.empty())
                m_path = res;
        }
        ImGui::EndCombo();
    }
#ifdef _WIN32
    ImGui::BeginDisabled();
    ImGui::TextWrapped("(i) If you cannot see some shortcuts, try looking at the public desktop");
    ImGui::EndDisabled();
#endif
}

void FilesystemAndURLItem::propertiesCancel() {
    m_path = path;
}

void FilesystemAndURLItem::properties_apply() {
    path = m_path;
    (*config)["path"] = path;
}

bool FilesystemAndURLItem::isDisabled() {
    return m_path.empty();
}

void FilesystemAndURLItem::execute() {
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOW);
#else
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

    execlp("xdg-open", "xdg-open", path.c_str(), static_cast<char*>(nullptr));

    _exit(0);
#endif
}
}
