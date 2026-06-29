// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "subagent_model.h"

#include <QSignalSpy>
#include <QtTest>
#include <QVariantList>
#include <QVariantMap>

namespace {

QVariantMap sub(const QString& id, const QString& title, const QString& status,
                const QString& detail) {
    return QVariantMap{
        {QStringLiteral("type"), QStringLiteral("subagent")},
        {QStringLiteral("id"), id},
        {QStringLiteral("title"), title},
        {QStringLiteral("status"), status},
        {QStringLiteral("detail"), detail},
    };
}

} // namespace

// Exercises the shared SubagentModel (live delegation rows for the composer
// status stack): it upserts "subagent" events keyed by id, ignores unrelated
// event types, tracks the running count, and clears. Both front ends render this
// model, so the roles and upsert semantics must stay stable.
class TestSubagentModel : public QObject {
    Q_OBJECT

private slots:
    void rolesExposeTitleStatusDetail() {
        SubagentModel model;
        model.applyEvents(QVariantList{sub(QStringLiteral("a"), QStringLiteral("explore"),
                                           QStringLiteral("running"), QStringLiteral("scanning"))});

        QCOMPARE(model.count(), 1);
        const QModelIndex i0 = model.index(0, 0);
        QCOMPARE(model.data(i0, SubagentModel::TitleRole).toString(), QStringLiteral("explore"));
        QCOMPARE(model.data(i0, SubagentModel::StatusRole).toString(), QStringLiteral("running"));
        QCOMPARE(model.data(i0, SubagentModel::DetailRole).toString(), QStringLiteral("scanning"));

        const QHash<int, QByteArray> roles = model.roleNames();
        QCOMPARE(roles.value(SubagentModel::TitleRole), QByteArrayLiteral("title"));
        QCOMPARE(roles.value(SubagentModel::StatusRole), QByteArrayLiteral("status"));
    }

    void upsertUpdatesExistingRowByIdNotAppend() {
        SubagentModel model;
        model.applyEvents(QVariantList{sub(QStringLiteral("a"), QStringLiteral("explore"),
                                           QStringLiteral("running"), QStringLiteral("0%"))});
        QCOMPARE(model.count(), 1);

        // Same id -> in-place update (running count drops to zero, no new row).
        model.applyEvents(QVariantList{sub(QStringLiteral("a"), QString(), QStringLiteral("done"),
                                           QStringLiteral("42 files"))});
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.statusAt(0), QStringLiteral("done"));
        QCOMPARE(model.detailAt(0), QStringLiteral("42 files"));
        QCOMPARE(model.titleAt(0), QStringLiteral("explore")); // empty title preserved
        QCOMPARE(model.runningCount(), 0);

        // A new id appends a second row.
        model.applyEvents(QVariantList{sub(QStringLiteral("b"), QStringLiteral("tests"),
                                           QStringLiteral("running"), QString())});
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.runningCount(), 1);
    }

    // runningCount / failedCount classify rows by status (fed to the footer's
    // agentsRunning/agentsFailed in both front ends).
    void runningAndFailedCounts() {
        SubagentModel model;
        model.applyEvents(QVariantList{
            sub(QStringLiteral("a"), QStringLiteral("explore"), QStringLiteral("running"),
                QString()),
            sub(QStringLiteral("b"), QStringLiteral("tests"), QStringLiteral("running"), QString()),
            sub(QStringLiteral("c"), QStringLiteral("build"), QStringLiteral("error"), QString()),
            sub(QStringLiteral("d"), QStringLiteral("lint"), QStringLiteral("done"), QString()),
        });
        QCOMPARE(model.runningCount(), 2);
        QCOMPARE(model.failedCount(), 1);

        // A status transition reclassifies without changing the row count.
        model.applyEvents(
            QVariantList{sub(QStringLiteral("a"), QString(), QStringLiteral("error"), QString())});
        QCOMPARE(model.count(), 4);
        QCOMPARE(model.runningCount(), 1);
        QCOMPARE(model.failedCount(), 2);
    }

    void ignoresNonSubagentEvents() {
        SubagentModel model;
        model.applyEvents(QVariantList{
            QVariantMap{{QStringLiteral("type"), QStringLiteral("usage")}},
            QVariantMap{{QStringLiteral("type"), QStringLiteral("text")},
                        {QStringLiteral("text"), QStringLiteral("hi")}},
        });
        QCOMPARE(model.count(), 0);
    }

    void clearEmptiesAndSignals() {
        SubagentModel model;
        model.applyEvents(QVariantList{sub(QStringLiteral("a"), QStringLiteral("explore"),
                                           QStringLiteral("running"), QString())});
        QCOMPARE(model.count(), 1);

        QSignalSpy countSpy(&model, &SubagentModel::countChanged);
        model.clear();
        QCOMPARE(model.count(), 0);
        QCOMPARE(countSpy.count(), 1);

        // Clearing an already-empty model is a no-op (no extra signal).
        model.clear();
        QCOMPARE(countSpy.count(), 1);
    }
};

QTEST_GUILESS_MAIN(TestSubagentModel)
#include "tst_subagent_model.moc"
