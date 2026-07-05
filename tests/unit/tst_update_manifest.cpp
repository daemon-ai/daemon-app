// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/semver.h"
#include "update/update_manifest.h"

#include <QtTest>

using namespace update;

// The SemVer monotonicity comparator (downgrade-replay guard) and the schema-1
// manifest parser + artifact selection (packaging/UPDATES.md).
class UpdateManifestTests : public QObject {
    Q_OBJECT

private slots:
    void semverOrdering();
    void semverBuildMetadataStripped();
    void semverPrerelease();
    void semverUpgradeGate();
    void parseRejectsWrongSchema();
    void parseRejectsWrongProductOrChannel();
    void parseValidManifest();
    void selectionMatchesKindOsArch();
    void selectionSkipsUnknownArch();
    void selectionEnforcesGlibcFloor();
    void selectionNoMatch();
};

void UpdateManifestTests::semverOrdering() {
    QCOMPARE(semver::compare(QStringLiteral("1.10.0"), QStringLiteral("1.9.0")), 1);
    QCOMPARE(semver::compare(QStringLiteral("1.9.0"), QStringLiteral("1.10.0")), -1);
    QCOMPARE(semver::compare(QStringLiteral("2.0.0"), QStringLiteral("1.999.999")), 1);
    QCOMPARE(semver::compare(QStringLiteral("1.2.3"), QStringLiteral("1.2.3")), 0);
    QCOMPARE(semver::compare(QStringLiteral("0.2.0"), QStringLiteral("0.1.0")), 1);
}

void UpdateManifestTests::semverBuildMetadataStripped() {
    // The running build carries a git suffix; it must not affect precedence.
    QCOMPARE(semver::compare(QStringLiteral("0.1.0+42.gdeadbee"), QStringLiteral("0.1.0")), 0);
    QCOMPARE(semver::compare(QStringLiteral("0.2.0"), QStringLiteral("0.1.0+9.gcafe.dirty")), 1);
    QCOMPARE(semver::stripBuildMetadata(QStringLiteral("v1.2.3+meta")), QStringLiteral("1.2.3"));
}

void UpdateManifestTests::semverPrerelease() {
    // A prerelease ranks below its release core.
    QCOMPARE(semver::compare(QStringLiteral("1.0.0-rc.1"), QStringLiteral("1.0.0")), -1);
    QCOMPARE(semver::compare(QStringLiteral("1.0.0"), QStringLiteral("1.0.0-rc.1")), 1);
    // Prerelease identifiers compare left-to-right; numeric < larger numeric.
    QCOMPARE(semver::compare(QStringLiteral("1.0.0-alpha.1"), QStringLiteral("1.0.0-alpha.2")), -1);
    // Numeric identifiers rank below alphanumeric ones.
    QCOMPARE(semver::compare(QStringLiteral("1.0.0-1"), QStringLiteral("1.0.0-alpha")), -1);
    // A longer identifier set outranks its prefix.
    QCOMPARE(semver::compare(QStringLiteral("1.0.0-alpha"), QStringLiteral("1.0.0-alpha.1")), -1);
}

void UpdateManifestTests::semverUpgradeGate() {
    QVERIFY(semver::isUpgrade(QStringLiteral("0.2.0"), QStringLiteral("0.1.0")));
    QVERIFY(!semver::isUpgrade(QStringLiteral("0.1.0"), QStringLiteral("0.1.0"))); // equal
    QVERIFY(!semver::isUpgrade(QStringLiteral("0.1.0"), QStringLiteral("0.2.0"))); // downgrade
    // Replay of the running build (with its git suffix) is not an upgrade.
    QVERIFY(!semver::isUpgrade(QStringLiteral("0.1.0"), QStringLiteral("0.1.0+7.gabc")));
}

namespace {
QByteArray sampleManifest(const QString& version = QStringLiteral("0.2.0"),
                          const QString& channel = QStringLiteral("stable")) {
    return QStringLiteral(R"({
      "schema": 1,
      "product": "daemon",
      "channel": "%1",
      "version": "%2",
      "released": "2026-07-03T21:58:12Z",
      "notesUrl": "https://example/releases/%2",
      "artifacts": [
        {"kind":"appimage","os":"linux","arch":"x86_64","file":"daemon-app-%2-x86_64.AppImage",
         "size":133742048,"sha256":"deadbeef","glibcFloor":"2.28","updateCapability":"SelfApply"},
        {"kind":"nsis","os":"windows","arch":"x86_64","file":"daemon-%2-setup.exe",
         "size":90000000,"sha256":"cafef00d","updateCapability":"DownloadAndOpen"},
        {"kind":"dmg","os":"macos","arch":"unknown","file":"daemon-%2.dmg",
         "size":80000000,"sha256":"abad1dea","updateCapability":"DownloadAndOpen"}
      ]
    })")
        .arg(channel, version)
        .toUtf8();
}
} // namespace

