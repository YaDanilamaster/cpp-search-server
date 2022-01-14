
/* Подставьте вашу реализацию класса SearchServer сюда */

/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/

template <class V>
ostream& operator << (ostream& os, const vector<V>& s) {
	os << "[";
	bool first = true;
	for (const auto& x : s) {
		if (!first) {
			os << ", ";
		}
		first = false;
		os << x;
	}
	return os << "]";
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
	const string& func, unsigned line, const string& hint) {
	if (t != u) {
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		cerr << t << " != "s << u << "."s;
		if (!hint.empty()) {
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
	const string& hint) {
	if (!value) {
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) {
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	// Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
	// находит нужный документ
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	// Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
	// возвращает пустой результат
	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT(server.FindTopDocuments("in"s).empty());
	}
}

/*
Разместите код остальных тестов здесь
*/

// Проверяем работу с минус словами
void TestExcludeMinusWordsFromAddedDocumentContent() {
	{
		SearchServer server;
		server.AddDocument(0, "next let set now how row coll", DocumentStatus::ACTUAL, { 1, 2, 3, 4 });
		server.AddDocument(1, "lot not root bool cat set get", DocumentStatus::ACTUAL, { 2, 3, 4, 5 });
		ASSERT_EQUAL(server.FindTopDocuments("set -lot")[0].id, 0);
	}
	{
		SearchServer server;
		server.AddDocument(0, "next let set now how row coll", DocumentStatus::ACTUAL, { 1, 2, 3, 4 });
		server.AddDocument(1, "lot not root bool cat set get", DocumentStatus::ACTUAL, { 2, 3, 4, 5 });
		server.AddDocument(15, "see room in best dog of bag set", DocumentStatus::ACTUAL, { 2, 3, 4, 5 });
		vector<Document> result = server.FindTopDocuments("set -lot");
		ASSERT_EQUAL(result[0].id, 15);
		ASSERT_EQUAL(result.size(), 2U);
	}
}

// Проверяем матчинг документа
void TestMatchDocument() {
	SearchServer server;
	server.AddDocument(15, "see room in best dog of bag set", DocumentStatus::ACTUAL, { 2, 3, 4, 5 });
	{
		const auto [words, status] = server.MatchDocument("room best bag set", 15);
		vector<string> match_words = { "bag", "best", "room", "set" };
		ASSERT_EQUAL_HINT(words, match_words, "MatchDocument wrong result");
	}
	{
		const auto [words, status] = server.MatchDocument("room best bag -set", 15);
		ASSERT_HINT(words.empty(), "MatchDocument wrong result");
	}
}

// Проверяем вычисление рейтинга документов
void TestRaitingDocuments() {
	SearchServer server;
	server.AddDocument(0, "next let set now how next row coll", DocumentStatus::ACTUAL, { 1, 2, 3, 4 });
	server.AddDocument(1, "lot not root bool cat set get next", DocumentStatus::ACTUAL, { 1, 2, 3, 4, 5 });
	server.AddDocument(15, "see room in best dog of bag bag bag bag bag bag", DocumentStatus::ACTUAL, { 1, 2, 5, 6, 7, 8 });
	vector<Document> result = server.FindTopDocuments("next set bag");
	ASSERT_EQUAL(result.size(), 3U);
	ASSERT_EQUAL(result[0].id, 15);
	ASSERT_EQUAL_HINT(result[0].rating, (1 + 2 + 5 + 6 + 7 + 8) / 6, "Wrong rating");
	ASSERT_EQUAL(result[1].id, 0);
	ASSERT_EQUAL_HINT(result[1].rating, (1 + 2 + 3 + 4) / 4, "Wrong rating");
}

// Проверяем вычисление релевантности документов и сортировку по релевантности
void TestRelevanceAndSortingDocuments() {
	SearchServer server;
	server.AddDocument(0, "a b c", DocumentStatus::ACTUAL, { 1, 2, 3, 4 });
	server.AddDocument(1, "b c b", DocumentStatus::ACTUAL, { 1, 2, 3, 4, 5 });
	server.AddDocument(2, "d e f", DocumentStatus::ACTUAL, { 1, 2, 5, 6, 7, 8 });
	server.AddDocument(3, "q y b", DocumentStatus::ACTUAL, { 3, 5, 4 });

	vector<Document> result = server.FindTopDocuments("b");

	ASSERT_EQUAL_HINT(result[0].relevance, log(4.0 / 3.0) * (2.0 / 3.0), "Wrong relevance");
	ASSERT_EQUAL_HINT(result[0].id, 1, "Wrong sorting to relevance");

	ASSERT_EQUAL_HINT(result[1].relevance, log(4.0 / 3.0) * (1.0 / 3.0), "Wrong relevance");
	ASSERT_EQUAL_HINT(result[1].id, 3, "Wrong sorting to relevance");

	ASSERT_EQUAL_HINT(result[2].relevance, log(4.0 / 3.0) * (1.0 / 3.0), "Wrong relevance");
	ASSERT_EQUAL_HINT(result[2].id, 0, "Wrong sorting to relevance");
}

// Проверяем максимальное количество выдаваемых документов
void TestMaxResultDocumentCount() {
	SearchServer server;
	server.AddDocument(0, "a b c", DocumentStatus::ACTUAL, { 1, 2 });
	server.AddDocument(1, "e a g", DocumentStatus::ACTUAL, { 1, 2 });
	server.AddDocument(2, "r t a", DocumentStatus::ACTUAL, { 1, 2 });
	server.AddDocument(3, "g a j", DocumentStatus::ACTUAL, { 1, 2 });
	server.AddDocument(4, "a d x", DocumentStatus::ACTUAL, { 1, 2 });
	server.AddDocument(5, "a n m", DocumentStatus::ACTUAL, { 1, 2 });
	server.AddDocument(6, "a p y", DocumentStatus::ACTUAL, { 1, 2 });
	server.AddDocument(7, "a l u", DocumentStatus::ACTUAL, { 1, 2 });
	ASSERT_EQUAL_HINT(server.FindTopDocuments("a").size(), 5U, "Wrong max count documents");
}

void TestDocumentSearchByStatus() {
	SearchServer server;
	server.AddDocument(0, "a", DocumentStatus::BANNED, { });
	server.AddDocument(1, "a", DocumentStatus::REMOVED, { });
	server.AddDocument(15, "a", DocumentStatus::ACTUAL, { });
	server.AddDocument(2, "a", DocumentStatus::BANNED, { });
	server.AddDocument(3, "a", DocumentStatus::REMOVED, { });
	server.AddDocument(6, "a", DocumentStatus::ACTUAL, { });

	ASSERT_EQUAL_HINT(server.FindTopDocuments("a", DocumentStatus::ACTUAL).size(), 2U, "ACTUAL status wrong");
	ASSERT_EQUAL_HINT(server.FindTopDocuments("a", DocumentStatus::REMOVED).size(), 2U, "REMOVED status wrong");
	ASSERT_EQUAL_HINT(server.FindTopDocuments("a", DocumentStatus::BANNED).size(), 2U, "BANNED status wrong");
}

void TestDocumentSearchByPredicate() {
	SearchServer server;
	server.AddDocument(0, "next let set now how next row coll", DocumentStatus::BANNED, { 1, 2, 3, 4 });
	server.AddDocument(1, "lot not root bool cat set get next", DocumentStatus::REMOVED, { 1, 2, 3, 4, 5 });
	server.AddDocument(15, "see room in best dog of bag bag bag bag bag bag", DocumentStatus::ACTUAL, { 1, 2, 5, 6, 7, 8 });
	server.AddDocument(2, "next let set now how next row coll", DocumentStatus::BANNED, { 1, 2, 3, 4 });
	server.AddDocument(3, "lot not root bool cat set get next", DocumentStatus::REMOVED, { 1, 2, 3, 4, 5 });
	server.AddDocument(6, "see room in best dog of bag bag bag bag bag bag", DocumentStatus::ACTUAL, { 1, 2, 5, 6, 7, 8 });

	// Проверяем работу предиката на четных и не четных индексах документов
	ASSERT_EQUAL(
		server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
			return document_id % 2 == 0; }).size(), 3U);
	ASSERT_EQUAL(
		server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
			return document_id % 2 == 0; })[0].id, 6);
	ASSERT_EQUAL(
		server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
			return document_id % 2 == 1; }).size(), 3U);
	ASSERT_EQUAL(
		server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
			return document_id % 2 == 1; })[0].id, 15);

	// Проверяем работу предиката на четных и не четных рейтингах документов
	ASSERT_EQUAL(
		server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
			return rating % 2 == 0; }).size(), 4U);
	ASSERT_EQUAL(
		server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
			return rating % 2 == 0; })[2].id, 0);
	ASSERT_EQUAL(
		server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
			return rating % 2 == 1; }).size(), 2U);
	ASSERT_EQUAL(
		server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
			return rating % 2 == 1; })[1].id, 3);
}

template <typename Func>
void RunTestImpl(Func& function, const string& p_str) {
	function();
	cerr << p_str << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)  // напишите недостающий код

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	// Не забудьте вызывать остальные тесты здесь
	RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
	RUN_TEST(TestMatchDocument);
	RUN_TEST(TestRaitingDocuments);
	RUN_TEST(TestRelevanceAndSortingDocuments);
	RUN_TEST(TestMaxResultDocumentCount);
	RUN_TEST(TestDocumentSearchByStatus);
	RUN_TEST(TestDocumentSearchByPredicate);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
	TestSearchServer();
	// Если вы видите эту строку, значит все тесты прошли успешно
	cout << "Search server testing finished"s << endl;
	return 0;
}
