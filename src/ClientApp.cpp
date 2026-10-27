#include "ClientApp.h"
#include "Authenticator.h"
#include "ConfigReader.h"
#include "DataExchanger.h"
#include "ErrorHandler.h"
#include "FileManager.h"
#include "TcpClient.h"
#include "errors.h"
#include <getopt.h>
#include <iostream>

void ClientApp::printHelp(const char* prog) {
    std::cout <<
        "Использование: " << prog << " -a АДРЕС -i ВХОДНОЙ_ФАЙЛ -o ВЫХОДНОЙ_ФАЙЛ [-p ПОРТ] [-c КОНФИГ]\n"
        "  -a АДРЕС   сетевой адрес сервера (обязательный)\n"
        "  -p ПОРТ    порт сервера (по умолчанию 33333)\n"
        "  -i ФАЙЛ    файл с исходными данными (обязательный)\n"
        "  -o ФАЙЛ    файл для сохранения результатов (обязательный)\n"
        "  -c ФАЙЛ    файл с LOGIN и PASSWORD (по умолчанию ~/.config/vclient.conf)\n"
        "  -h         вывести эту справку\n";
}

bool ClientApp::parseArgs(int argc, char** argv) {
    if (argc == 1) return false;
    optind = 1;
    int c;
    while ((c = getopt(argc, argv, "ha:p:i:o:c:")) != -1) {
        switch (c) {
        case 'h': return false;
        case 'a': addr_ = optarg; break;
        case 'i': inFile_ = optarg; break;
        case 'o': outFile_ = optarg; break;
        case 'c': confFile_ = optarg; break;
        case 'p': {
            std::string s = optarg;
            size_t pos = 0; long v = -1;
            try { v = std::stol(s, &pos); } catch (...) { pos = 0; }
            if (pos != s.size() || s.empty() || v < 1 || v > 65535)
                throw ClientError("ClientApp::parseArgs", "port='" + s + "'", "недопустимое значение порта (1..65535)");
            port_ = static_cast<uint16_t>(v);
            break;
        }
        default:
            throw ClientError("ClientApp::parseArgs", "опция='" + std::string(1, static_cast<char>(optopt)) + "'",
                              "неизвестная опция или отсутствует аргумент");
        }
    }
    if (optind < argc)
        throw ClientError("ClientApp::parseArgs", "аргумент='" + std::string(argv[optind]) + "'", "лишний аргумент");
    if (addr_.empty() || inFile_.empty() || outFile_.empty())
        throw ClientError("ClientApp::parseArgs",
                          "addr='" + addr_ + "', input='" + inFile_ + "', output='" + outFile_ + "'",
                          "не заданы обязательные параметры (-a, -i, -o)");
    return true;
}

int ClientApp::run() {
    ConfigReader cfg;
    FileManager fm;
    Credentials cred = cfg.readConf(confFile_);
    std::vector<VectorData> vecs = fm.readVectors(inFile_);

    TcpClient tcp;
    tcp.connect(addr_, port_);
    Authenticator auth(cred);
    auth.auth(tcp);

    DataExchanger ex(tcp);
    ex.sendCount(static_cast<uint32_t>(vecs.size()));
    std::vector<double> results;
    results.reserve(vecs.size());
    for (const VectorData& v : vecs) {
        ex.sendVec(v);
        results.push_back(ex.recvRes());
    }
    tcp.close();
    fm.writeResults(outFile_, results);
    return 0;
}
