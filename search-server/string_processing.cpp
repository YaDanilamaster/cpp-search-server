#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(const string_view& text) {
	vector<string_view> words;
	words.reserve(500);

	size_t pos0 = 0;
	size_t pos1 = 0;
	for (const char c : text) {
		if (c != ' ') {
			++pos1;
		}
		else {
			if (pos1 != 0) {
				words.push_back(text.substr(pos0, pos1));
				pos0 += ++pos1;
				pos1 = 0;
			}
		}
	}
	if (pos1 != 0) {
		words.push_back(text.substr(pos0, pos1));
	}

	return words;
}

std::set<std::string> MakeUniqueNonEmptyStrings(const std::vector<std::string_view>& strings)
{
	std::set<std::string> non_empty_strings;
	for (const std::string_view& str : strings) {
		if (!str.empty()) {
			non_empty_strings.insert(string{ str });
		}
	}
	return non_empty_strings;
}