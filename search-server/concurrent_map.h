#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        Value& ref_to_value;
        std::mutex* mtx;

        ~Access();
    };

    explicit ConcurrentMap(size_t bucket_count);

    Access operator[](const Key& key)
    {
        const int index = key % bucket_count_;
        bucket_mute_[index].lock();
        return { concurr_map_[index][key], &bucket_mute_[index] };
    }

    void Erase(const Key& key);

    std::map<Key, Value> BuildOrdinaryMap();

    size_t Size() const;

    typename std::vector<std::map<Key, Value>>::const_iterator begin() const;

    typename std::vector<std::map<Key, Value>>::const_iterator end() const;

private:
    const size_t bucket_count_;
    std::vector<std::map<Key, Value>> concurr_map_;
    std::vector<std::mutex> bucket_mute_;
};

template<typename Key, typename Value>
inline ConcurrentMap<Key, Value>::ConcurrentMap(size_t bucket_count) :
    bucket_count_(bucket_count),
    concurr_map_(bucket_count),
    bucket_mute_(bucket_count)
{
}

template<typename Key, typename Value>
inline void ConcurrentMap<Key, Value>::Erase(const Key& key)
{
    const int index = key % bucket_count_;
    bucket_mute_[index].lock();
    concurr_map_[index].erase(key);
    bucket_mute_[index].unlock();
}

template<typename Key, typename Value>
inline std::map<Key, Value> ConcurrentMap<Key, Value>::BuildOrdinaryMap()
{
    std::map<Key, Value> result;
    for (size_t i = 0; i < bucket_count_; i++) {
        std::lock_guard<std::mutex> lock_bucket = std::lock_guard(bucket_mute_[i]);
        result.insert(concurr_map_[i].begin(), concurr_map_[i].end());
    }
    return result;
}

template<typename Key, typename Value>
inline size_t ConcurrentMap<Key, Value>::Size() const
{
    size_t size = 0;
    for (auto item : concurr_map_) {
        size += item.size();
    }
    return size;
}

template<typename Key, typename Value>
inline typename std::vector<std::map<Key, Value>>::const_iterator ConcurrentMap<Key, Value>::begin() const
{
    return concurr_map_.begin();
}

template<typename Key, typename Value>
inline typename std::vector<std::map<Key, Value>>::const_iterator ConcurrentMap<Key, Value>::end() const
{
    return concurr_map_.end();
}

template<typename Key, typename Value>
inline ConcurrentMap<Key, Value>::Access::~Access()
{
    (*mtx).unlock();
}
