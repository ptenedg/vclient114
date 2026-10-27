#include "ClientApp.h"
#include "ErrorHandler.h"
#include "errors.h"
#include <exception>

int main(int argc, char** argv) {
    ClientApp app;
    try {
        if (!app.parseArgs(argc, argv)) {
            ClientApp::printHelp(argv[0]);
            return 0;
        }
        return app.run();
    } catch (const ClientError& e) {
        ErrorHandler::abort(e.what());
    } catch (const std::exception& e) {
        ErrorHandler::abort(std::string("непредвиденная ошибка: ") + e.what());
    }
}
