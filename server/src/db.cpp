// db.cpp
#include <cppconn/driver.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <iostream>
#include <memory>
#include <string>

class DB {
private:
    sql::Driver* driver;
    std::unique_ptr<sql::Connection> con;

public:
    DB(const std::string& host,
       const std::string& user,
       const std::string& pass,
       const std::string& dbname) {
        try {
            driver = get_driver_instance();
            con.reset(driver->connect(host, user, pass));
            {
                std::unique_ptr<sql::Statement> stmt(con->createStatement());
                std::string create_db_sql = "CREATE DATABASE IF NOT EXISTS `" + dbname + "`";
                stmt->execute(create_db_sql);
            }
            con->setSchema(dbname);
            std::cout << "✅ Connected to MySQL database: " << dbname << std::endl;
            std::unique_ptr<sql::Statement> stmt(con->createStatement());
            stmt->execute(
                "CREATE TABLE IF NOT EXISTS kv_store ("
                "k VARCHAR(255) PRIMARY KEY, "
                "v TEXT)"
            );
        } catch (sql::SQLException &e) {
            std::cerr << "❌ DB Connection failed: " << e.what() << std::endl;
            exit(1);
        }
    }

    void insert(const std::string& key, const std::string& value) {
        try {
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
            std::unique_ptr<sql::PreparedStatement> pstmt(
                con->prepareStatement("SELECT v FROM kv_store WHERE k = ?")
            );
            pstmt->setString(1, key);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (res->next()) {
                return res->getString("v");
            } else {
                std::cout << "⚠️  Key not found: " << key << std::endl;
            }
        } catch (sql::SQLException &e) {
            std::cerr << "❌ Read failed: " << e.what() << std::endl;
        }
        return "";
    }

    void remove(const std::string& key) {
        try {
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
