#include "service/banking/server.hpp"

int main() {
    try {
        server::banking::BankingServer bankingServer("host=127.0.0.1 port=5433 dbname=banking user=admin password=root");
        bankingServer.start(8080);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}