void UpdateManifestTests::parseRejectsWrongSchema() {
    const QByteArray json = QByteArrayLiteral(
        R"({"schema":2,"product":"daemon","channel":"stable","version":"9.0.0","artifacts":[]})");
    const ManifestParseResult r =
        parseManifest(json, QStringLiteral("daemon"), QStringLiteral("stable"));
    QVERIFY(!r.ok);
    QVERIFY(r.error.contains(QStringLiteral("schema")));
}

void UpdateManifestTests::parseRejectsWrongProductOrChannel() {
    const ManifestParseResult wrongProduct =
        parseManifest(sampleManifest(), QStringLiteral("other"), QStringLiteral("stable"));
    QVERIFY(!wrongProduct.ok);
    const ManifestParseResult wrongChannel =
        parseManifest(sampleManifest(), QStringLiteral("daemon"), QStringLiteral("beta"));
    QVERIFY(!wrongChannel.ok);
}

void UpdateManifestTests::parseValidManifest() {
    const ManifestParseResult r =
        parseManifest(sampleManifest(), QStringLiteral("daemon"), QStringLiteral("stable"));
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(r.manifest.version, QStringLiteral("0.2.0"));
    QCOMPARE(r.manifest.artifacts.size(), 3);
    QCOMPARE(r.manifest.artifacts.at(0).size, static_cast<qint64>(133742048));
    QCOMPARE(r.manifest.artifacts.at(0).glibcFloor, QStringLiteral("2.28"));
}

void UpdateManifestTests::selectionMatchesKindOsArch() {
    const Manifest m =
        parseManifest(sampleManifest(), QStringLiteral("daemon"), QStringLiteral("stable"))
            .manifest;
    const SelectionResult r =
        selectArtifact(m, {QStringLiteral("appimage"), QStringLiteral("linux"),
                           QStringLiteral("x86_64"), QStringLiteral("2.38")});
    QVERIFY(r.matched);
    QCOMPARE(r.artifact.kind, QStringLiteral("appimage"));
    QCOMPARE(r.artifact.updateCapability, QStringLiteral("SelfApply"));
}

void UpdateManifestTests::selectionSkipsUnknownArch() {
    const Manifest m =
        parseManifest(sampleManifest(), QStringLiteral("daemon"), QStringLiteral("stable"))
            .manifest;
    // The dmg row has arch=unknown and must never be auto-selected.
    const SelectionResult r = selectArtifact(
        m, {QStringLiteral("dmg"), QStringLiteral("macos"), QStringLiteral("unknown"), QString()});
    QVERIFY(!r.matched);
}

void UpdateManifestTests::selectionEnforcesGlibcFloor() {
    const Manifest m =
        parseManifest(sampleManifest(), QStringLiteral("daemon"), QStringLiteral("stable"))
            .manifest;
    // Host glibc below the artifact floor (2.28) -> skipped.
    const SelectionResult tooOld =
        selectArtifact(m, {QStringLiteral("appimage"), QStringLiteral("linux"),
                           QStringLiteral("x86_64"), QStringLiteral("2.17")});
    QVERIFY(!tooOld.matched);
    // Unknown host glibc -> floor check is skipped (cannot prove incompatible).
    const SelectionResult unknownHost =
        selectArtifact(m, {QStringLiteral("appimage"), QStringLiteral("linux"),
                           QStringLiteral("x86_64"), QString()});
    QVERIFY(unknownHost.matched);
}

void UpdateManifestTests::selectionNoMatch() {
    const Manifest m =
        parseManifest(sampleManifest(), QStringLiteral("daemon"), QStringLiteral("stable"))
            .manifest;
    // No deb row exists -> Notify-only degradation upstream.
    const SelectionResult r = selectArtifact(m, {QStringLiteral("deb"), QStringLiteral("linux"),
                                                 QStringLiteral("x86_64"), QStringLiteral("2.38")});
    QVERIFY(!r.matched);
    QVERIFY(!r.reason.isEmpty());
}

QTEST_MAIN(UpdateManifestTests)
#include "tst_update_manifest.moc"
