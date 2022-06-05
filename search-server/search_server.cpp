#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const std::string& stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::string_view stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(
	int document_id,
	const std::string_view document,
	DocumentStatus status,
	const vector<int>& ratings)
{
	if ((document_id < 0) || (documents_.count(document_id) > 0)) {
		throw invalid_argument("Invalid document_id"s);
	}
	const auto [doc_it, _] = documents_words_.insert(string{ document });

	const vector<string_view> words = SplitIntoWordsNoStop(*doc_it);
	const double inv_word_count = 1.0 / words.size();
	const int raiting = ComputeAverageRating(ratings);
	DocumentData word_frequencies{ raiting, status, {} };

	for (const std::string_view& word : words) {
		word_to_document_freqs_[word][document_id] += inv_word_count;
		word_frequencies.words[word] += inv_word_count;
	}
	documents_.emplace(document_id, move(word_frequencies));
	documents_id_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(
	const std::string_view raw_query,
	DocumentStatus status) const
{
	return FindTopDocuments(raw_query,
		[status](int document_id, DocumentStatus document_status, int rating)
		{
			return document_status == status;
		});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const
{
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(
	const std::execution::sequenced_policy& policy,
	const std::string_view raw_query,
	DocumentStatus status) const
{
	return FindTopDocuments(raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(
	const std::execution::sequenced_policy& policy,
	const std::string_view raw_query) const
{
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(
	const std::execution::parallel_policy& policy,
	const std::string_view raw_query,
	DocumentStatus status) const
{
	return FindTopDocuments(policy, raw_query,
		[status](int document_id, DocumentStatus document_status, int rating)
		{
			return document_status == status;
		});
}

std::vector<Document> SearchServer::FindTopDocuments(
	const std::execution::parallel_policy& policy, 
	const std::string_view raw_query) const
{
	return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const 
{
	return documents_.size();
}

tuple<vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
	const std::string_view& raw_query, 
	int document_id) const 
{
	const auto query = ParseQuery(raw_query);
	vector<string_view> matched_words;

	for (const std::string_view word : query.minus_words) {
		auto it = word_to_document_freqs_.find(word);
		if (it == word_to_document_freqs_.end()) {
			continue;
		}
		if (it->second.count(document_id)) {
			return { matched_words, documents_.at(document_id).status };
		}
	}

	for (const string_view word : query.plus_words) {
		auto it = word_to_document_freqs_.find(word);
		if (it == word_to_document_freqs_.end()) {
			continue;
		}
		if (it->second.count(document_id)) {
			matched_words.push_back(word);
		}
	}

	return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
	const std::execution::sequenced_policy& policy,
	const std::string_view& raw_query,
	int document_id) const
{
	return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<string_view>, DocumentStatus> SearchServer::MatchDocument(
	const std::execution::parallel_policy& policy,
	const std::string_view& raw_query,
	int document_id) const
{
	const auto query = ParseQuery(raw_query);
	vector<string_view> matched_words(query.plus_words.size());

	for (const std::string_view word : query.minus_words) {
		auto it = word_to_document_freqs_.find(word);
		if (it == word_to_document_freqs_.end()) {
			continue;
		}
		if (it->second.count(document_id)) {
			matched_words.clear();
			return { matched_words, documents_.at(document_id).status };
		}
	}

	atomic_size_t count = 0;
	for_each(policy,
		query.plus_words.begin(), query.plus_words.end(),
		[this, document_id, &count, &matched_words](const string_view word) {
			auto it = documents_.find(document_id);
			if (it != documents_.end()) {
				if (it->second.words.count(word)) {
					matched_words[++count] = word;
				}
			}
		}
	);

	matched_words.resize(count);
	return { matched_words, documents_.at(document_id).status };
}


bool SearchServer::IsStopWord(const string_view word) const 
{
	return stop_words_.count(string{ word }) == 1;
}

bool SearchServer::IsValidWord(const std::string_view word) 
{
	return none_of(word.begin(), word.end(), [](const char c) {
		return c >= '\0' && c < ' ';
		});
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const 
{
	vector<string_view> words;
	for (string_view word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw invalid_argument("Word "s + string{ text } + " is invalid"s);
		}
		if (!IsStopWord(word)) {
			words.push_back(move(word));
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) 
{
	if (ratings.empty()) {
		return 0;
	}
	int rating_sum = 0;
	for (const int rating : ratings) {
		rating_sum += rating;
	}
	return rating_sum / static_cast<int>(ratings.size());
}

inline SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const 
{
	if (text.empty()) {
		throw invalid_argument("Query word is empty"s);
	}
	string_view word;
	bool is_minus = false;
	if (text[0] == '-') {
		is_minus = true;
		word = text.substr(1);
	}
	else {
		word = text;
	}
	if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
		throw invalid_argument("Query word "s + string{ text } + " is invalid");
	}
	return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const 
{
	Query result;
	vector<string_view> words = SplitIntoWords(text);

	for_each(
		words.begin(), words.end(),
		[&result, this](const string_view word) {
			const auto query_word = ParseQueryWord(word);
			if (!query_word.is_stop) {
				if (query_word.is_minus) {
					result.minus_words.push_back(query_word.data);
				}
				else {
					result.plus_words.push_back(query_word.data);
				}
			}
		}
	);

	sort(execution::par, result.minus_words.begin(), result.minus_words.end());
	sort(execution::par, result.plus_words.begin(), result.plus_words.end());

	auto end_it_m = unique(execution::par, result.minus_words.begin(), result.minus_words.end());
	auto end_it_p = unique(execution::par, result.plus_words.begin(), result.plus_words.end());

	result.minus_words.resize(end_it_m - result.minus_words.begin());
	result.plus_words.resize(end_it_p - result.plus_words.begin());

	return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const 
{
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const 
{
	auto result = documents_.find(document_id);
	if (result != documents_.end())
	{
		return result->second.words;
	}
	static const map<string_view, double> null_map;
	return null_map;
}

void SearchServer::RemoveDocument(int document_id) 
{
	auto itemIt = documents_.find(document_id);
	for (auto& [word, _] : itemIt->second.words) {
		word_to_document_freqs_[word].erase(document_id);
	}
	documents_.erase(itemIt);
	documents_id_.erase(find(documents_id_.begin(), documents_id_.end(), document_id));
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) 
{
	RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) 
{
	auto itemIt = documents_.find(document_id);

	vector<string_view> words(itemIt->second.words.size());

	transform(execution::par, itemIt->second.words.begin(), itemIt->second.words.end(), words.begin(),
		[](pair<const string_view, double>& word) {
			return word.first;
		});

	for_each(execution::par, words.begin(), words.end(),
		[this, document_id](const string_view word) {
			word_to_document_freqs_.find(word)->second.erase(document_id);
		});

	documents_.erase(itemIt);
	documents_id_.erase(find(documents_id_.begin(), documents_id_.end(), document_id));
}

vector<int>::iterator SearchServer::begin()
{
	return documents_id_.begin();
}

vector<int>::iterator SearchServer::end()
{
	return documents_id_.end();
}

vector<int>::const_iterator SearchServer::begin() const
{
	return documents_id_.begin();
}

vector<int>::const_iterator SearchServer::end() const
{
	return documents_id_.end();
}