#include "Server.h"

int main() {
    Server server;
    if (server.initialize()) {
        server.start();
    }
    return 0;
}
