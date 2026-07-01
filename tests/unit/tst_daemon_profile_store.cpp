// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_accounts_service.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_profile_store.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "uimodels/variant_list_model.h"
#include "wire_mux_fixture.h"

#include <memory>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <QUrl>

using daemonapp::daemon::DecodedDistribution;
using daemonapp::daemon::DecodedRevision;
using daemonapp::daemon::DecodedSkillBundle;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::daemon::ProfileRepository;
using daemonapp::test::cborText;
using daemonapp::test::cborUint;
using daemonapp::test::WireMuxServer;
using profiles::DaemonProfileStore;

namespace {

// A small but complete sample distribution (profile + one bundled skill file).
DecodedDistribution sampleDist() {
    DecodedDistribution d;
    d.wireVersion = 1;
    d.profile.id = QStringLiteral("work");
    d.profile.provider = QStringLiteral("genai");
    d.profile.model = QStringLiteral("claude");
    d.profile.systemPrompt = QStringLiteral("be helpful");
    DecodedSkillBundle skill;
    skill.name = QStringLiteral("notes");
    skill.category = QStringLiteral("writing");
    skill.files.insert(QStringLiteral("SKILL.md"), QStringLiteral("# notes"));
    d.skills.append(skill);
    return d;
}

QByteArray encodeResponse(const api_response_r& resp) {
    QByteArray out(64 * 1024, Qt::Uninitialized);
    size_t written = 0;
    const int rc = cbor_encode_api_response(reinterpret_cast<uint8_t*>(out.data()),
                                            static_cast<size_t>(out.size()), &resp, &written);
    if (rc != ZCBOR_SUCCESS) {
        return {};
    }
    out.resize(static_cast<qsizetype>(written));
    return out;
}

// Build a Distribution response by reusing the production import encoder (which fully populates a
// distribution from a DecodedDistribution), decoding that request, and re-wrapping the recovered
// distribution as a Distribution response. `req` must outlive the encode (the struct borrows it).
QByteArray distributionResponse(const DecodedDistribution& d) {
    const QByteArray req = NodeApiCodec::encodeProfileImportRequest(d, QString());
    auto reqStruct = std::make_unique<api_request_r>();
    size_t consumed = 0;
    if (cbor_decode_api_request(reinterpret_cast<const uint8_t*>(req.constData()),
                                static_cast<size_t>(req.size()), reqStruct.get(),
                                &consumed) != ZCBOR_SUCCESS) {
        return {};
    }
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_distribution_m_c;
    resp->api_response_response_distribution_m.response_distribution_Distribution =
        reqStruct->api_request_request_profile_import_m.ProfileImport_dist;
    return encodeResponse(*resp);
}

// {"Revisions":[ {seq:1, operator, "init"}, {seq:2, parent:1, agent "bot", "tuned"} ]}
QByteArray revisionsResponse() {
    const QByteArray reason0 = QByteArrayLiteral("init");
    const QByteArray reason1 = QByteArrayLiteral("tuned");
    const QByteArray agent1 = QByteArrayLiteral("bot");
    const auto setZ = [](zcbor_string& z, const QByteArray& b) {
        z.value = reinterpret_cast<const uint8_t*>(b.constData());
        z.len = static_cast<size_t>(b.size());
    };
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_revisions_m_c;
    response_revisions& rr = resp->api_response_response_revisions_m;
    rr.response_revisions_Revisions_revision_m_count = 2;
    revision& r0 = rr.response_revisions_Revisions_revision_m[0];
    r0.revision_seq = 1;
    r0.revision_parent_choice = revision::revision_parent_null_m_c;
    r0.revision_content_hash.content_hash_uint_count = 0;
    r0.revision_author.author_choice = author_r::author_operator_tstr_c;
    setZ(r0.revision_reason, reason0);
    r0.revision_ts_ms = 100;
    revision& r1 = rr.response_revisions_Revisions_revision_m[1];
    r1.revision_seq = 2;
    r1.revision_parent_choice = revision::revision_parent_uint64_m_c;
    r1.revision_parent_uint64_m = 1;
    r1.revision_content_hash.content_hash_uint_count = 1;
    r1.revision_content_hash.content_hash_uint[0] = 0xAA;
    r1.revision_author.author_choice = author_r::author_agent_m_c;
    setZ(r1.revision_author.author_agent_m.author_agent_agent, agent1);
    setZ(r1.revision_reason, reason1);
    r1.revision_ts_ms = 200;
    return encodeResponse(*resp);
}

QByteArray profileIdResponse(const char* id) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "ProfileId");
    cborText(b, id);
    return b;
}

