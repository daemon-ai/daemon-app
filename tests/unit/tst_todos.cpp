#include "todo_list_model.h"

#include <QSignalSpy>
#include <QVariantList>
#include <QVariantMap>
#include <QtTest>

// Exercises the shared TodoListModel (the C++ port of the composer's QML demo
// seam): role exposure, setTodos replacement semantics, count signalling, and
// clear. Both front ends render this model, so the roles must stay stable.
class TestTodos : public QObject {
    Q_OBJECT

private slots:
    void rolesExposeTextAndDone()
    {
        TodoListModel model;
        model.setTodos(QVariantList{
            QVariantMap{ { QStringLiteral("text"), QStringLiteral("a") },
                         { QStringLiteral("done"), true } },
            QVariantMap{ { QStringLiteral("text"), QStringLiteral("b") },
                         { QStringLiteral("done"), false } },
        });

        QCOMPARE(model.count(), 2);
        QCOMPARE(model.rowCount(), 2);

        const QModelIndex i0 = model.index(0, 0);
        QCOMPARE(model.data(i0, TodoListModel::TextRole).toString(), QStringLiteral("a"));
        QCOMPARE(model.data(i0, TodoListModel::DoneRole).toBool(), true);

        const QModelIndex i1 = model.index(1, 0);
        QCOMPARE(model.data(i1, TodoListModel::DoneRole).toBool(), false);

        const QHash<int, QByteArray> roles = model.roleNames();
        QCOMPARE(roles.value(TodoListModel::TextRole), QByteArrayLiteral("text"));
        QCOMPARE(roles.value(TodoListModel::DoneRole), QByteArrayLiteral("done"));
    }

    void setTodosReplacesAndSignals()
    {
        TodoListModel model;
        QSignalSpy countSpy(&model, &TodoListModel::countChanged);

        model.setTodos(QVariantList{ QVariantMap{ { QStringLiteral("text"),
                                                    QStringLiteral("one") } } });
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.textAt(0), QStringLiteral("one"));
        QVERIFY(!model.doneAt(0)); // defaulted

        // A second setTodos fully replaces the contents.
        model.setTodos(QVariantList{
            QVariantMap{ { QStringLiteral("text"), QStringLiteral("x") } },
            QVariantMap{ { QStringLiteral("text"), QStringLiteral("y") } },
        });
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.textAt(1), QStringLiteral("y"));
        QVERIFY(countSpy.count() >= 2);
    }

    void clearEmptiesTheModel()
    {
        TodoListModel model;
        model.setTodos(QVariantList{ QVariantMap{ { QStringLiteral("text"),
                                                    QStringLiteral("z") } } });
        QCOMPARE(model.count(), 1);

        QSignalSpy countSpy(&model, &TodoListModel::countChanged);
        model.clear();
        QCOMPARE(model.count(), 0);
        QCOMPARE(countSpy.count(), 1);

        // Clearing an already-empty model is a no-op (no extra signal).
        model.clear();
        QCOMPARE(countSpy.count(), 1);
    }
};

QTEST_GUILESS_MAIN(TestTodos)
#include "tst_todos.moc"
