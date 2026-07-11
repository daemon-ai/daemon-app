// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/mirror_session_store.h"
#include "mirror/mirror_service.h"
#include "participants_model.h"

#include <QtTest>

using participants::ParticipantsModel;

// Exercises the right sidebar's Participants section model (the GUI/TUI-shared
// ParticipantsModel) over the production store (AD: the InMemory fixture died with the legacy
// stores). ISessionStore::participants() is empty on every production store since A8 aligned
// mock to daemon (the mock-only seeded rows were a shape fork), so the section is the header
// alone — the model must render that truthfully and stay fold/no-store safe. (The row/presence
// rendering paths keep their coverage epitaph in LEDGER-ad: they become live again only if a
// participants vertical lands on the mirror.)
class TestParticipantsModel : public QObject {
    Q_OBJECT

private:
    template <typename T>
    static T roleAt(const ParticipantsModel& m, int row, ParticipantsModel::Role role) {
        return m.data(m.index(row, 0), role).template value<T>();
    }

private slots:
    // Row 0 is the collapsible section header ("Participants"); the production store carries no
    // participant rows (the aligned mock==daemon degradation), so the header is all there is.
    void exposesHeaderOnlyOverProductionStore() {
        mirror::MirrorService svc;
        svc.openInMemory();
        daemonapp::daemon::MirrorSessionStore store(&svc.store(), &svc.ingestor());
        ParticipantsModel model;
        model.setStore(&store);

        QCOMPARE(model.rowCount(), 1);
        QVERIFY(roleAt<bool>(model, 0, ParticipantsModel::IsSeparatorRole));
        QCOMPARE(roleAt<QString>(model, 0, ParticipantsModel::LabelRole),
                 QStringLiteral("Participants"));
    }

    // Folding the (empty) header stays safe and reversible.
    void headerCollapsesAndExpands() {
        mirror::MirrorService svc;
        svc.openInMemory();
        daemonapp::daemon::MirrorSessionStore store(&svc.store(), &svc.ingestor());
        ParticipantsModel model;
        model.setStore(&store);
        QCOMPARE(model.rowCount(), 1);

        model.toggleExpand(0);
        QVERIFY(!roleAt<bool>(model, 0, ParticipantsModel::ExpandedRole));
        model.toggleExpand(0);
        QVERIFY(roleAt<bool>(model, 0, ParticipantsModel::ExpandedRole));
    }

    // With no store bound the section is just the (empty) header - no crash.
    void emptyWithoutStore() {
        ParticipantsModel model;
        QCOMPARE(model.rowCount(), 1);
        QVERIFY(roleAt<bool>(model, 0, ParticipantsModel::IsSeparatorRole));
    }
};

QTEST_MAIN(TestParticipantsModel)
#include "tst_participants_model.moc"
