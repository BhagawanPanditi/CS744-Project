// ./kv_server 8080 1000 8

#include "lib/httplib.h"
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include "db.hpp"
#include "cache.hpp"
#include "thread_pool.hpp"

int main(int argc, char** argv) {

    int port = 8080;
    size_t cache_size = 1000;
    size_t pool_threads = 8;

    if (argc < 4) {
        std::cout << "Usage: ./kv_server <port> <cache_entries> <threads>\n";
        std::cout << "Example: ./kv_server 8080 1000 8\n";
        return 0;
    }

    port = std::atoi(argv[1]);
    cache_size = std::stoul(argv[2]);
    pool_threads = std::stoul(argv[3]);



    std::cout << "Starting Product Catalog KV Server\n";
    std::cout << "-----------------------------------\n";
    std::cout << "Port:          " << port << "\n";
    std::cout << "Cache entries: " << cache_size << "\n";
    std::cout << "Thread pool:   " << pool_threads << "\n";
    std::cout << "-----------------------------------\n";

    // Initialize backend components
    DB db("tcp://127.0.0.1:3306", "root", "password", "shopping_catalog");
    Cache cache(cache_size);
    ThreadPool pool(pool_threads);
    httplib::Server svr;

    svr.Post("/create", [&](const httplib::Request& req, httplib::Response& res) {
        if (!(req.has_param("key") && req.has_param("value"))) {
            res.status = 400;
            res.set_content("Missing key/value\n", "text/plain");
            return;
        }

        auto key = req.get_param_value("key");
        auto value = req.get_param_value("value");

        auto future = pool.enqueue([&db, &cache, key, value]() {
            db.insert(key, value);
            cache.put(key, value);
        });
        future.get();

        res.set_content("Inserted (" + key + ")\n", "text/plain");
    });

    // GET /read — read operation (cache first, DB fallback)
    svr.Get("/read", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("key")) {
            res.status = 400;
            res.set_content("Missing key\n", "text/plain");
            return;
        }

        auto key = req.get_param_value("key");
        std::string value;

        if (cache.get(key, value)) {
            res.set_content("[CACHE] " + value + "\n", "application/json");
            return;
        }

        auto future = pool.enqueue([&db, key]() {
            return db.get(key);
        });
        value = future.get();

        if (value.empty()) {
            res.status = 404;
            res.set_content("Not found\n", "text/plain");
        } else {
            cache.put(key, value);
            res.set_content("[DB] " + value + "\n", "application/json");
        }
    });

    // DELETE /delete — remove key from DB and cache
    svr.Delete("/delete", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("key")) {
            res.status = 400;
            res.set_content("Missing key\n", "text/plain");
            return;
        }

        auto key = req.get_param_value("key");

        auto future = pool.enqueue([&db, &cache, key]() {
            db.remove(key);
            cache.remove(key);
        });
        future.get();

        res.set_content("Deleted " + key + "\n", "text/plain");
    });

    std::cout << "✅ Server ready on http://0.0.0.0:" << port << "\n";
    svr.listen("0.0.0.0", port);
}

