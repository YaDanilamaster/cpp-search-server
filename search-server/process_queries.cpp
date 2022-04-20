#include "process_queries.h"

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries)
{
	vector<vector<Document>> result(queries.size());
	transform(execution::par,
		queries.begin(), queries.end(),
		result.begin(),
		[&search_server](const string& q) {
			return search_server.FindTopDocuments(q);
		}
	);
	return result;
}

list<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries)
{
	list<Document> result;
	vector<vector<Document>> documents_query(ProcessQueries(search_server, queries));
	for (vector<Document>& documents : documents_query) {
		for (Document doc : documents) {
			result.push_back(move(doc));
		}
	}
	return result;
}