#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <set>

std::vector<std::string_view> SplitIntoWords(const std::string_view& text);

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

std::set<std::string> MakeUniqueNonEmptyStrings(const std::vector<std::string_view>& strings);