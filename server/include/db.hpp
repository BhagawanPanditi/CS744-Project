#pragma once
#include <cppconn/driver.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <cppconn/statement.h>
#include <memory>
#include <string>
#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>

// ---------------------------
// Connection Pool
// ---------------------------
class ConnectionPool {
private:
    sql::Driver* driver;
    std::string host, user, pass, dbname;
    std::queue<std::unique_ptr<sql::Connection>> pool;
    std::mutex mtx;
    std::condition_variable cv;
    size_t max_size;

public:
    ConnectionPool(sql::Driver* drv, const std::string& h, const std::string& u,
                   const std::string& p, const std::string& db, size_t size)
        : driver(drv), host(h), user(u), pass(p), dbname(db), max_size(size)
    {
        for (size_t i = 0; i < max_size; ++i) {
            auto con = std::unique_ptr<sql::Connection>(driver->connect(host, user, pass));
            con->setSchema(dbname);
            pool.push(std::move(con));
        }
    }

    std::unique_ptr<sql::Connection> acquire() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !pool.empty(); });
        auto con = std::move(pool.front());
        pool.pop();
        return con;
    }

    void release(std::unique_ptr<sql::Connection> con) {
        std::lock_guard<std::mutex> lock(mtx);
        pool.push(std::move(con));
        cv.notify_one();
    }
};

// ---------------------------
// Thread-safe MySQL KV Wrapper
// ---------------------------
class DB {
private:
    sql::Driver* driver;
    std::unique_ptr<ConnectionPool> pool;

public:
    DB(const std::string& host,
       const std::string& user,
       const std::string& pass,
       const std::string& dbname,
       size_t pool_size = 10)
    {
        try {
            driver = get_driver_instance();

            // Initial connection for DB + table setup
            auto con = std::unique_ptr<sql::Connection>(driver->connect(host, user, pass));
            {
                std::unique_ptr<sql::Statement> stmt(con->createStatement());
                stmt->execute("CREATE DATABASE IF NOT EXISTS `" + dbname + "`");
            }
            con->setSchema(dbname);
            {
                std::unique_ptr<sql::Statement> stmt(con->createStatement());
                stmt->execute(
                    "CREATE TABLE IF NOT EXISTS kv_store ("
                    "k VARCHAR(255) PRIMARY KEY, "
                    "v TEXT)"
                );
            }

            std::cout << "✅ Connected to MySQL and ready: " << dbname << std::endl;

            // Initialize connection pool
            pool = std::make_unique<ConnectionPool>(driver, host, user, pass, dbname, pool_size);
        } catch (sql::SQLException &e) {
            std::cerr << "❌ DB initialization failed: " << e.what() << std::endl;
            exit(1);
        }
    }

    void insert(const std::string& key, const std::string& value) {
        auto con = pool->acquire();
        try {
            auto pstmt = std::unique_ptr<sql::PreparedStatement>(
                con->prepareStatement("REPLACE INTO kv_store (k, v) VALUES (?, ?)")
            );
            pstmt->setString(1, key);
            pstmt->setString(2, value);
            pstmt->execute();
        } catch (sql::SQLException &e) {
            std::cerr << "❌ Insert failed: " << e.what() << std::endl;
        }
        pool->release(std::move(con));
    }

    std::string get(const std::string& key) {
        auto con = pool->acquire();
        std::string result;
        try {
            auto pstmt = std::unique_ptr<sql::PreparedStatement>(
                con->prepareStatement("SELECT v FROM kv_store WHERE k = ?")
            );
            pstmt->setString(1, key);
            auto res = std::unique_ptr<sql::ResultSet>(pstmt->executeQuery());
            if (res->next()) {
                result = res->getString("v");
            }
        } catch (sql::SQLException &e) {
            std::cerr << "❌ Read failed: " << e.what() << std::endl;
        }
        pool->release(std::move(con));
        return result;
    }

    void remove(const std::string& key) {
        auto con = pool->acquire();
        try {
            auto pstmt = std::unique_ptr<sql::PreparedStatement>(
                con->prepareStatement("DELETE FROM kv_store WHERE k = ?")
            );
            pstmt->setString(1, key);
            pstmt->execute();
        } catch (sql::SQLException &e) {
            std::cerr << "❌ Delete failed: " << e.what() << std::endl;
        }
        pool->release(std::move(con));
    }
};
