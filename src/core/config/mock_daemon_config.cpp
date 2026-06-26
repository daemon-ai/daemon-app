#include "config/mock_daemon_config.h"

#include "appcache/json_cache.h"

namespace config {

namespace {
const QString kCacheFile = QStringLiteral("daemon_config.json");
} // namespace

MockDaemonConfig::MockDaemonConfig(QObject* parent) : IDaemonConfig(parent) {
    // Believable node-side defaults across every settings section.
    m_values = {
        // Model defaults
        {QStringLiteral("model/default"), QStringLiteral("llama-3.1-8b-instruct")},
        {QStringLiteral("model/effort"), QStringLiteral("Balanced")},
        {QStringLiteral("model/fast"), false},
        // Chat
        {QStringLiteral("chat/streaming"), true},
        {QStringLiteral("chat/sendOnEnter"), true},
        {QStringLiteral("chat/showTokenCounts"), true},
        {QStringLiteral("chat/systemPrompt"), QString()},
        // Safety
        {QStringLiteral("safety/approvalPolicy"), QStringLiteral("On request")},
        {QStringLiteral("safety/sandbox"), QStringLiteral("Workspace write")},
        {QStringLiteral("safety/allowNetwork"), false},
        // Memory & context
        {QStringLiteral("memory/contextWindow"), 128000},
        {QStringLiteral("memory/autoCompact"), true},
        {QStringLiteral("memory/persistMemory"), true},
        // Workspace
        {QStringLiteral("workspace/root"), QStringLiteral("~/daemon/workspace")},
        {QStringLiteral("workspace/followGitignore"), true},
        // Voice
        {QStringLiteral("voice/enabled"), false},
        {QStringLiteral("voice/model"), QStringLiteral("whisper-small")},
        // Advanced
        {QStringLiteral("advanced/logLevel"), QStringLiteral("Info")},
        {QStringLiteral("advanced/telemetry"), false},
        {QStringLiteral("advanced/experimentalTools"), false},
    };

    m_options = {
        {QStringLiteral("model/effort"),
         {QStringLiteral("Fast"), QStringLiteral("Balanced"), QStringLiteral("Thorough")}},
        {QStringLiteral("safety/approvalPolicy"),
         {QStringLiteral("Never"), QStringLiteral("On request"), QStringLiteral("On failure"),
          QStringLiteral("Always")}},
        {QStringLiteral("safety/sandbox"),
         {QStringLiteral("Read only"), QStringLiteral("Workspace write"),
          QStringLiteral("Full access")}},
        {QStringLiteral("voice/model"),
         {QStringLiteral("whisper-tiny"), QStringLiteral("whisper-small"),
          QStringLiteral("whisper-medium")}},
        {QStringLiteral("advanced/logLevel"),
         {QStringLiteral("Error"), QStringLiteral("Warn"), QStringLiteral("Info"),
          QStringLiteral("Debug"), QStringLiteral("Trace")}},
    };

    // Overlay the last-known on-disk values over the seeded defaults (no-op on a
    // first run). Only known keys are touched; unknown cached keys are ignored.
    const QVariantHash cached = appcache::loadObject(kCacheFile).toVariantHash();
    for (auto it = cached.constBegin(); it != cached.constEnd(); ++it) {
        m_values.insert(it.key(), it.value());
    }
}

void MockDaemonConfig::save() const {
    appcache::saveObject(kCacheFile, QJsonObject::fromVariantHash(m_values));
}

QVariant MockDaemonConfig::value(const QString& key, const QVariant& fallback) const {
    return m_values.value(key, fallback);
}

void MockDaemonConfig::setValue(const QString& key, const QVariant& value) {
    if (m_values.value(key) == value) {
        return;
    }
    m_values.insert(key, value);
    save();
    emit changed(key);
}

QStringList MockDaemonConfig::options(const QString& key) const {
    return m_options.value(key);
}

} // namespace config
