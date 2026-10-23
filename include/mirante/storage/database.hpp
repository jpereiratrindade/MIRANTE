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

    // Transação Atômica: Salvar Estado e Registrar Evento no Ledger (Garantia P0)
    std::expected<std::string, std::string> SaveSessionWithEvent(
        const domain::SessionAggregate& session,
        const std::string& event_type,
        const std::string& payload_json,
        const std::string& author
    );

    // Ledger Imutável de Eventos (Merkle-Chain INV-03)
    std::expected<std::string, std::string> AppendEvent(
        const std::string& session_id,
        const std::string& event_type,
        const std::string& payload_json,
        const std::string& author
    );
    std::expected<std::vector<domain::SessionEvent>, std::string> GetEventLedger(const std::string& session_id);
    std::expected<bool, std::string> VerifyLedgerIntegrity(const std::string& session_id);

    // Autenticação SisTer (E-mail + Código OTP + Sessões Passwordless)
    struct AuthUser {
        bool authenticated{false};
        std::string email;
        std::string name;
        std::string role{"researcher"};
    };

    std::expected<std::string, std::string> RequestOtp(const std::string& email, const std::string& name_hint = "");
    std::expected<std::pair<std::string, AuthUser>, std::string> VerifyOtp(const std::string& email, const std::string& code);
    std::expected<AuthUser, std::string> AuthenticateToken(const std::string& token);
    std::expected<void, std::string> RevokeToken(const std::string& token);

    // Utilitários Criptográficos e Aleatoriedade Segura
    static std::string Sha256(const std::string& input);
    static std::string RandomDigits(size_t count = 6);
    static std::string RandomHex(size_t bytes = 16);

private:
    sqlite3* db_{nullptr};
    std::mutex mutex_;

    std::expected<void, std::string> RunMigrations();
    std::string GetLastEventHash(const std::string& session_id);
    static bool SendOtpEmail(const std::string& to_email, const std::string& code, std::string& err_out);
};

} // namespace mirante::storage
