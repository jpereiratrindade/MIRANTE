#include "mirante/storage/database.hpp"
#include "mirante/assist/ollama_client.hpp"
#include "mirante/http/server.hpp"
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  Ecossistema SisTer — MIRANTE v0.1.0" << std::endl;
    std::cout << "  Experiência Assistida de Percepção Relacional (MPRR)" << std::endl;
    std::cout << "==========================================================" << std::endl;
    std::cout << "  Assinatura:" << std::endl;
    std::cout << "  MIRANTE — Deslocar perspectivas. Reconhecer relações. Construir questões." << std::endl;
    std::cout << "  Máxima Arquitetural:" << std::endl;
    std::cout << "  \"Automatizar a exploração. Preservar a dúvida. Registrar o percurso. Submeter a descoberta à crítica.\"" << std::endl;
    std::cout << "  Lema: \"Sempre pronto. Sempre incompleto.\"" << std::endl;
    std::cout << "==========================================================" << std::endl;

    std::string host = "0.0.0.0";
    if (const char* env_host = std::getenv("MIRANTE_HOST")) {
        host = env_host;
    }

    int port = 8088;
    if (const char* env_port = std::getenv("MIRANTE_PORT")) {
        port = std::stoi(env_port);
    }

    std::string db_path = "mirante.sqlite3";
    std::string web_dir = "web";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--db" && i + 1 < argc) {
            db_path = argv[++i];
        } else if (arg == "--web" && i + 1 < argc) {
            web_dir = argv[++i];
        }
    }

    auto db = std::make_shared<mirante::storage::Database>();
    auto db_res = db->Open(db_path);
    if (!db_res) {
        std::cerr << "[ERRO] Falha ao abrir banco de dados SQLite: " << db_res.error() << std::endl;
        return 1;
    }
    std::cout << "[INFO] Banco de dados SQLite aberto em: " << db_path << std::endl;

    auto ollama = std::make_shared<mirante::assist::OllamaClient>();
    if (ollama->IsOnline()) {
        std::cout << "[INFO] Daemon Ollama Local detectado e ONLINE (localhost:11434)" << std::endl;
        auto models = ollama->ListModels();
        std::cout << "[INFO] Modelos locais homologados disponíveis: " << models.size() << std::endl;
        for (const auto& m : models) {
            std::cout << "       - " << m.name << " (" << m.digest << ")" << std::endl;
        }
    } else {
        std::cout << "[AVISO] Daemon Ollama Local não detectado ou offline." << std::endl;
        std::cout << "[AVISO] MIRANTE operando em MODO HUMANO PURO (INV-05 satisfeito)." << std::endl;
    }

    std::cout << "[INFO] Escutando em: http://" << host << ":" << port << std::endl;
    std::cout << "       Acesso local: http://localhost:" << port << std::endl;
    std::cout << "       Acesso rede:  http://10.163.80.176:" << port << std::endl;

    mirante::http::HttpServer server(db, ollama, host, web_dir, port);
    server.Start();

    return 0;
}
