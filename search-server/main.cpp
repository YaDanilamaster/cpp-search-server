
/* Подставьте вашу реализацию класса SearchServer сюда */

/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/
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

void TestMinusWords() {
    // Проверяем работу с минус словами
    SearchServer server;
    server.AddDocument(0, "next let set now how row coll", DocumentStatus::ACTUAL, { 1,2,3,4 });
    server.AddDocument(1, "lot not root bool cat set get", DocumentStatus::ACTUAL, { 2,3,4,5 });
    vector<Document> result = server.FindTopDocuments("set -lot");
    ASSERT_EQUAL(result[0].id, 0);
    server.AddDocument(15, "see room in best dog of bag set", DocumentStatus::ACTUAL, { 2,3,4,5 });
    vector<Document> result2 = server.FindTopDocuments("set -lot");
    ASSERT_EQUAL(result2[0].id, 15);
    ASSERT_EQUAL(result2.size(), 2);

    // Проверяем матчинг документа
    tuple<vector<string>, DocumentStatus> result3;
    result3 = server.MatchDocument("room best bag set", 15);
    ASSERT_EQUAL_HINT(get<0>(result3).size(), 4, "MatchDocument wrong result");
    result3 = server.MatchDocument("room best bag -set", 15);
    ASSERT_EQUAL_HINT(get<0>(result3).size(), 0, "MatchDocument wrong result");
}

void TestRaitingsScore() {
    // Проверяем работу системы рейтинга и релевантности
    SearchServer server;
    server.AddDocument(0, "next let set now how next row coll", DocumentStatus::ACTUAL, { 1,2,3,4 });
    server.AddDocument(1, "lot not root bool cat set get next", DocumentStatus::ACTUAL, { 1,2,3,4,5 });
    server.AddDocument(15, "see room in best dog of bag bag bag bag bag bag", DocumentStatus::ACTUAL, { 1,2,5,6,7,8 });
    vector<Document> result = server.FindTopDocuments("next set bag");
    ASSERT_EQUAL(result.size(), 3);
    ASSERT_EQUAL(result[0].id, 15);
    ASSERT_EQUAL_HINT(result[0].rating, 4, "Wrong rating");
    ASSERT_EQUAL(result[1].id, 0);
    ASSERT_EQUAL_HINT(result[1].rating, 2, "Wrong rating");
    ASSERT_EQUAL_HINT(result[0].relevance, 0.54930614433405478, "Wrong relevance");
    ASSERT_EQUAL_HINT(result[2].relevance, 0.10136627702704110, "Wrong relevance");
}

void TestStatus() {
    SearchServer server;
    server.AddDocument(0, "next let set now how next row coll", DocumentStatus::BANNED, { 1,2,3,4 });
    server.AddDocument(1, "lot not root bool cat set get next", DocumentStatus::REMOVED, { 1,2,3,4,5 });
    server.AddDocument(15, "see room in best dog of bag bag bag bag bag bag", DocumentStatus::ACTUAL, { 1,2,5,6,7,8 });
    server.AddDocument(2, "next let set now how next row coll", DocumentStatus::BANNED, { 1,2,3,4 });
    server.AddDocument(3, "lot not root bool cat set get next", DocumentStatus::REMOVED, { 1,2,3,4,5 });
    server.AddDocument(6, "see room in best dog of bag bag bag bag bag bag", DocumentStatus::ACTUAL, { 1,2,5,6,7,8 });

    vector<Document> result1 = server.FindTopDocuments("next set bag", DocumentStatus::ACTUAL);
    vector<Document> result2 = server.FindTopDocuments("next set bag", DocumentStatus::REMOVED);
    vector<Document> result3 = server.FindTopDocuments("next set bag", DocumentStatus::BANNED);
    ASSERT_EQUAL_HINT(result1.size(), 2, "ACTUAL status wrong");
    ASSERT_EQUAL_HINT(result2.size(), 2, "REMOVED status wrong");
    ASSERT_EQUAL_HINT(result3.size(), 2, "BANNED status wrong");
}

void TestPredicate() {
    SearchServer server;
    server.AddDocument(0, "next let set now how next row coll", DocumentStatus::BANNED, { 1,2,3,4 });
    server.AddDocument(1, "lot not root bool cat set get next", DocumentStatus::REMOVED, { 1,2,3,4,5 });
    server.AddDocument(15, "see room in best dog of bag bag bag bag bag bag", DocumentStatus::ACTUAL, { 1,2,5,6,7,8 });
    server.AddDocument(2, "next let set now how next row coll", DocumentStatus::BANNED, { 1,2,3,4 });
    server.AddDocument(3, "lot not root bool cat set get next", DocumentStatus::REMOVED, { 1,2,3,4,5 });
    server.AddDocument(6, "see room in best dog of bag bag bag bag bag bag", DocumentStatus::ACTUAL, { 1,2,5,6,7,8 });
    //predicate(document_id, status, rating)

    vector<Document> result1 = server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
        return document_id % 2 == 0;
        });
    vector<Document> result2 = server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
        return document_id % 2 == 1;
        });
    vector<Document> result3 = server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
        return rating % 2 == 0;
        });
    vector<Document> result4 = server.FindTopDocuments("next set bag", [](const int document_id, const DocumentStatus status, const int rating) {
        return rating % 2 == 1;
        });

    ASSERT_EQUAL(result1.size(), 3);
    ASSERT_EQUAL(result1[0].id, 6);
    ASSERT_EQUAL(result2.size(), 3);
    ASSERT_EQUAL(result2[0].id, 15);
    ASSERT_EQUAL(result3.size(), 4);
    ASSERT_EQUAL(result3[2].id, 0);
    ASSERT_EQUAL(result4.size(), 2);
    ASSERT_EQUAL(result4[1].id, 3);
}

template <typename Func>
void RunTestImpl(Func& function, const string& p_str) {
    function();
    cerr << p_str << " OK" << endl;
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestRaitingsScore);
    RUN_TEST(TestStatus);
    RUN_TEST(TestPredicate);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}