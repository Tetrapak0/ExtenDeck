#pragma once

#include "Item.hpp"

namespace ShortcutView::Item {
class ExecutableItem : public Item {
public:
    std::string command;
    std::string args;
    std::string cwd;
    bool admin = false;
    bool console = false;
#ifndef _WIN32
    vector<std::string> strargv;
    vector<char*> argv;
#endif
    void args_to_argv();

    explicit ExecutableItem(json* config, ShortcutView& parentView, FolderItem* parent);

    std::string_view getTypename() override {return "Executable";}
    void drawProperties() override;
    void propertiesCancel() override;
    void properties_apply() override;
    bool isDisabled() override;
    void execute() override;
protected:
    std::string m_command = command;
    std::string m_args = args;
    std::string m_cwd = cwd;
    bool m_admin = admin;
    bool m_console = console;
};
}
