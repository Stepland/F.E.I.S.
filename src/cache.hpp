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
        using key_type = Key;
        using value_type = Value;
        using reference_type = std::reference_wrapper<Value>;
        using const_reference_type = std::reference_wrapper<const Value>;
        Cache(std::function<Value(Key)> _load_resource): load_resource(_load_resource) {};

        // Does not trigger loading
        std::optional<const_reference_type> get(const Key& key) const {
            std::shared_lock lock{mapping_mutex};
            if (has(key)) {
                return std::cref(mapping.at(key));
            } else {
                return {};
            }
        }

        // Returns empty if not already loaded
        std::optional<reference_type> async_load(const Key& key) {
            if (not has(key)) {
                if (not is_loading(key)) {
                    async_emplace(key);
                }
                return {};
            } else {
                return get(key);
            }
        }

        void async_emplace(const Key& key) {
            std::thread t(&Cache::blocking_emplace, this, key);
            t.detach();
        }


        void blocking_emplace(const Key& key) {
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

        reference_type blocking_load(const Key& key) {
            std::shared_lock lock{mapping_mutex};
            blocking_emplace(key);
            return mapping.at(key);
        }

        bool has(const Key& key) const {
            std::shared_lock lock{mapping_mutex};
            return mapping.find(key) != mapping.end();
        }

        bool is_loading(const Key& key) const {
            std::shared_lock lock{currently_loading_mutex};
            return currently_loading.find(key) != currently_loading.end();
        }

        void reserve(const std::size_t& n) const {
            std::scoped_lock lock{mapping_mutex, currently_loading_mutex};
            mapping.reserve(n);
            currently_loading.reserve(n);
        }

    private:
        std::map<Key, Value> mapping;
        mutable std::shared_mutex mapping_mutex;
        std::set<Key> currently_loading;
        mutable std::shared_mutex currently_loading_mutex;
        std::function<Value(Key)> load_resource;
    };
}
