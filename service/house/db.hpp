#pragma once

#include <iostream>
#include <libpq-fe.h>
#include "tagfilterdb/logging.hpp"

namespace service::house {
class HouseDatabase {
    PGconn* conn;
    public:
    HouseDatabase() : conn(nullptr) {}
    bool Connect(const std::string& conninfo) {
        conn = PQconnectdb(conninfo.c_str()); 

        if (PQstatus(conn) != CONNECTION_OK) {
            std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << std::endl;
            return false;
        }

        std::cout << "Connected to the database successfully!" << std::endl;
        return true;
    }
   PGresult* GetHouse(int houseId) {
 
     std::string query = "SELECT * FROM House WHERE houseid = " + std::to_string(houseId);

    PGresult* res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Query execution failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return nullptr;
    }

    return res;
    }

    PGresult* GetRoom(int houseId) {
     std::string query = "SELECT * FROM Room WHERE houseid = " + std::to_string(houseId);

    PGresult* res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Query execution failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return nullptr;
    }

    return res;
    }

    PGresult* AddRoom(int houseId, const std::string& roomName, const std::string& roomType, 
                  int floorNumber, double area, const std::string& dimensions, bool hasWindow) {
        std::string insertRoomQuery = 
            "INSERT INTO Room (RoomId, RoomName, HouseID, RoomType, FloorNumber, Area, Dimensions, HasWindow) "
            "VALUES ( (SELECT COALESCE(MAX(RoomId), 0) + 1 FROM Room),'" + roomName + "', " + std::to_string(houseId) + ", '" + roomType + "', " +
            std::to_string(floorNumber) + ", " + std::to_string(area) + ", '" + dimensions + "', " +
            (hasWindow ? "TRUE" : "FALSE") + ")";


        PGresult* res_insert = PQexec(conn, insertRoomQuery.c_str());

        if (PQresultStatus(res_insert) != PGRES_COMMAND_OK) {
            std::cerr << "Error inserting room: " << PQerrorMessage(conn) << std::endl;
            PQclear(res_insert);
            return nullptr;
        }

        return res_insert;
    }

    PGresult* DeleteRoom(int roomId) {
        std::string deleteRoomQuery = "DELETE FROM Room WHERE roomid = " + std::to_string(roomId);

        PGresult* res_delete = PQexec(conn, deleteRoomQuery.c_str());

        if (PQresultStatus(res_delete) != PGRES_COMMAND_OK) {
            std::cerr << "Error deleting room: " << PQerrorMessage(conn) << std::endl;
            PQclear(res_delete);
            return nullptr;
       }
      
        return res_delete; 
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

     ~HouseDatabase() {
        Disconnect();
    }
};
}