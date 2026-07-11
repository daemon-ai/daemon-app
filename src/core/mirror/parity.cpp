#include "mirror/parity.h"

#include <QLoggingCategory>

namespace mirror::parity {

namespace {
Q_LOGGING_CATEGORY(lcParity, "daemon.mirror.parity")
} // namespace

Result compareKeys(const QSet<QString>& mirrorKeys, const QSet<QString>& legacyKeys) {
    Result r;
    for (const QString& k : mirrorKeys) {
        if (!legacyKeys.contains(k)) {
            r.onlyInMirror << k;
        }
    }
    for (const QString& k : legacyKeys) {
        if (!mirrorKeys.contains(k)) {
            r.onlyInLegacy << k;
        }
    }
    r.onlyInMirror.sort();
    r.onlyInLegacy.sort();
    return r;
}

QSet<QString> conversationKeys(const MirrorModel& snapshot, const QString& transport) {
    QSet<QString> out;
    if (snapshot.conversationsByTransport.count(transport)) {
        for (const ConversationKey& k : snapshot.conversationsByTransport[transport]) {
            out.insert(k.serialize());
        }
    }
    return out;
}

QSet<QString> contactKeys(const MirrorModel& snapshot, const QString& transport) {
    QSet<QString> out;
    for (const Contact& c : snapshot.contacts) {
        if (c.transport == transport) {
            out.insert(c.key().serialize());
        }
    }
    return out;
}

QSet<QString> sessionKeys(const MirrorModel& snapshot) {
    QSet<QString> out;
    for (const Session& s : snapshot.sessions) {
        out.insert(s.key().serialize());
    }
    return out;
}

QSet<QString> fleetUnitKeys(const MirrorModel& snapshot) {
    QSet<QString> out;
    for (const FleetUnit& u : snapshot.fleet_units) {
        out.insert(u.key().serialize());
    }
    return out;
}

bool checkAndLog(const QString& domain, const Result& result) {
    if (result.matches()) {
        return true;
    }
    qCWarning(lcParity).noquote() << "dual-write parity MISMATCH for" << domain
                                  << "| only-in-mirror:"
                                  << result.onlyInMirror.join(QLatin1Char(','))
                                  << "| only-in-legacy:"
                                  << result.onlyInLegacy.join(QLatin1Char(','));
    return false;
}

} // namespace mirror::parity
