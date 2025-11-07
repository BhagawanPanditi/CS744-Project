// kv_client.cpp
// Usage:
//   ./kv_client <workload_type> <load_level>
// Example:
//   ./kv_client put_all 32
//   ./kv_client get_all 64
//   ./kv_client get_popular 16
//   ./kv_client get_mix 32
//
// workload_type = put_all | get_all | get_popular | get_mix

#include "lib/httplib.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <sstream>

using namespace std::chrono;

struct Config {
    std::string host = "localhost";
    int port = 8080;
    size_t threads = 4;
    size_t duration_s = 30;
    std::string workload = "get_all";
    size_t key_space = 10000;   // Large key space for cache misses
    size_t popular_size = 20;   // Small set for cache hits
};

static std::atomic<uint64_t> successful_requests{0};
static std::atomic<uint64_t> total_requests{0};
static std::atomic<uint64_t> total_latency_ns{0};
static std::atomic<uint64_t> create_counter{0};

// --------------------------- HTTP Operations ---------------------------

void do_create(httplib::Client &cli, const std::string &key, const std::string &value) {
    auto t0 = steady_clock::now();
    std::string body = "key=" + key + "&value=" + value;
    auto res = cli.Post("/create", body, "application/x-www-form-urlencoded");
    auto t1 = steady_clock::now();

    total_requests.fetch_add(1, std::memory_order_relaxed);
    total_latency_ns.fetch_add(duration_cast<nanoseconds>(t1 - t0).count(), std::memory_order_relaxed);
    if (res && res->status >= 200 && res->status < 300)
        successful_requests.fetch_add(1, std::memory_order_relaxed);
}

void do_read(httplib::Client &cli, const std::string &key) {
    auto t0 = steady_clock::now();
    std::string path = "/read?key=" + httplib::detail::encode_path(key);
    auto res = cli.Get(path.c_str());
    auto t1 = steady_clock::now();

    total_requests.fetch_add(1, std::memory_order_relaxed);
    total_latency_ns.fetch_add(duration_cast<nanoseconds>(t1 - t0).count(), std::memory_order_relaxed);
    if (res && res->status >= 200 && res->status < 300)
        successful_requests.fetch_add(1, std::memory_order_relaxed);
}

void do_delete(httplib::Client &cli, const std::string &key) {
    auto t0 = steady_clock::now();
    std::string path = "/delete?key=" + httplib::detail::encode_path(key);
    auto res = cli.Delete(path.c_str());
    auto t1 = steady_clock::now();

    total_requests.fetch_add(1, std::memory_order_relaxed);
    total_latency_ns.fetch_add(duration_cast<nanoseconds>(t1 - t0).count(), std::memory_order_relaxed);
    if (res && res->status >= 200 && res->status < 300)
        successful_requests.fetch_add(1, std::memory_order_relaxed);
}

// --------------------------- Workloads ---------------------------

void worker_thread(const Config &cfg, size_t tid, steady_clock::time_point end_time) {
    httplib::Client cli(cfg.host.c_str(), cfg.port);
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(5, 0);

    std::mt19937 rng(std::random_device{}() ^ (tid << 16));
    std::uniform_int_distribution<size_t> key_dist(0, cfg.key_space - 1);
    std::uniform_int_distribution<size_t> pop_dist(0, cfg.popular_size - 1);
    std::uniform_int_distribution<int> percent(1, 100);

    while (steady_clock::now() < end_time) {
        if (cfg.workload == "put_all") {
            // Disk-bound write workload (creates mostly)
            if (percent(rng) <= 70) {
                uint64_t id = create_counter.fetch_add(1, std::memory_order_relaxed);
                std::string key = "key_" + std::to_string(id);
                std::string value = "value_" + std::to_string(rng());
                do_create(cli, key, value);
            } else {
                std::string key = "key_" + std::to_string(key_dist(rng));
                do_delete(cli, key);
            }
        }
        else if (cfg.workload == "get_all") {
            // Random reads (cache misses)
            std::string key = "key_" + std::to_string(key_dist(rng));
            do_read(cli, key);
        }
        else if (cfg.workload == "get_popular") {
            // Small shared hot set (cache hits)
            std::string key = "popular_" + std::to_string(pop_dist(rng));
            do_read(cli, key);
        }
        else if (cfg.workload == "get_mix") {
            // Mixed workload: 30% put, 60% get, 10% delete
            int op = percent(rng);
            if (op <= 30) {
                uint64_t id = create_counter.fetch_add(1, std::memory_order_relaxed);
                std::string key = "key_" + std::to_string(id);
                std::string value = "value_" + std::to_string(rng());
                do_create(cli, key, value);
            } else if (op <= 90) {
                std::string key = "key_" + std::to_string(key_dist(rng));
                do_read(cli, key);
            } else {
                std::string key = "key_" + std::to_string(key_dist(rng));
                do_delete(cli, key);
            }
        }
    }
}

