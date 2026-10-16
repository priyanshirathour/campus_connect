#include "CampusConnectApp.h"
#include <exception>
#include <iostream>
#include <string>

// Entry point belongs only to the campus_connect application target.
int main(int argc, char* argv[]) {
    try {
        CampusConnectApp application;
        if (argc == 2 && std::string(argv[1]) == "--api") {
            return application.runApi();
        }
        if (argc == 3 && std::string(argv[1]) == "--provision-student") {
            return application.provisionStudentAccount(argv[2]);
        }
        if (argc == 4 && std::string(argv[1]) == "--provision-coordinator") {
            return application.provisionCoordinatorAccount(argv[2], argv[3]);
        }
        if (argc > 1) {
            std::cerr << "Usage: campus_connect [--api | --provision-student ID | "
                         "--provision-coordinator EMAIL DISPLAY_NAME]\n";
            return 2;
        }
        return application.run();
    } catch (const std::exception& error) {
        std::cerr << "Campus Connect could not start: " << error.what() << '\n';
        std::cerr << "Check that the MongoDB Windows service is running and reachable at 127.0.0.1:27017.\n";
        return 1;
    }
}
