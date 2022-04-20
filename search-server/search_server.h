#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <map>

#include <iterator>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <functional>

#include <execution>
#include <atomic>

#include <cmath>

#include "document.h"
#include "concurrent_map.h"
#include "string_processing.h"


const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double MIN_RELEVANCE_DIFFERENCE = 1e-6;
const size_t CONCURENT_MAP_BUCKET_COUNT = 100;

class SearchServer {
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);
	explicit SearchServer(const std::string& stop_words_text);
	explicit SearchServer(const std::string_view stop_words_text);

	void AddDocument(
		int document_id,
		const std::string_view document,
		DocumentStatus status,
		const std::vector<int>& ratings);

	std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
	std::vector<Document> FindTopDocuments(
		const std::string_view raw_query,
		DocumentStatus status) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(
		const std::string_view raw_query,
		DocumentPredicate document_predicate) const;

	std::vector<Document> FindTopDocuments(
		const std::execution::sequenced_policy& policy,
		const std::string_view raw_query) const;

	std::vector<Document> FindTopDocuments(
		const std::execution::sequenced_policy& policy,
		const std::string_view raw_query,
		DocumentStatus status) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(
		const std::execution::sequenced_policy& policy,
		const std::string_view raw_query,
		DocumentPredicate document_predicate) const;

	std::vector<Document> FindTopDocuments(
		const std::execution::parallel_policy& policy,
		const std::string_view raw_query) const;

	std::vector<Document> FindTopDocuments(
		const std::execution::parallel_policy& policy,
		const std::string_view raw_query,
		DocumentStatus status) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(
		const std::execution::parallel_policy& policy,
		const std::string_view raw_query,
		DocumentPredicate document_predicate) const;

	int GetDocumentCount() const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
		const std::string_view& raw_query,
		int document_id) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
		const std::execution::sequenced_policy& policy,
		const std::string_view& raw_query,
		int document_id
	) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
		const std::execution::parallel_policy& policy,
		const std::string_view& raw_query,
		int document_id
	) const;

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

	void RemoveDocument(int document_id);
	void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id);
	void RemoveDocument(const std::execution::parallel_policy& policy, int document_id);

	std::vector<int>::iterator begin();
	std::vector<int>::const_iterator begin() const;

	std::vector<int>::iterator end();
	std::vector<int>::const_iterator end() const;

private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
		std::map<std::string_view, double> words;
	};
	const std::set<std::string> stop_words_;
	std::set<std::string, std::less<>> documents_words_;

	std::map<std::string_view, std::map<int, double>, std::less<>> word_to_document_freqs_;
	std::map<int, DocumentData> documents_;
	std::vector<int> documents_id_;

	inline bool IsStopWord(const std::string_view word) const;

	static bool IsValidWord(const std::string_view word);

	std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	struct QueryWord {
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	inline QueryWord ParseQueryWord(const std::string_view text) const;

	struct Query {
		std::vector<std::string_view> plus_words;
		std::vector<std::string_view> minus_words;
	};

	Query ParseQuery(const std::string_view text) const;

	double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(
		const Query& query,
		DocumentPredicate document_predicate) const;

	template<typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(
		std::execution::parallel_policy policy,
		const Query& query,
		DocumentPredicate document_predicate) const;
};

template<typename StringContainer>
inline SearchServer::SearchServer(const StringContainer& stop_words)
	: stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
	if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
		throw std::invalid_argument("Some of stop words are invalid");
	}
}

template<typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const
{
	const auto query = ParseQuery(raw_query);

	auto matched_documents = FindAllDocuments(query, document_predicate);

	std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
		if (std::abs(lhs.relevance - rhs.relevance) < MIN_RELEVANCE_DIFFERENCE) {
			return lhs.rating > rhs.rating;
		}
		else {
			return lhs.relevance > rhs.relevance;
		}
		});
	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}

	return matched_documents;
}

template<typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindTopDocuments(
	const std::execution::sequenced_policy& policy,
	const std::string_view raw_query,
	DocumentPredicate document_predicate) const
{
	return FindTopDocuments(raw_query, document_predicate);
}

template<typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindTopDocuments(
	const std::execution::parallel_policy& policy,
	const std::string_view raw_query,
	DocumentPredicate document_predicate) const
{
	const auto query = ParseQuery(raw_query);

	auto matched_documents = FindAllDocuments(policy, query, document_predicate);

	std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
		if (std::abs(lhs.relevance - rhs.relevance) < MIN_RELEVANCE_DIFFERENCE) {
			return lhs.rating > rhs.rating;
		}
		else {
			return lhs.relevance > rhs.relevance;
		}
		});
	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}

	return matched_documents;
}

template<typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const
{
	std::map<int, double> document_to_relevance;
	for (const std::string_view word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(std::string{ word });
		for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
			const auto& document_data = documents_.at(document_id);
			if (document_predicate(document_id, document_data.status, document_data.rating)) {
				document_to_relevance[document_id] += term_freq * inverse_document_freq;
			}
		}
	}

	for (const std::string_view word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
			document_to_relevance.erase(document_id);
		}
	}

	std::vector<Document> matched_documents;

	for (const auto [document_id, relevance] : document_to_relevance) {
		matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
	}
	return matched_documents;
}

template<typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindAllDocuments(
	std::execution::parallel_policy policy,
	const Query& query,
	DocumentPredicate document_predicate) const
{
	ConcurrentMap<int, double> document_to_relevance(CONCURENT_MAP_BUCKET_COUNT);

	for_each(policy,
		query.plus_words.begin(), query.plus_words.end(),
		[this, &document_to_relevance, &document_predicate](const std::string_view word) {
			auto it = word_to_document_freqs_.find(word);
			if (it == word_to_document_freqs_.end()) {
				return;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto [document_id, term_freq] : it->second) {
				const auto& document_data = documents_.at(document_id);
				if (document_predicate(document_id, document_data.status, document_data.rating)) {
					document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
				}
			}
		});

	for (const std::string_view word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
			document_to_relevance.Erase(document_id);
		}
	}

	std::vector<Document> matched_documents(document_to_relevance.Size());
	std::atomic_size_t index = 0;
	for_each(policy,
		document_to_relevance.begin(), document_to_relevance.end(),
		[this, &index, &matched_documents](const auto& documents) {
			for (auto [document_id, relevance] : documents) {
				int raiting = documents_.at(document_id).rating;
				matched_documents[index++] = { document_id, relevance, raiting };
			}
		});

	return matched_documents;
}