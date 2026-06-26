#include "memory/imemory_service.h"

namespace memory {

void IMemoryService::setScope(const QString& profile, const QString& session, bool includeGlobal) {
    if (m_profile == profile && m_session == session && m_includeGlobal == includeGlobal) {
        return;
    }
    m_profile = profile;
    m_session = session;
    m_includeGlobal = includeGlobal;
    emit scopeChanged();
}

void registerMemoryMetatypes() {
    qRegisterMetaType<memory::MemoryEntry>("memory::MemoryEntry");
    qRegisterMetaType<QList<memory::MemoryEntry>>("QList<memory::MemoryEntry>");
    qRegisterMetaType<memory::Bucket>("memory::Bucket");
    qRegisterMetaType<memory::MemoryStats>("memory::MemoryStats");
    qRegisterMetaType<memory::MemoryNode>("memory::MemoryNode");
    qRegisterMetaType<memory::MemoryEdge>("memory::MemoryEdge");
    qRegisterMetaType<memory::MemoryCluster>("memory::MemoryCluster");
    qRegisterMetaType<memory::MemoryGraph>("memory::MemoryGraph");
    qRegisterMetaType<memory::MemoryEvent>("memory::MemoryEvent");
    qRegisterMetaType<memory::MemoryTimelineGroup>("memory::MemoryTimelineGroup");
    qRegisterMetaType<QList<memory::MemoryTimelineGroup>>("QList<memory::MemoryTimelineGroup>");
}

} // namespace memory
