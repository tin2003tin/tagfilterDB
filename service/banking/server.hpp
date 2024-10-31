#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "json.hpp"
#include "service/banking/sql.hpp"
#include "service/banking/db.hpp"
#include "tagfilterdb/logging.hpp"
#include <iostream>

using json = nlohmann::json;
using namespace service::banking;

void set_cors_headers(httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*"); // Allow requests from any origin
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS"); // Allowed methods
    res.set_header("Access-Control-Allow-Headers", "Content-Type"); // Allowed headers
}

namespace server::banking {
    class BankingServer {
    public:
        BankingServer(const std::string& db_conninfo) : db(), server(), compiler() {
            if (!db.Connect(db_conninfo)) {
                std::cerr << "Database connection failed." << std::endl;
                throw std::runtime_error("Database connection failed.");
            }
            setupRoutes();
        }

        void start(int port) {
            std::cout << "Starting server on port " << port << "..." << std::endl;
            server.listen("0.0.0.0", port);
        }

        ~BankingServer() {
            cleanup();
        }

    private:
        BankingDatabase db;
        httplib::Server server;
        BankingCompiler compiler;

        void setupRoutes() {
            server.Get("/", [](const httplib::Request &, httplib::Response &res) {
                set_cors_headers(res);
                json response_json = {
                    {"message", "Hello World!"}
                };
                res.set_content(response_json.dump(), "application/json");
            });

            server.Get("/customers", [&](const httplib::Request&, httplib::Response& res) {
                set_cors_headers(res);
                getAllCustomers(res);
            });

            server.Get("/customer/:name", [&](const httplib::Request& req, httplib::Response& res) {
                set_cors_headers(res);
                getCustomer(req, res);
                
            });

            server.Get("/compiler/details", [&](const httplib::Request &, httplib::Response &res) {
                set_cors_headers(res);
                getCompilerDetails(res);
            });

            server.Post("/compiler/plainText", [&](const httplib::Request& req, httplib::Response& res) {
                set_cors_headers(res);
                getAST(req,res);
            });
        
            server.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
                set_cors_headers(res);
                healthCheck(res);
            });
        }

        void getCustomer(const httplib::Request& req, httplib::Response& res) {
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
        }

        void getAllCustomers(httplib::Response& res) {
            PGresult* res_db = db.GetAllCustomers(); 
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
                res.set_content("{\"error\": \"No customers found\"}", "application/json");
            }
        }

        void getCompilerDetails(httplib::Response& res) {
            json response_json;
            response_json["Detail"] = compiler.toJson();
            res.set_content(response_json.dump(), "application/json");
        }

        void getAST(const httplib::Request& req, httplib::Response& res) {
            json response_json;
            if (req.has_header("Content-Length")) {
                    response_json["content_length"] = req.get_header_value("Content-Length");
            }
            LOG_DEBUG(req.body)

            compiler.setInput(req.body);
            auto ast = compiler.Parse(compiler.Tokenize());
            if (ast != nullptr) {
                response_json["AST"] = tin_compiler::ASTNode::toJson(ast);
            } else {
                response_json["AST"] = "NULL";
            }

            res.set_content("", "text/plain");
            res.set_content(response_json.dump(), "application/json");
        }

        void healthCheck(httplib::Response& res) {
            if (db.isConnected()) {
                res.set_content("{\"status\": \"healthy\"}", "application/json");
            } else {
                res.status = 500;
                res.set_content("{\"error\": \"Database connection is down\"}", "application/json");
            }
        }

        void cleanup() {
            db.Disconnect();
            std::cout << "Cleanup done." << std::endl;
        }
    };
}
