// cache.cpp
#pragma once
#include <unordered_map>
#include <list>
#include <mutex>
#include <string>

class Cache {
private:
    size_t capacity;
    std::unordered_map<std::string,
        std::pair<std::string, std::list<std::string>::iterator>> map;
    std::list<std::string> order;
    std::mutex mtx;

public:
    explicit Cache(size_t cap = 100) : capacity(cap) {}

    bool get(const std::string& key, std::string& value) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = map.find(key);
        if (it == map.end()) return false;
        order.erase(it->second.second);
        order.push_front(key);
        it->second.second = order.begin();
        value = it->second.first;
        return true;
    }

    void put(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = map.find(key);
        if (it != map.end()) order.erase(it->second.second);
        else if (map.size() >= capacity) {
            auto last = order.back();
            order.pop_back();
            map.erase(last);
        }
        order.push_front(key);
        map[key] = {value, order.begin()};
    }

    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = map.find(key);
        if (it != map.end()) {
            order.erase(it->second.second);
            map.erase(it);
        }
    }
};