QByteArray okResponse() {
    QByteArray b;
    cborText(b, "Ok");
    return b;
}

QString zToQString(const zcbor_string& z) {
    return QString::fromUtf8(reinterpret_cast<const char*>(z.value), static_cast<int>(z.len));
}

void setZstr(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

// A CredentialList response: profile "work" has a key (present + hint), "other" does not. Built via
// the generated encoder so the CBOR shape matches the decoder exactly.
QByteArray credentialsResponse() {
    const QByteArray p0 = QByteArrayLiteral("work");
    const QByteArray h0 = QByteArrayLiteral("sk-live-...abcd");
    const QByteArray p1 = QByteArrayLiteral("other");
    const QByteArray h1 = QByteArrayLiteral("");
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_credentials_m_c;
    response_credentials& rc = resp->api_response_response_credentials_m;
    rc.response_credentials_Credentials_credential_info_m_count = 2;
    credential_info& c0 = rc.response_credentials_Credentials_credential_info_m[0];
    setZstr(c0.credential_info_profile, p0);
    c0.credential_info_present = true;
    setZstr(c0.credential_info_hint, h0);
    credential_info& c1 = rc.response_credentials_Credentials_credential_info_m[1];
    setZstr(c1.credential_info_profile, p1);
    c1.credential_info_present = false;
    setZstr(c1.credential_info_hint, h1);
    return encodeResponse(*resp);
}

// {"Error":{"Unsupported":"no versioning"}} - the no-revision-log daemon's answer. (The message is
// kept under 24 bytes so the fixture's single-byte tstr header encodes it correctly.)
QByteArray unsupportedResponse() {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "Error");
    b.append(static_cast<char>(0xA1));
    cborText(b, "Unsupported");
    cborText(b, "no versioning");
    return b;
}

// A server Hello frame advertising the given capability strings.
QByteArray helloFrame(const QList<const char*>& features) {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborText(b, "Hello");
    b.append(static_cast<char>(0xA2)); // map(2): wire_version, features
    cborText(b, "wire_version");
    cborUint(b, 1);
    cborText(b, "features");
    b.append(static_cast<char>(0x80 | static_cast<char>(features.size())));
    for (const char* f : features) {
        cborText(b, f);
    }
    return b;
}

} // namespace

// Unit coverage for the client-side logic in DaemonProfileStore that is not exercised by the wire
// gates: the id-derivation (slug) for create/clone. The encode byte-correctness is covered by the
// daemon-node verify-codec fixtures, and create/update/clone/get against a real daemon by the
// system-tests; here we only need an (unconnected) repository so the synchronous return value is
// observable.
class TestDaemonProfileStore : public QObject {
    Q_OBJECT

private slots:
    void createSlugsTheNameIntoAnId() {
        daemonapp::daemon::DaemonTransport transport;
        NodeApiClient client(&transport);
        ProfileRepository repo(&client, nullptr);
        DaemonProfileStore store(&repo);

        QCOMPARE(store.createProfile(QStringLiteral("My Work Agent")),
                 QStringLiteral("my-work-agent"));
        QCOMPARE(store.createProfile(QStringLiteral("Coder/Reviewer 2")),
                 QStringLiteral("coder-reviewer-2"));
        // Empty / punctuation-only names fall back to a stable default rather than an empty id.
        QCOMPARE(store.createProfile(QString()), QStringLiteral("profile"));
        QCOMPARE(store.createProfile(QStringLiteral("  --  ")), QStringLiteral("profile"));
    }

    void cloneSlugsTheNewId() {
        daemonapp::daemon::DaemonTransport transport;
        NodeApiClient client(&transport);
        ProfileRepository repo(&client, nullptr);
        DaemonProfileStore store(&repo);

        QCOMPARE(store.cloneProfile(QStringLiteral("default"), QStringLiteral("Work Copy")),
                 QStringLiteral("work-copy"));
        // An empty target id is rejected (no clone issued).
        QVERIFY(store.cloneProfile(QStringLiteral("default"), QString()).isEmpty());
    }

