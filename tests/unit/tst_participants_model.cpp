// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemonnet/mock_fleet_source.h"
#include "participants_model.h"
#include "persistence/in_memory_session_store.h"

#include <QtTest>

using daemonnet::MockFleetSource;
using participants::ParticipantsModel;
using persistence::InMemorySessionStore;

// Exercises the right sidebar's Participants section model (the GUI/TUI-shared
// ParticipantsModel): the collapsible "Participants" header, the participant rows
// with their green presence dots, and the section fold.
class TestParticipantsModel : public QObject {
    Q_OBJECT

private:
    template <typename T>
    static T roleAt(const ParticipantsModel& m, int row, ParticipantsModel::Role role) {
        return m.data(m.index(row, 0), role).template value<T>();
    }

private slots:
    // Row 0 is the collapsible section header ("Participants").
    void exposesSectionHeader() {
        MockFleetSource net;
        InMemorySessionStore store(&net);
        ParticipantsModel model;
        model.setStore(&store);

        QVERIFY(model.rowCount() >= 1);
        QVERIFY(roleAt<bool>(model, 0, ParticipantsModel::IsSeparatorRole));
        QVERIFY(roleAt<bool>(model, 0, ParticipantsModel::HasChildrenRole));
        QVERIFY(roleAt<bool>(model, 0, ParticipantsModel::ExpandedRole));
        QCOMPARE(roleAt<QString>(model, 0, ParticipantsModel::LabelRole),
                 QStringLiteral("Participants"));
    }

    // The two seeded participants ("Agent"/"User") follow the header, both with a
    // green ("available") presence dot; the agent flag distinguishes them.
    void exposesParticipantRows() {
        MockFleetSource net;
        InMemorySessionStore store(&net);
        ParticipantsModel model;
        model.setStore(&store);

        QCOMPARE(model.rowCount(), 3); // header + Agent + User

        QCOMPARE(roleAt<QString>(model, 1, ParticipantsModel::LabelRole), QStringLiteral("Agent"));
        QVERIFY(roleAt<bool>(model, 1, ParticipantsModel::IsAgentRole));
        QCOMPARE(roleAt<QString>(model, 2, ParticipantsModel::LabelRole), QStringLiteral("User"));
        QVERIFY(!roleAt<bool>(model, 2, ParticipantsModel::IsAgentRole));

        for (int row = 1; row <= 2; ++row) {
            QVERIFY(!roleAt<bool>(model, row, ParticipantsModel::IsSeparatorRole));
            QCOMPARE(roleAt<QString>(model, row, ParticipantsModel::PresenceRole),
                     QStringLiteral("available"));
            // Green dot color carried straight to the GUI/TUI renderers.
            QCOMPARE(roleAt<QString>(model, row, ParticipantsModel::ColorRole),
                     QStringLiteral("#3fb950"));
        }
    }

    // Folding the header hides the participant rows; unfolding restores them.
    void headerCollapsesAndExpands() {
        MockFleetSource net;
        InMemorySessionStore store(&net);
        ParticipantsModel model;
        model.setStore(&store);
        QCOMPARE(model.rowCount(), 3);

        model.toggleExpand(0);
        QCOMPARE(model.rowCount(), 1); // only the header remains
        QVERIFY(!roleAt<bool>(model, 0, ParticipantsModel::ExpandedRole));

        model.toggleExpand(0);
        QCOMPARE(model.rowCount(), 3);
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
