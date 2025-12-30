#pragma once
#include <string>
class DBPool;

class UserStore {
public:
    UserStore(DBPool* db_pool): db_pool_(db_pool) {}
    bool register_user(const std::string& username, const std::string& password);
    bool check_login(const std::string& username, const std::string& password);

private:
    DBPool* db_pool_;
};