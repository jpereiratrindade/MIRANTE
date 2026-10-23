#pragma once

#include "mirante/storage/database.hpp"
#include "mirante/assist/ollama_client.hpp"
#include <string>
#include <memory>

namespace mirante::http {

class HttpServer {
public:
    HttpServer(
        std::shared_ptr<storage::Database> db,
        std::shared_ptr<assist::OllamaClient> ollama,
        std::string host = "0.0.0.0",
        std::string web_dir = "web",
        int port = 8088
    );
    ~HttpServer();

    void Start();
    void Stop();

private:
    std::shared_ptr<storage::Database> db_;
    std::shared_ptr<assist::OllamaClient> ollama_;
    std::string host_;
    std::string web_dir_;
    int port_;
    bool running_{false};

    void SetupRoutes(void* svr_ptr);
};

} // namespace mirante::http
