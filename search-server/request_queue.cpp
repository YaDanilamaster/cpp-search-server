#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server)
    , curent_time_(0)
    , no_result_count(0) {
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    vector<Document> result;
    result = search_server_.FindTopDocuments(raw_query, status);
    RequestToQueue(result);
    return result;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    vector<Document> result;
    result = search_server_.FindTopDocuments(raw_query);
    RequestToQueue(result);
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_count;
}

void RequestQueue::RequestToQueue(vector<Document>& result) {
    ++curent_time_;

    if (!requests_.empty()) {
        while (curent_time_ - requests_.front().time >= min_in_day_) {
            if (requests_.front().no_result) {
                --no_result_count;
            }
            requests_.pop_front();
        }
    }

    requests_.push_back({ curent_time_, result.empty() });
    no_result_count += result.empty();
}
