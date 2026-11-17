#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>
namespace dojo {
class CallbackRegistry {
public:
    using Token=std::uint64_t;
    Token subscribe(std::function<void(int)> callback); // TODO reject empty callback
    bool unsubscribe(Token token); // TODO
    void publish(int value);       // TODO snapshot then call outside lock
private:
    std::mutex mutex_;
    std::map<Token,std::function<void(int)>> callbacks_;
    Token next_=1;
};
inline CallbackRegistry::Token CallbackRegistry::subscribe(std::function<void(int)> callback) {
    if (!callback) throw std::invalid_argument("empty callback");
    std::lock_guard<std::mutex> lock(mutex_);
    const Token token = next_++;
    callbacks_.emplace(token, std::move(callback));
    return token;
}

inline bool CallbackRegistry::unsubscribe(Token token) {
    std::lock_guard<std::mutex> lock(mutex_);
    return callbacks_.erase(token) != 0;
}

inline void CallbackRegistry::publish(int value) {
    std::vector<std::function<void(int)>> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot.reserve(callbacks_.size());
        for (const auto& entry : callbacks_) {
            snapshot.push_back(entry.second);
        }
    }
    for (const auto& callback : snapshot) {
        callback(value);
    }
}

}
