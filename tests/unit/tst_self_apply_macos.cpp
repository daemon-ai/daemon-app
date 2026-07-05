// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Pure-logic coverage for the macOS SelfApply guard decision + path predicates
// (src/core/update/self_apply_macos.{h,cpp}). These are compiled and run on the
// Linux CI host: the guard verdict and the path classifiers carry no macOS
// syscalls, so they are the testable core of the "decide SelfApply vs
// DownloadAndOpen BEFORE mutating anything" contract.

#include "update/self_apply_macos.h"

#include <QtTest/QtTest>

using update::macos::evaluateGuards;
using update::macos::GuardFacts;
using update::macos::GuardVerdict;
using update::macos::pathIsTranslocated;
using update::macos::pathUnderVolumes;

namespace {

// A fact set where every guard passes; the tests flip one field at a time.
GuardFacts allClear() {
    GuardFacts f;
    f.bundleParentWritable = true;
    f.sameFilesystem = true;
    f.translocated = false;
    f.readOnlyVolume = false;
    return f;
}

} // namespace

class TestSelfApplyMacos : public QObject {
    Q_OBJECT

private slots:
    void allGuardsPass_selfApplies() {
        const GuardVerdict v = evaluateGuards(allClear());
        QVERIFY(v.selfApply);
        QVERIFY(v.reason.isEmpty());
    }

    void translocated_fallsBack() {
        GuardFacts f = allClear();
        f.translocated = true;
        const GuardVerdict v = evaluateGuards(f);
        QVERIFY(!v.selfApply);
        QVERIFY(v.reason.contains(QStringLiteral("translocated"), Qt::CaseInsensitive));
    }

    void readOnlyVolume_fallsBack() {
        GuardFacts f = allClear();
        f.readOnlyVolume = true;
        const GuardVerdict v = evaluateGuards(f);
        QVERIFY(!v.selfApply);
        QVERIFY(v.reason.contains(QStringLiteral("read-only"), Qt::CaseInsensitive));
    }

    void unwritableParent_fallsBack() {
        GuardFacts f = allClear();
        f.bundleParentWritable = false;
        const GuardVerdict v = evaluateGuards(f);
        QVERIFY(!v.selfApply);
        QVERIFY(!v.reason.isEmpty());
    }

    void crossFilesystem_fallsBack() {
        GuardFacts f = allClear();
        f.sameFilesystem = false;
        const GuardVerdict v = evaluateGuards(f);
        QVERIFY(!v.selfApply);
        QVERIFY(v.reason.contains(QStringLiteral("filesystem"), Qt::CaseInsensitive));
    }

    // Translocation is the most severe condition: it must win even when other
    // guards would also trip, so the user gets the actionable "move to
    // Applications" message rather than a generic one.
    void translocationTakesPrecedence() {
        GuardFacts f = allClear();
        f.translocated = true;
        f.readOnlyVolume = true;
        f.bundleParentWritable = false;
        const GuardVerdict v = evaluateGuards(f);
        QVERIFY(!v.selfApply);
        QVERIFY(v.reason.contains(QStringLiteral("translocated"), Qt::CaseInsensitive));
    }

    void pathIsTranslocated_classifies() {
        QVERIFY(pathIsTranslocated(QStringLiteral(
            "/private/var/folders/x7/abc/T/AppTranslocation/1234-UUID/d/daemon-app.app")));
        QVERIFY(!pathIsTranslocated(QStringLiteral("/Applications/daemon-app.app")));
        QVERIFY(!pathIsTranslocated(QStringLiteral("/Users/j/Applications/daemon-app.app")));
    }

    void pathUnderVolumes_classifies() {
        QVERIFY(pathUnderVolumes(QStringLiteral("/Volumes/daemon-app 0.9.1/daemon-app.app")));
        QVERIFY(!pathUnderVolumes(QStringLiteral("/Applications/daemon-app.app")));
        QVERIFY(!pathUnderVolumes(QStringLiteral("/Users/j/Applications/daemon-app.app")));
    }
};

QTEST_MAIN(TestSelfApplyMacos)
#include "tst_self_apply_macos.moc"
