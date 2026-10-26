#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
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
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
inline CallbackRegistry::Token CallbackRegistry::subscribe(std::function<void(int)> callback) { (void)callback; return 0; /* TODO */ }
inline bool CallbackRegistry::unsubscribe(Token token) { (void)token; return false; /* TODO */ }
inline void CallbackRegistry::publish(int value) { (void)value; /* TODO */ }

}