// --------------------------- Utility ---------------------------

void populate_keys(const Config &cfg) {
    httplib::Client cli(cfg.host.c_str(), cfg.port);
    cli.set_connection_timeout(5, 0);

    if (cfg.workload == "get_all") {
        std::cout << "Populating " << cfg.key_space << " keys (cache miss test)...\n";
        for (size_t i = 0; i < cfg.key_space; ++i) {
            std::string key = "key_" + std::to_string(i);
            std::string value = "value_" + std::to_string(i);
            std::string body = "key=" + key + "&value=" + value;
            cli.Post("/create", body, "application/x-www-form-urlencoded");
            if (i % 2000 == 0)
                std::cout << "  inserted " << i << "\n";
        }
    } else if (cfg.workload == "get_popular") {
        std::cout << "Populating " << cfg.popular_size << " hot keys (cache hit test)...\n";
        for (size_t i = 0; i < cfg.popular_size; ++i) {
            std::string key = "popular_" + std::to_string(i);
            std::string value = "value_" + std::to_string(i);
            std::string body = "key=" + key + "&value=" + value;
            cli.Post("/create", body, "application/x-www-form-urlencoded");
        }
    }
    std::cout << "Prepopulation complete.\n\n";
}

void print_summary(const Config &cfg, double duration_s) {
    uint64_t succ = successful_requests.load();
    uint64_t tot = total_requests.load();
    uint64_t lat = total_latency_ns.load();

    double throughput = succ / duration_s;
    double avg_latency_ms = (tot > 0) ? (double)lat / tot / 1e6 : 0.0;

    std::cout << "\n========== RESULTS ==========\n";
    std::cout << "Workload: " << cfg.workload << "\n";
    std::cout << "Threads: " << cfg.threads << "\n";
    std::cout << "Duration: " << duration_s << " s\n";
    std::cout << "Total Requests: " << tot << "\n";
    std::cout << "Successful: " << succ << "\n";
    std::cout << "Throughput: " << throughput << " req/s\n";
    std::cout << "Avg Latency: " << avg_latency_ms << " ms\n";
    std::cout << "=============================\n";
}

// --------------------------- Main ---------------------------

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <workload_type> <load_level>\n";
        std::cerr << "  workload_type: put_all | get_all | get_popular | get_mix\n";
        std::cerr << "  load_level: number of threads (e.g., 4, 8, 16)\n";
        return 1;
    }

    Config cfg;
    cfg.workload = argv[1];
    cfg.threads = std::stoul(argv[2]);

    std::cout << "Starting load generator...\n";
    std::cout << "Server: " << cfg.host << ":" << cfg.port << "\n";
    std::cout << "Workload: " << cfg.workload << "\n";
    std::cout << "Threads: " << cfg.threads << "\n";
    std::cout << "Duration: " << cfg.duration_s << " seconds\n\n";

    if (cfg.workload == "get_all" || cfg.workload == "get_popular") {
        populate_keys(cfg);
    }

    auto start = steady_clock::now();
    auto end_time = start + seconds(cfg.duration_s);

    std::vector<std::thread> threads;
    for (size_t i = 0; i < cfg.threads; ++i) {
        threads.emplace_back(worker_thread, std::cref(cfg), i, end_time);
    }

    for (auto &t : threads) t.join();
    auto stop = steady_clock::now();

    double actual_duration = duration_cast<duration<double>>(stop - start).count();
    print_summary(cfg, actual_duration);

    return 0;
}