    void exposesAProfilesModel() {
        daemonapp::daemon::DaemonTransport transport;
        NodeApiClient client(&transport);
        ProfileRepository repo(&client, nullptr);
        DaemonProfileStore store(&repo);

        auto* model = qobject_cast<uimodels::VariantListModel*>(store.profiles());
        QVERIFY(model != nullptr);
        QCOMPARE(model->count(), 0); // empty until a ProfileList round-trips
    }

    // Offline-first: a profile persisted to the cache (as a prior online session would have, via
    // ProfileGet) renders in the Profiles UI with NO daemon connection - the store seeds from the
    // cache in its ctor. This is the offline-read guarantee for agents.
    void rendersCachedProfilesOffline() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        daemonapp::daemon::DaemonCacheStore cache(dir.filePath(QStringLiteral("profiles.db")));
        QVERIFY(cache.isOpen());

        daemonapp::daemon::CachedProfileRow row;
        row.profileRef = QStringLiteral("work");
        row.displayName = QStringLiteral("work");
        // spec_cbor is the raw ProfileGet response in production; a CBOR null here is enough to
        // exercise the offline-seed path (the row still renders id + active; decode is
        // best-effort).
        row.specCbor = QByteArrayLiteral("\xF6");
        row.active = true;
        row.updatedAtMs = 1;
        QVERIFY(cache.upsertProfile(row));

        // An unconnected client (no daemon): the store must still render the cached agent.
        daemonapp::daemon::DaemonTransport transport;
        NodeApiClient client(&transport);
        ProfileRepository repo(&client, &cache);
        DaemonProfileStore store(&repo);

