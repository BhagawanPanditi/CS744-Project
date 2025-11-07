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

// Thread-safe MySQL wrapper for key-value operations
class DB {
private:
    sql::Driver* driver;
    std::string host, user, pass, dbname;
    std::mutex init_mtx;  // protect DB initialization

    // helper to create a new connection for each query
    std::unique_ptr<sql::Connection> new_conn() {
        auto c = std::unique_ptr<sql::Connection>(driver->connect(host, user, pass));
        c->setSchema(dbname);
        return c;
    }

public:
    DB(const std::string& host_,
       const std::string& user_,
       const std::string& pass_,
       const std::string& dbname_)
       : host(host_), user(user_), pass(pass_), dbname(dbname_) 
    {
        try {
            driver = get_driver_instance();

            // One-time initialization (create DB + table)
            std::lock_guard<std::mutex> lock(init_mtx);
            auto con = std::unique_ptr<sql::Connection>(driver->connect(host, user, pass));
            {
                std::unique_ptr<sql::Statement> stmt(con->createStatement());
                stmt->execute("CREATE DATABASE IF NOT EXISTS `" + dbname + "`");
            }
            con->setSchema(dbname);
            std::unique_ptr<sql::Statement> stmt(con->createStatement());
            stmt->execute(
                "CREATE TABLE IF NOT EXISTS kv_store ("
                "k VARCHAR(255) PRIMARY KEY, "
                "v TEXT)"
            );

            std::cout << "✅ Connected to MySQL and ready: " << dbname << std::endl;
        } catch (sql::SQLException &e) {
            std::cerr << "❌ DB initialization failed: " << e.what() << std::endl;
            exit(1);
        }
    }

    void insert(const std::string& key, const std::string& value) {
        try {
            auto con = new_conn();
            std::unique_ptr<sql::PreparedStatement> pstmt(
                con->prepareStatement("REPLACE INTO kv_store (k, v) VALUES (?, ?)")
            );
            pstmt->setString(1, key);
            pstmt->setString(2, value);
            pstmt->execute();
            std::cout << "✅ Inserted (" << key << ")\n";
        } catch (sql::SQLException &e) {
            std::cerr << "❌ Insert failed: " << e.what() << std::endl;
        }
    }

    std::string get(const std::string& key) {
        try {
            auto con = new_conn();
            std::unique_ptr<sql::PreparedStatement> pstmt(
                con->prepareStatement("SELECT v FROM kv_store WHERE k = ?")
            );
            pstmt->setString(1, key);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (res->next()) {
                return res->getString("v");
            }
        } catch (sql::SQLException &e) {
            std::cerr << "❌ Read failed: " << e.what() << std::endl;
        }
        return "";
    }

    void remove(const std::string& key) {
        try {
            auto con = new_conn();
            std::unique_ptr<sql::PreparedStatement> pstmt(
                con->prepareStatement("DELETE FROM kv_store WHERE k = ?")
            );
            pstmt->setString(1, key);
            pstmt->execute();
            std::cout << "🗑️  Deleted key: " << key << std::endl;
        } catch (sql::SQLException &e) {
            std::cerr << "❌ Delete failed: " << e.what() << std::endl;
        }
    }
};
