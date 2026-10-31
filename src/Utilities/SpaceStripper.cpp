#include "Utilities/SpaceStripper.hpp"

namespace ExtenDeck::Utilities {
string strip_spaces(string str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(str[start]))
        ++start;

    if (start == str.size())
        return "";

    size_t end = str.size() - 1;
    while (end > start && std::isspace(str[end]))
        --end;

    return str.substr(start, end - start + 1);
}
}
