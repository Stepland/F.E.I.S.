#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <thread>

namespace Toolkit {
    /* Asynchronously Loads a Value associated with a Key in another thread and
    stores it for later reuse */
    template <class Key, class Value>
    class Cache {
    public:
        Cache(std::function<Value(Key)> _load_resource): load_resource(_load_resource) {};

        // Triggers async loading and returns empty if not already loaded
        std::optional<Value> async_get(const Key& key) {
            if (not has(key)) {
                if (not is_loading(key)) {
                    async_load(key);
                }
                return {};
            } else {
                return get(key);
            }
        }

        // Does not trigger loading
        std::optional<Value> get(const Key& key)  {
            std::shared_lock lock{mapping_mutex};
            if (has(key)) {
                return mapping.at(key);
            } else {
                return {};
            }
        }

        // Blocks until loaded
        Value blocking_get(const Key& key) {
            std::shared_lock lock{mapping_mutex};
            blocking_load(key);
            while (is_loading(key)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            return mapping.at(key);
        }

        void blocking_load(const Key& key) {
            if (has(key) or is_loading(key)) {
                return;
            }
            {
                std::unique_lock lock{currently_loading_mutex};
                currently_loading.insert(key);
            }
            Value resource = load_resource(key);
            {
                std::scoped_lock lock{mapping_mutex, currently_loading_mutex};
                mapping.emplace(key, resource);
                currently_loading.erase(key);
            }
        }
        
        void async_load(const Key& key) {
            std::thread t(&Cache::blocking_load, this, key);
            t.detach();
        }

        bool has(const Key& key) {
            std::shared_lock lock{mapping_mutex};
            return mapping.find(key) != mapping.end();
        }

        bool is_loading(const Key& key) {
            std::shared_lock lock{currently_loading_mutex};
            return currently_loading.find(key) != currently_loading.end();
        }

        void reserve(const std::size_t& n) {
            std::scoped_lock lock{mapping_mutex, currently_loading_mutex};
            mapping.reserve(n);
            currently_loading.reserve(n);
        }

    private:
        std::map<Key, Value> mapping;
        std::shared_mutex mapping_mutex;
        std::set<Key> currently_loading;
        std::shared_mutex currently_loading_mutex;
        std::function<Value(Key)> load_resource;
    };
}
