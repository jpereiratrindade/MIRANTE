#pragma once

#include "mirante/domain/entities.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <expected>
#include <sqlite3.h>

namespace mirante::storage {

class Database {
public:
    Database() = default;
    ~Database();

    // Desabilitar cópia
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    std::expected<void, std::string> Open(const std::string& db_path);
    void Close();

    // Gestão de Sessões
    std::expected<void, std::string> SaveSession(const domain::SessionAggregate& session);
    std::expected<domain::SessionAggregate, std::string> LoadSession(const std::string& session_id);
    std::expected<std::vector<domain::SessionAggregate>, std::string> ListSessions();

    // Ledger Imutável de Eventos (Merkle-Chain INV-03)
    std::expected<std::string, std::string> AppendEvent(
        const std::string& session_id,
        const std::string& event_type,
        const std::string& payload_json,
        const std::string& author
    );
    std::expected<std::vector<domain::SessionEvent>, std::string> GetEventLedger(const std::string& session_id);
    std::expected<bool, std::string> VerifyLedgerIntegrity(const std::string& session_id);

    // Utilitário Criptográfico
    static std::string Sha256(const std::string& input);

private:
    sqlite3* db_{nullptr};
    std::mutex mutex_;

    std::expected<void, std::string> RunMigrations();
    std::string GetLastEventHash(const std::string& session_id);
};

} // namespace mirante::storage