        auto* model = qobject_cast<uimodels::VariantListModel*>(store.profiles());
        QVERIFY(model != nullptr);
        QCOMPARE(model->count(), 1);
        const int idx = model->indexOfId(QStringLiteral("work"));
        QVERIFY(idx >= 0);
        QVERIFY(model->at(idx).value(QStringLiteral("isDefault")).toBool());
    }

    // PRO-7 codec round-trip: a DecodedDistribution survives encode (import request) + decode
    // (Distribution response) with its profile + skills intact.
    void distributionRoundTrips() {
        const QByteArray resp = distributionResponse(sampleDist());
        QVERIFY(!resp.isEmpty());
        QCOMPARE(NodeApiCodec::responseKind(resp),
                 daemonapp::daemon::ApiResponseKind::Distribution);
        DecodedDistribution back;
        QVERIFY(NodeApiCodec::decodeDistribution(resp, &back));
        QCOMPARE(back.wireVersion, 1u);
        QCOMPARE(back.profile.id, QStringLiteral("work"));
        QCOMPARE(back.profile.model, QStringLiteral("claude"));
        QCOMPARE(back.skills.size(), 1);
        QCOMPARE(back.skills.at(0).name, QStringLiteral("notes"));
        QCOMPARE(back.skills.at(0).files.value(QStringLiteral("SKILL.md")),
                 QStringLiteral("# notes"));
    }

    // PRO-8 codec round-trip: a Revisions response decodes to the revision log, with the
    // operator/agent author distinction and the parent linkage preserved.
    void revisionsRoundTrip() {
        const QByteArray resp = revisionsResponse();
        QVERIFY(!resp.isEmpty());
        QCOMPARE(NodeApiCodec::responseKind(resp), daemonapp::daemon::ApiResponseKind::Revisions);
        QList<DecodedRevision> revs;
        QVERIFY(NodeApiCodec::decodeRevisions(resp, &revs));
        QCOMPARE(revs.size(), 2);
        QCOMPARE(revs.at(0).seq, 1ull);
        QVERIFY(!revs.at(0).hasParent);
        QCOMPARE(revs.at(0).author, QStringLiteral("operator"));
        QCOMPARE(revs.at(0).reason, QStringLiteral("init"));
        QCOMPARE(revs.at(1).seq, 2ull);
        QVERIFY(revs.at(1).hasParent);
        QCOMPARE(revs.at(1).parent, 1ull);
        QCOMPARE(revs.at(1).author, QStringLiteral("bot"));
    }

    // PRO-7 store flow: exportProfileToFile writes the Distribution artifact to the chosen file,
    // and importProfileFromFile reads it back + crosses ProfileImport (answered with a new id).
    void exportThenImportOverMux() {
        const QString sock = m_tmp.filePath(QStringLiteral("prof.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(distributionResponse(sampleDist()));
        DaemonTransportFixture tx(sock);
        ProfileRepository repo(&tx.client, nullptr);
        DaemonProfileStore store(&repo);

        const QString path = m_tmp.filePath(QStringLiteral("work.dist"));
        QSignalSpy saved(&store, &profiles::IProfileStore::exportSaved);
        store.exportProfileToFile(QStringLiteral("work"), QUrl::fromLocalFile(path));
        QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 3000);

        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray artifact = f.readAll();
        f.close();
        DecodedDistribution onDisk;
        QVERIFY(NodeApiCodec::decodeDistribution(artifact, &onDisk));
        QCOMPARE(onDisk.profile.id, QStringLiteral("work"));

        // Import the artifact under a new id: the daemon answers ProfileImport with the new id.
        fake.setReplyPayload(profileIdResponse("work-copy"));
        QSignalSpy imported(&store, &profiles::IProfileStore::imported);
        store.importProfileFromFile(QUrl::fromLocalFile(path), QStringLiteral("work-copy"));
        QTRY_COMPARE_WITH_TIMEOUT(imported.count(), 1, 3000);
        QCOMPARE(imported.at(0).at(0).toString(), QStringLiteral("work-copy"));
    }

    // PRO-8 store flow: requestHistory populates the history model; revert crosses ProfileRevert
    // (answered Ok) and surfaces reverted().
    void historyAndRevertOverMux() {
        const QString sock = m_tmp.filePath(QStringLiteral("prof-hist.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(revisionsResponse());
        DaemonTransportFixture tx(sock);
        ProfileRepository repo(&tx.client, nullptr);
        DaemonProfileStore store(&repo);

        QSignalSpy historyChanged(&store, &profiles::IProfileStore::historyChanged);
        store.requestHistory(QStringLiteral("work"));
        QTRY_COMPARE_WITH_TIMEOUT(historyChanged.count(), 1, 3000);
        auto* hist = qobject_cast<uimodels::VariantListModel*>(store.history());
        QVERIFY(hist != nullptr);
        QCOMPARE(hist->count(), 2);
        // Newest-first projection: revision #2 leads.
        QCOMPARE(hist->at(0).value(QStringLiteral("seq")).toInt(), 2);

        fake.setReplyPayload(okResponse());
        QSignalSpy reverted(&store, &profiles::IProfileStore::reverted);
        store.revertProfile(QStringLiteral("work"), 1);
        QTRY_COMPARE_WITH_TIMEOUT(reverted.count(), 1, 3000);
        QCOMPARE(reverted.at(0).at(0).toString(), QStringLiteral("work"));
    }

    // B: a daemon with no revision log answers ProfileHistory with Unsupported - a capability gap,
    // not a decode failure. historyAvailable flips false (so the UI hides the panel) and flips back
    // true once a real Revisions page arrives.
    void historyAvailabilityTracksDaemon() {
        const QString sock = m_tmp.filePath(QStringLiteral("prof-cap.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(unsupportedResponse());
        DaemonTransportFixture tx(sock);
        ProfileRepository repo(&tx.client, nullptr);
        DaemonProfileStore store(&repo);
        QVERIFY(store.historyAvailable()); // optimistic until proven otherwise

        QSignalSpy availability(&store, &profiles::IProfileStore::historyAvailableChanged);
        QSignalSpy opFailed(&store, &profiles::IProfileStore::profileOpFailed);
        store.requestHistory(QStringLiteral("work"));
        QTRY_VERIFY_WITH_TIMEOUT(!store.historyAvailable(), 3000);
        QVERIFY(availability.count() >= 1);
        // Unsupported is a capability gap, not a misleading "failed to decode" toast.
        QCOMPARE(opFailed.count(), 0);

        fake.setReplyPayload(revisionsResponse());
        store.requestHistory(QStringLiteral("work"));
        QTRY_VERIFY_WITH_TIMEOUT(store.historyAvailable(), 3000);
        auto* hist = qobject_cast<uimodels::VariantListModel*>(store.history());
        QCOMPARE(hist->count(), 2);
    }

    // C2 gap #6a: a daemon_api profile round-trips provider + base_url through ProfileUpdate
    // (encode) and ProfileGet (decode). Guards the three app-mapper sites that the codec regen did
    // NOT touch.
    void daemonApiProviderRoundTrips() {
        daemonapp::daemon::DecodedProfileSpec spec;
        spec.id = QStringLiteral("work");
        spec.provider = QStringLiteral("daemon_api");
        spec.model = QStringLiteral("anthropic/claude-sonnet-4-5");
        spec.hasBaseUrl = true;
        spec.baseUrl = QStringLiteral("https://api.daemon.ai/api/v1");

        // Encode direction: ProfileUpdate carries the daemon_api choice + base_url tstr.
        const QByteArray req = NodeApiCodec::encodeProfileUpdateRequest(spec);
        QVERIFY(!req.isEmpty());
        auto reqStruct = std::make_unique<api_request_r>();
        size_t consumed = 0;
        QCOMPARE(cbor_decode_api_request(reinterpret_cast<const uint8_t*>(req.constData()),
                                         static_cast<size_t>(req.size()), reqStruct.get(),
                                         &consumed),
                 ZCBOR_SUCCESS);
        QCOMPARE(reqStruct->api_request_choice,
                 api_request_r::api_request_request_profile_update_m_c);
        const profile_spec& ps = reqStruct->api_request_request_profile_update_m.ProfileUpdate_spec;
        QCOMPARE(ps.profile_spec_provider.provider_selector_choice,
                 provider_selector_r::provider_selector_daemon_api_tstr_c);
        QCOMPARE(ps.profile_spec_base_url_choice, profile_spec::profile_spec_base_url_tstr_c);
        QCOMPARE(zToQString(ps.profile_spec_base_url_tstr), spec.baseUrl);

        // Decode direction: a Profile(ProfileGet) response with that spec decodes daemon_api back.
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_profile_m_c;
        resp->api_response_response_profile_m.response_profile_Profile_choice =
            response_profile::response_profile_Profile_profile_spec_m_c;
        resp->api_response_response_profile_m.response_profile_Profile_profile_spec_m = ps;
        const QByteArray respBytes = encodeResponse(*resp);
        QVERIFY(!respBytes.isEmpty());
        daemonapp::daemon::DecodedProfileSpec out;
        bool found = false;
        QVERIFY(NodeApiCodec::decodeProfile(respBytes, &out, &found));
        QVERIFY(found);
        QCOMPARE(out.provider, QStringLiteral("daemon_api"));
        QVERIFY(out.hasBaseUrl);
        QCOMPARE(out.baseUrl, spec.baseUrl);

        // Empty base URL => no override on the wire (Option::None), i.e. the node/provider default.
        daemonapp::daemon::DecodedProfileSpec noBase = spec;
        noBase.hasBaseUrl = false;
        noBase.baseUrl.clear();
        const QByteArray req2 = NodeApiCodec::encodeProfileUpdateRequest(noBase);
        auto reqStruct2 = std::make_unique<api_request_r>();
        QCOMPARE(cbor_decode_api_request(reinterpret_cast<const uint8_t*>(req2.constData()),
                                         static_cast<size_t>(req2.size()), reqStruct2.get(),
                                         &consumed),
                 ZCBOR_SUCCESS);
        QCOMPARE(reqStruct2->api_request_request_profile_update_m.ProfileUpdate_spec
                     .profile_spec_base_url_choice,
                 profile_spec::profile_spec_base_url_null_m_c);
    }

    // Regression: the mock provider still decodes as "mock" (not swallowed by the new daemon_api
    // arm)
    // - so a Mock-valued profile keeps round-tripping and the editor can render it read-only.
    void mockProviderStillDecodesAsMock() {
        daemonapp::daemon::DecodedProfileSpec spec;
        spec.id = QStringLiteral("m");
        spec.provider = QStringLiteral("mock");
        spec.model = QStringLiteral("mock-model");
        const QByteArray req = NodeApiCodec::encodeProfileUpdateRequest(spec);
        QVERIFY(!req.isEmpty());
        auto reqStruct = std::make_unique<api_request_r>();
        size_t consumed = 0;
        QCOMPARE(cbor_decode_api_request(reinterpret_cast<const uint8_t*>(req.constData()),
                                         static_cast<size_t>(req.size()), reqStruct.get(),
                                         &consumed),
                 ZCBOR_SUCCESS);
        const profile_spec& ps = reqStruct->api_request_request_profile_update_m.ProfileUpdate_spec;
        QCOMPARE(ps.profile_spec_provider.provider_selector_choice,
                 provider_selector_r::provider_selector_mock_tstr_c);

        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_profile_m_c;
        resp->api_response_response_profile_m.response_profile_Profile_choice =
            response_profile::response_profile_Profile_profile_spec_m_c;
        resp->api_response_response_profile_m.response_profile_Profile_profile_spec_m = ps;
        daemonapp::daemon::DecodedProfileSpec out;
        bool found = false;
        QVERIFY(NodeApiCodec::decodeProfile(encodeResponse(*resp), &out, &found));
        QCOMPARE(out.provider, QStringLiteral("mock"));
    }

    // C2: the DaemonAccountsService projects the redacted CredentialList into profile-scoped
    // status, and a profile-scoped key set is guarded/routed (no request for an empty key).
    void credentialForReflectsCredentialList() {
        const QString sock = m_tmp.filePath(QStringLiteral("cred.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(credentialsResponse());
        DaemonTransportFixture tx(sock);
        daemonapp::daemon::CredentialRepository creds(&tx.client, nullptr);
        daemonapp::daemon::ProfileRepository profs(&tx.client, nullptr);
        accounts::DaemonAccountsService acct(&creds, &profs);

        QSignalSpy changed(&acct, &accounts::IAccountsService::credentialsChanged);
        creds.refreshList();
        QTRY_VERIFY_WITH_TIMEOUT(changed.count() >= 1, 3000);

        const QVariantMap work = acct.credentialFor(QStringLiteral("work"));
        QVERIFY(work.value(QStringLiteral("present")).toBool());
        QVERIFY(work.value(QStringLiteral("hint")).toString().contains(QStringLiteral("abcd")));
        QVERIFY(
            !acct.credentialFor(QStringLiteral("other")).value(QStringLiteral("present")).toBool());
        QVERIFY(!acct.credentialFor(QStringLiteral("missing"))
                     .value(QStringLiteral("present"))
                     .toBool());

        // An empty key is a guarded no-op (no CredentialSet issued); a real key issues a request
        // scoped to the target profile (setCredential(profileId, key)).
        const int before = fake.requestCount();
        acct.addApiKeyForProfile(QStringLiteral("work"), QStringLiteral("daemon_api"), QString(),
                                 QString());
        QCOMPARE(fake.requestCount(), before);
        acct.addApiKeyForProfile(QStringLiteral("work"), QStringLiteral("daemon_api"), QString(),
                                 QStringLiteral("sk-new-key"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.requestCount() > before, 3000);
    }

    // C: decodeWireFrame surfaces the server's advertised Hello feature list (incl. versioning).
    void decodeWireFrameSurfacesFeatures() {
        daemonapp::daemon::DecodedWireFrame frame;
        QVERIFY(NodeApiCodec::decodeWireFrame(helloFrame({"mux", "stream", "versioning"}), &frame));
        QCOMPARE(frame.kind, daemonapp::daemon::WireFrameKind::Hello);
        QVERIFY(frame.features.contains(QStringLiteral("mux")));
        QVERIFY(frame.features.contains(QStringLiteral("stream")));
        QVERIFY(frame.features.contains(QStringLiteral("versioning")));

        // A node without a revision log omits versioning.
        daemonapp::daemon::DecodedWireFrame plain;
        QVERIFY(NodeApiCodec::decodeWireFrame(helloFrame({"mux", "stream"}), &plain));
        QVERIFY(!plain.features.contains(QStringLiteral("versioning")));
    }

private:
    QTemporaryDir m_tmp;

    // A connected transport + client bundle (created together so the client outlives the
    // transport).
    struct DaemonTransportFixture {
        explicit DaemonTransportFixture(const QString& sock) : client(&transport) {
            transport.setSocketPath(sock);
        }
        daemonapp::daemon::DaemonTransport transport;
        NodeApiClient client;
    };
};

QTEST_MAIN(TestDaemonProfileStore)
#include "tst_daemon_profile_store.moc"
