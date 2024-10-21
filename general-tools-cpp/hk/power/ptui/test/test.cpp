#include "listen.h"
#include <thread>
#include    <unistd.h>
#include <chrono>
#include <iostream>

int run_server() {
    return system("./bin/debug-server 127.0.0.1 7777 &");
} 

int run_client() {
    return system("./bin/debug-client 127.0.0.1 7776 127.0.0.1 7777 &");
}


/**
 * @brief A simple wrapper to test HKADCNode communication.
 * 
 * Starts a server to send fake Housekeeping messages, and a client that asks for them and writes responses to log/.
 */
int main() {
    int id = fork();
    if (id < 0) {
        exit(EXIT_FAILURE);
    } else if (id == 0) {
        int pid_s = run_server();
        std::cout << "started server: " << id << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::string prefix = "kill ";
        system((prefix + std::to_string(id)).c_str());
        
        exit(EXIT_SUCCESS);
    
    } else {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // auto t2 = std::async(std::launch::async, run_client);
        int pid_c = run_client();
        std::cout << "started client: " << id << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));

        exit(EXIT_SUCCESS);

    }
}