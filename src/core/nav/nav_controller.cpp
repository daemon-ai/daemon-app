// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "nav/nav_controller.h"

namespace nav {

void NavController::open(const QString& page, const QString& section) {
    const bool pageChanged = (m_page != page) || (m_section != section);
    m_page = page;
    m_section = section;
    if (pageChanged) {
        emit changed();
    }
    emit openRequested(page, section);
}

void NavController::setSection(const QString& section) {
    if (m_page.isEmpty() || m_section == section) {
        return;
    }
    m_section = section;
    emit changed();
    emit openRequested(m_page, m_section);
}

void NavController::openAgent(const QString& kind, const QString& profileRef,
                              const QString& title) {
    if (profileRef.isEmpty()) {
        return;
    }
    emit openAgentRequested(kind, profileRef, title);
}

void NavController::close() {
    if (m_page.isEmpty()) {
        return;
    }
    m_page.clear();
    m_section.clear();
    emit changed();
    emit closeRequested();
}

} // namespace nav
