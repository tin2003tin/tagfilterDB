#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "json.hpp"
#include "service/house/db.hpp"
#include "tagfilterdb/logging.hpp"
#include <iostream>

using json = nlohmann::json;
using namespace service::house;


const char* dd_conninfo = "host=127.0.0.1 port=5433 dbname=banking user=admin password=root";


void set_cors_headers(httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*"); // Allow requests from any origin
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS"); // Allowed methods
    res.set_header("Access-Control-Allow-Headers", "Content-Type"); // Allowed headers
}

namespace server::house {
    class HouseServer {
    public:
        HouseServer(const std::string& db_conninfo = dd_conninfo) : db(), server() {
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

        ~HouseServer() {
            cleanup();
        }

    private:
        HouseDatabase db;
        httplib::Server server;

        void setupRoutes() {
            server.Get("/", [](const httplib::Request &, httplib::Response &res) {
                set_cors_headers(res);
                json response_json = {
                    {"message", "Hello World!"}
                };
                res.set_content(response_json.dump(), "application/json");
            });

            server.Get("/room/:houseid", [&](const httplib::Request& req, httplib::Response& res) {
                set_cors_headers(res);
                getRoom(req, res);
            });

            server.Get("/house/:houseid", [&](const httplib::Request& req, httplib::Response& res) {
                set_cors_headers(res);
                getHouse(req, res);
            });

            server.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
                set_cors_headers(res);
                healthCheck(res);
            });

            server.Post("/house/:houseid/room", [&](const httplib::Request& req, httplib::Response& res) {
                set_cors_headers(res);
                addRoom(req, res);
            });

            server.Post("/house/:houseid/room", [&](const httplib::Request& req, httplib::Response& res) {
                set_cors_headers(res);
                addRoom(req, res);
            });

            server.Delete("/room/:roomid", [&](const httplib::Request& req, httplib::Response& res) {
                set_cors_headers(res);
                deleteRoom(req, res);
            });
        }

      
        void healthCheck(httplib::Response& res) {
            if (db.isConnected()) {
                res.set_content("{\"status\": \"healthy\"}", "application/json");
            } else {
                res.status = 500;
                res.set_content("{\"error\": \"Database connection is down\"}", "application/json");
            }
        }

        void getRoom(const httplib::Request& req, httplib::Response& res) {
            auto houseId = req.path_params.at("houseid");

            PGresult* res_db = db.GetRoom(std::stoi(houseId));
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
                    res.set_content("{\"error\": \"Room not found\"}", "application/json");
                }
        }

        void getHouse(const httplib::Request& req, httplib::Response& res) {
            auto houseid = req.path_params.at("houseid");

            PGresult* res_db = db.GetHouse(std::stoi(houseid));
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
                    res.set_content("{\"error\": \"House not found\"}", "application/json");
                }
        }

        void addRoom(const httplib::Request& req, httplib::Response& res) {

                int houseId = std::stoi(req.path_params.at("houseid"));
                
                // Parse the incoming JSON body
                try {
                    json room_data = json::parse(req.body);
                    
                    std::string roomName = room_data["RoomName"];
                    std::string roomType = room_data["RoomType"];
                    int floorNumber = room_data["FloorNumber"];
                    double area = room_data["Area"];
                    std::string dimensions = room_data["Dimensions"];
                    bool hasWindow = room_data["HasWindow"];
                    
                    PGresult* res_db = db.AddRoom(houseId, roomName, roomType, floorNumber, area, dimensions, hasWindow);
                    
                    if (PQresultStatus(res_db) != PGRES_COMMAND_OK) {
                        res.status = 500;
                        res.set_content("{\"error\": \"Failed to add room\"}", "application/json");
                    } else {
                        res.set_content("{\"message\": \"Room added successfully\"}", "application/json");
                    }
                     PQclear(res_db);
                } catch (const std::exception& e) {
                    res.status = 400;
                    res.set_content("{\"error\": \"Invalid input\"}", "application/json");
                }
        }

        void deleteRoom(const httplib::Request& req, httplib::Response& res) {
             int roomid = std::stoi(req.path_params.at("roomid"));
             try {
                    PGresult* res_db = db.DeleteRoom(roomid);
                    
                    if (PQresultStatus(res_db) != PGRES_COMMAND_OK) {
                        res.status = 500;
                        res.set_content("{\"error\": \"Failed to remove room\"}", "application/json");
                    } else {
                        res.set_content("{\"message\": \"Room removed successfully\"}", "application/json");
                    }
                    PQclear(res_db);
                } catch (const std::exception& e) {
                    res.status = 400;
                    res.set_content("{\"error\": \"Invalid input\"}", "application/json");
                }
        }

        void cleanup() {
            db.Disconnect();
            std::cout << "Cleanup done." << std::endl;
        }
    };
}
