#include "service/house/server.hpp"

int main() {
    try {
        server::house::HouseServer locationServer("host=127.0.0.1 port=5433 dbname=tagfilterdb user=admin password=root");
        locationServer.start(8080);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}