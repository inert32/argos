#ifndef __TH_QUEUE_H__
#define __TH_QUEUE_H__

/* 
    Потокобезопасная очередь
*/

#include <queue>
#include <mutex>
#include <optional>

template<class T>
class th_queue {
public:
    th_queue() {}
    th_queue(const th_queue& other) {
        std::lock_guard<std::mutex> lock(other.m);
        storage = other.storage;
    }
    th_queue& operator=(const th_queue&) = delete;

    void add(T value) {
        std::lock_guard<std::mutex> lock(m);
        storage.push(value);
    }

    std::optional<T> take() {
        std::lock_guard<std::mutex> lock(m);
        if (storage.empty()) return {};
        auto ret = storage.front();
        storage.pop();
        return ret;
    }

    size_t count() const {
        std::lock_guard<std::mutex> lock(m);
        return storage.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return storage.empty();
    }
private:
    std::queue<T> storage;
    mutable std::mutex m;
};

#endif /* __TH_QUEUE_H__ */