#pragma once

#include <iostream>
#include <libpq-fe.h>

const char* conninfo = "host=127.0.0.1 port=5433 dbname=banking user=admin password=root";

namespace service::banking {
class BankingDatabase {
    PGconn* conn;
    public:
    BankingDatabase() : conn(nullptr) {}
    bool Connect(const std::string& conninfo) {
        conn = PQconnectdb(conninfo.c_str()); 

        if (PQstatus(conn) != CONNECTION_OK) {
            std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << std::endl;
            return false;
        }

        std::cout << "Connected to the database successfully!" << std::endl;
        return true;
    }
   PGresult* GetCustomer(const std::string& name) {
    std::string query = "SELECT * FROM customer AS c "
                        "NATURAL JOIN depositor AS d  "
                        "NATURAL JOIN account AS a   "
                        "WHERE c.customer_name = '" + name + "';";

    PGresult* res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Query execution failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return nullptr;
    }

    return res;
    }

    PGresult* GetAllCustomers() {
    std::string query = "SELECT * FROM customer AS c "
                        "NATURAL JOIN depositor AS d  "
                        "NATURAL JOIN account AS a";

    PGresult* res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Query execution failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return nullptr;
    }

    return res;
    }

    bool isConnected() const {
        return conn != nullptr && PQstatus(conn) == CONNECTION_OK;
    }

    static void Print(PGresult* res) {
        if (res == nullptr) {
            std::cerr << "No account data to display." << std::endl;
            return;
        }

        int nFields = PQnfields(res);
        int nRows = PQntuples(res);

        for (int i = 0; i < nFields; i++) {
            std::cout << PQfname(res, i) << "\t";
        }
        std::cout << std::endl;

        for (int i = 0; i < nRows; i++) {
            for (int j = 0; j < nFields; j++) {
                std::cout << PQgetvalue(res, i, j) << "\t";
            }
            std::cout << std::endl;
        }
    }

    void Disconnect() {
        if (conn) {
            PQfinish(conn);
            conn = nullptr;
        }
    }

     ~BankingDatabase() {
        Disconnect();
    }
};
}