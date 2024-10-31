#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "json.hpp"
#include "tagfilterdb/service1.hpp"
#include "tagfilterdb/logging.hpp"
#include <iostream>
#include <libpq-fe.h>

using json = nlohmann::json;

void set_cors_headers(httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*"); // Allow requests from any origin
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS"); // Allowed methods
    res.set_header("Access-Control-Allow-Headers", "Content-Type"); // Allowed headers
}

const char* conninfo = "host=127.0.0.1 port=5433 dbname=banking user=admin password=root";

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

int main() {
    using namespace httplib;

    Server svr;

    BankingDatabase db;
    if (!db.Connect("host=127.0.0.1 port=5433 dbname=banking user=admin password=root")) {
            std::cerr << "Database connection failed." << std::endl;
            return 1;
    }
    tin_compiler::CompilerService cs;

    svr.Get("/", [](const Request &, Response &res) {
        set_cors_headers(res); 
        json response_json = {
            {"message", "Hello World!"}
        };
        
        res.set_content(response_json.dump(), "application/json");
    });

   svr.Get("/customer/:name", [&](const Request& req, Response& res) {
        set_cors_headers(res);
        auto customer_name = req.path_params.at("name");
        
        PGresult* res_db = db.GetCustomer(customer_name);  
        if (res_db) {
            json response_json;
            int nRows = PQntuples(res_db);
            int nFields = PQnfields(res_db);

            for (int i = 0; i < nRows; ++i) {
                json customer_json;
                for (int j = 0; j < nFields; ++j) {
                    customer_json[PQfname(res_db, j)] = PQgetvalue(res_db, i, j);
                }
                response_json.push_back(customer_json);
            }
            PQclear(res_db);
            res.set_content(response_json.dump(), "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"error\": \"Customer not found\"}", "application/json");
        }
    });

    svr.Get("/compiler/details", [&](const Request &, Response &res) {
        set_cors_headers(res);
        json response_json;
        response_json["Detail"] = cs.toJson();
       
        res.set_content(response_json.dump(), "application/json");
    });

    svr.Post("/compiler/plainText", [&](const Request& req, Response& res) {
        set_cors_headers(res);

        json response_json;
        if (req.has_header("Content-Length")) {
            response_json["content_length"] = req.get_header_value("Content-Length");
        }
        LOG_DEBUG(req.body)
        tin_compiler::CompilerService cs;

        cs.setInput(req.body);
        auto ast = cs.Parse(cs.Tokenize());
        if (ast != nullptr) {
            response_json["AST"] = tin_compiler::ASTNode::toJson(ast);
        } else {
            response_json["AST"] = "NULL";
        }

        res.set_content("", "text/plain");
        res.set_content(response_json.dump(), "application/json");
    });

    // Start the server
    svr.listen("0.0.0.0", 8080);
}
