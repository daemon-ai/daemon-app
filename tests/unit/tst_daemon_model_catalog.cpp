// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_model_catalog.h"
#include "daemon/daemon_provider_catalog.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "models/imodel_catalog.h"
#include "uimodels/variant_list_model.h"
#include "wire_mux_fixture.h"

#include <memory>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::CredentialRepository;
using daemonapp::daemon::DaemonProviderCatalog;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::ModelRepository;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::daemon::ProfileRepository;
using daemonapp::daemon::ProviderRepository;
using daemonapp::test::WireMuxServer;
using models::DaemonModelCatalog;

namespace {

void setZ(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
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

// {"Models": {"items": [], "next": absent}} — the empty first (and only) cloud-catalog page.
QByteArray emptyModelsResponse() {
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_models_m_c;
    model_page& page = resp->api_response_response_models_m.response_models_Models;
    page.model_page_items_model_descriptor_m_count = 0;
    page.model_page_next_present = false;
    return encodeResponse(*resp);
}

// {"ModelCurrent": null} — no resolvable current model.
QByteArray nullModelCurrentResponse() {
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_model_current_m_c;
    resp->api_response_response_model_current_m.response_model_current_ModelCurrent_choice =
        response_model_current::response_model_current_ModelCurrent_null_m_c;
    return encodeResponse(*resp);
}

// {"ModelCatalog": [installed llama models named by `ids`]}. Buffers must outlive encode, so the
// utf8 copies are returned alongside... instead keep them static per call via QByteArrayList.
QByteArray modelCatalogResponse(const QList<QByteArray>& ids) {
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_model_catalog_m_c;
    response_model_catalog& cat = resp->api_response_response_model_catalog_m;
    cat.response_model_catalog_ModelCatalog_installed_model_m_count =
        static_cast<size_t>(ids.size());
    static const QByteArray repo = "org/repo";
    static const QByteArray file = "m.gguf";
    static const QByteArray revision = "main";
    static const QByteArray path = "/cache/m.gguf";
    for (qsizetype i = 0; i < ids.size(); ++i) {
        installed_model& m = cat.response_model_catalog_ModelCatalog_installed_model_m[i];
        m = installed_model{};
        setZ(m.installed_model_id, ids.at(i));
        m.installed_model_model.model_ref_engine.model_engine_choice =
            model_engine_r::model_engine_Llama_tstr_c;
        m.installed_model_model.model_ref_source.model_source_choice =
            model_source_r::model_source_hf_m_c;
        model_source_hf& hf = m.installed_model_model.model_ref_source.model_source_hf_m;
        setZ(hf.Hf_repo, repo);
        hf.Hf_file_choice = model_source_hf::Hf_file_tstr_c;
        setZ(hf.Hf_file_tstr, file);
        setZ(hf.Hf_revision, revision);
        setZ(m.installed_model_display_name, ids.at(i));
        setZ(m.installed_model_local_path, path);
        m.installed_model_size_bytes = 42;
        m.installed_model_quant_choice = installed_model::installed_model_quant_null_m_c;
        m.installed_model_installed_at_ms = 1;
        m.installed_model_arch_present = false;
        m.installed_model_context_length_present = false;
        m.installed_model_file_type_present = false;
    }
    return encodeResponse(*resp);
}

// {"ModelSearch": {"page": N, "results": [repos...], "has_more": B}}.
QByteArray searchPageResponse(quint32 page, const QList<QByteArray>& repos, bool hasMore) {
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_model_search_m_c;
    search_page& sp = resp->api_response_response_model_search_m.response_model_search_ModelSearch;
    sp.search_page_page = page;
    sp.search_page_results_search_hit_m_count = static_cast<size_t>(repos.size());
    sp.search_page_has_more = hasMore;
    for (qsizetype i = 0; i < repos.size(); ++i) {
        search_hit& h = sp.search_page_results_search_hit_m[i];
        h = search_hit{};
        setZ(h.search_hit_repo, repos.at(i));
        h.search_hit_author_choice = search_hit::search_hit_author_null_m_c;
        h.search_hit_downloads = 10;
        h.search_hit_likes = 1;
        h.search_hit_num_parameters_choice = search_hit::search_hit_num_parameters_null_m_c;
        h.search_hit_pipeline_tag_choice = search_hit::search_hit_pipeline_tag_null_m_c;
        h.search_hit_last_modified_choice = search_hit::search_hit_last_modified_null_m_c;
        h.search_hit_gated = false;
        h.search_hit_private = false;
    }
    return encodeResponse(*resp);
}

// {"ModelDownloadStarted": id}.
QByteArray downloadStartedResponse(quint64 id) {
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_model_download_started_m_c;
    resp->api_response_response_model_download_started_m
        .response_model_download_started_ModelDownloadStarted = id;
    return encodeResponse(*resp);
}

// {"ModelDownloads": []} — the empty snapshot the post-start refresh consumes.
QByteArray emptyDownloadsResponse() {
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_model_downloads_m_c;
    resp->api_response_response_model_downloads_m
        .response_model_downloads_ModelDownloads_download_status_m_count = 0;
    return encodeResponse(*resp);
}

// The generated choice enums are anonymous; name the download-state one for parameters.
using DownloadStateChoice = decltype(download_state_r::download_state_choice);

// {"ModelDownloads": [{id, model: org/repo + m.gguf, state, bytes, files, error?}]} — one
// full-shape job row (the same org/repo + m.gguf source the modelCatalogResponse fixture
// installs). files_done/files_total default to the single-file shape.
QByteArray downloadStatusResponse(quint64 id, DownloadStateChoice state, quint64 downloadedBytes,
                                  quint64 totalBytes, const QByteArray& error,
                                  quint32 filesDone = 0, quint32 filesTotal = 1) {
    static const QByteArray repo = "org/repo";
    static const QByteArray file = "m.gguf";
    static const QByteArray revision = "main";
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_model_downloads_m_c;
    response_model_downloads& dl = resp->api_response_response_model_downloads_m;
    dl.response_model_downloads_ModelDownloads_download_status_m_count = 1;
    download_status& s = dl.response_model_downloads_ModelDownloads_download_status_m[0];
    s = download_status{};
    s.download_status_id = id;
    s.download_status_model.model_ref_engine.model_engine_choice =
        model_engine_r::model_engine_Llama_tstr_c;
    s.download_status_model.model_ref_source.model_source_choice =
        model_source_r::model_source_hf_m_c;
    model_source_hf& hf = s.download_status_model.model_ref_source.model_source_hf_m;
    setZ(hf.Hf_repo, repo);
    hf.Hf_file_choice = model_source_hf::Hf_file_tstr_c;
    setZ(hf.Hf_file_tstr, file);
    setZ(hf.Hf_revision, revision);
    s.download_status_state.download_state_choice = state;
    s.download_status_downloaded_bytes = downloadedBytes;
    s.download_status_total_bytes = totalBytes;
    s.download_status_files_done = filesDone;
    s.download_status_files_total = filesTotal;
    if (error.isEmpty()) {
        s.download_status_error_choice = download_status::download_status_error_null_m_c;
    } else {
        s.download_status_error_choice = download_status::download_status_error_tstr_c;
        setZ(s.download_status_error_tstr, error);
    }
    return encodeResponse(*resp);
}

// {"ProviderCatalog": [llama_cpp (local, keyless), anthropic (cloud/genai, keyed),
//                      daemon_cloud (daemon_cloud/daemon_api, keyed)]}.
QByteArray providerCatalogResponse() {
    static const QByteArray idLlama = "llama_cpp", nameLlama = "llama.cpp (local)";
    static const QByteArray idAnthropic = "anthropic", nameAnthropic = "Anthropic";
    static const QByteArray idCloud = "daemon_cloud", nameCloud = "Daemon Cloud";
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_provider_catalog_m_c;
    response_provider_catalog& pc = resp->api_response_response_provider_catalog_m;
    pc.response_provider_catalog_ProviderCatalog_provider_descriptor_m_count = 3;

    provider_descriptor& d0 = pc.response_provider_catalog_ProviderCatalog_provider_descriptor_m[0];
    d0 = provider_descriptor{};
    setZ(d0.provider_descriptor_id, idLlama);
    setZ(d0.provider_descriptor_display_name, nameLlama);
    d0.provider_descriptor_kind.provider_kind_wire_choice =
        provider_kind_wire_r::provider_kind_wire_local_tstr_c;
    d0.provider_descriptor_wire_selector.provider_selector_choice =
        provider_selector_r::provider_selector_llama_cpp_tstr_c;
    d0.provider_descriptor_requires_key = false;
    d0.provider_descriptor_supports_model_discovery = true;
    d0.provider_descriptor_default_base_url_choice =
        provider_descriptor::provider_descriptor_default_base_url_null_m_c;

    provider_descriptor& d1 = pc.response_provider_catalog_ProviderCatalog_provider_descriptor_m[1];
    d1 = provider_descriptor{};
    setZ(d1.provider_descriptor_id, idAnthropic);
    setZ(d1.provider_descriptor_display_name, nameAnthropic);
    d1.provider_descriptor_kind.provider_kind_wire_choice =
        provider_kind_wire_r::provider_kind_wire_cloud_tstr_c;
    d1.provider_descriptor_wire_selector.provider_selector_choice =
        provider_selector_r::provider_selector_genai_tstr_c;
    d1.provider_descriptor_requires_key = true;
    d1.provider_descriptor_supports_model_discovery = true;
    d1.provider_descriptor_default_base_url_choice =
        provider_descriptor::provider_descriptor_default_base_url_null_m_c;

    provider_descriptor& d2 = pc.response_provider_catalog_ProviderCatalog_provider_descriptor_m[2];
    d2 = provider_descriptor{};
    setZ(d2.provider_descriptor_id, idCloud);
    setZ(d2.provider_descriptor_display_name, nameCloud);
    d2.provider_descriptor_kind.provider_kind_wire_choice =
        provider_kind_wire_r::provider_kind_wire_daemon_cloud_tstr_c;
    d2.provider_descriptor_wire_selector.provider_selector_choice =
        provider_selector_r::provider_selector_daemon_api_tstr_c;
    d2.provider_descriptor_requires_key = true;
    d2.provider_descriptor_supports_model_discovery = true;
    d2.provider_descriptor_default_base_url_choice =
        provider_descriptor::provider_descriptor_default_base_url_null_m_c;
    return encodeResponse(*resp);
}

// {"Profiles": [{id: "work", provider: genai, model: "claude", active}]}.
QByteArray profilesResponse() {
    static const QByteArray id = "work";
    static const QByteArray model = "claude-opus-4-8";
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_profiles_m_c;
    response_profiles& ps = resp->api_response_response_profiles_m;
    ps.response_profiles_Profiles_profile_info_m_count = 1;
    profile_info& p = ps.response_profiles_Profiles_profile_info_m[0];
    p = profile_info{};
    setZ(p.profile_info_id, id);
    p.profile_info_provider.provider_selector_choice =
        provider_selector_r::provider_selector_genai_tstr_c;
    setZ(p.profile_info_model, model);
    p.profile_info_is_active = true;
    p.profile_info_bound_accounts_bound_account_m_count = 0;
    return encodeResponse(*resp);
}

// {"Credentials": [{profile: "work", present: true, hint}]}.
QByteArray credentialsResponse() {
    static const QByteArray profile = "work";
    static const QByteArray hint = "…cret";
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_credentials_m_c;
    response_credentials& cs = resp->api_response_response_credentials_m;
    cs.response_credentials_Credentials_credential_info_m_count = 1;
    credential_info& c = cs.response_credentials_Credentials_credential_info_m[0];
    c = credential_info{};
    setZ(c.credential_info_profile, profile);
    c.credential_info_present = true;
    setZ(c.credential_info_hint, hint);
    return encodeResponse(*resp);
}

// A minimal IModelCatalog stub that lets a test EMIT the facade signals directly.
class StubCatalog : public models::IModelCatalog {
public:
    explicit StubCatalog(QObject* parent = nullptr)
        : IModelCatalog(parent), m_rows(new uimodels::VariantListModel(this)) {}
    [[nodiscard]] QObject* discover() const override { return m_rows; }
    [[nodiscard]] QObject* files() const override { return m_rows; }
    [[nodiscard]] QObject* downloads() const override { return m_rows; }
    [[nodiscard]] QObject* installed() const override { return m_rows; }
    [[nodiscard]] QString currentModelId() const override { return {}; }
    [[nodiscard]] QString filesRepo() const override { return {}; }
    [[nodiscard]] QStringList installedIds() const override { return {}; }
    [[nodiscard]] QVariantList providers() const override { return {}; }
    void search(const QString&, const QString&) override {}
    void repoFiles(const QString&) override {}
    void recommend(const QString&) override {}
    [[nodiscard]] QVariantMap recommendation() const override { return {}; }
    void download(const QString&) override {}
    void downloadFile(const QString&, const QString&, const QString&) override {}
    void pauseDownload(const QString&) override {}
    void resumeDownload(const QString&) override {}
    void cancelDownload(const QString&) override {}
    [[nodiscard]] QString installedIdFor(const QString&, const QString&) const override {
        return {};
    }
    void dismissDownload(const QString&) override {}
    void activate(const QString&) override {}
    void remove(const QString&) override {}
    void fireDownloadFinished(const QString& id) { emit downloadFinished(id); }

private:
    uimodels::VariantListModel* m_rows = nullptr;
};

} // namespace

// Guards the daemon-backed model-catalog facade: uint64 download-id encoding, search paging
// accumulation, the downloadStarted/downloadFinished signal bridge (incl. installed-set growth
// detection), the construction-time downloads seed (pre-existing jobs render immediately),
// operationFailed -> lastError surfacing, and the real-data Providers projection.
class TestDaemonModelCatalog : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { QVERIFY(m_tmp.isValid()); }

    // The download lifecycle encoders carry the full uint64 id (the contract's download-id):
    // a >32-bit id must round-trip through the generated codec without truncation.
    void downloadIdEncodesUint64() {
        const quint64 big = (quint64(1) << 32) + 5;
        struct Case {
            QByteArray bytes;
            int choice;
        };
        const QList<Case> cases = {
            {NodeApiCodec::encodeModelCancelRequest(big),
             api_request_r::api_request_request_model_cancel_m_c},
            {NodeApiCodec::encodeModelPauseRequest(big),
             api_request_r::api_request_request_model_pause_m_c},
            {NodeApiCodec::encodeModelResumeRequest(big),
             api_request_r::api_request_request_model_resume_m_c},
        };
        for (const Case& c : cases) {
            QVERIFY(!c.bytes.isEmpty());
            auto req = std::make_unique<api_request_r>();
            size_t consumed = 0;
            QCOMPARE(cbor_decode_api_request(reinterpret_cast<const uint8_t*>(c.bytes.constData()),
                                             static_cast<size_t>(c.bytes.size()), req.get(),
                                             &consumed),
                     ZCBOR_SUCCESS);
            QCOMPARE(static_cast<int>(req->api_request_choice), c.choice);
            quint64 decoded = 0;
            switch (req->api_request_choice) {
            case api_request_r::api_request_request_model_cancel_m_c:
                decoded = req->api_request_request_model_cancel_m.ModelCancel_id;
                break;
            case api_request_r::api_request_request_model_pause_m_c:
                decoded = req->api_request_request_model_pause_m.ModelPause_id;
                break;
            default:
                decoded = req->api_request_request_model_resume_m.ModelResume_id;
                break;
            }
            QCOMPARE(decoded, big);
        }
    }

    // Search paging: a fresh search replaces the hit list and reports has_more; searchMore()
    // APPENDS the next page (re-issuing the query with page+1) and clears has_more on the last
    // page. A repeated searchMore with no further page is a no-op.
    void searchPagingAccumulates() {
        const QString sock = m_tmp.filePath(QStringLiteral("search.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        ModelRepository repo(&client, nullptr);
        QSignalSpy hits(&repo, &ModelRepository::searchHitsChanged);

        fake.setReplyPayload(searchPageResponse(0, {"org/a"}, true));
        repo.search(QStringLiteral("smol"));
        QTRY_COMPARE_WITH_TIMEOUT(hits.count(), 1, 3000);
        QCOMPARE(repo.searchHits().size(), 1);
        QVERIFY(repo.searchHasMore());

        fake.setReplyPayload(searchPageResponse(1, {"org/b"}, false));
        repo.searchMore();
        QTRY_COMPARE_WITH_TIMEOUT(hits.count(), 2, 3000);
        QCOMPARE(repo.searchHits().size(), 2);
        QCOMPARE(repo.searchHits().at(0).repo, QStringLiteral("org/a"));
        QCOMPARE(repo.searchHits().at(1).repo, QStringLiteral("org/b"));
        QVERIFY(!repo.searchHasMore());

        // The continuation request re-issued the query with page = 1.
        const QList<QByteArray> payloads = fake.callPayloads();
        QVERIFY(payloads.size() >= 2);
        auto req = std::make_unique<api_request_r>();
        size_t consumed = 0;
        const QByteArray& last = payloads.last();
        QCOMPARE(cbor_decode_api_request(reinterpret_cast<const uint8_t*>(last.constData()),
                                         static_cast<size_t>(last.size()), req.get(), &consumed),
                 ZCBOR_SUCCESS);
        QCOMPARE(static_cast<int>(req->api_request_choice),
                 static_cast<int>(api_request_r::api_request_request_model_search_m_c));
        QCOMPARE(req->api_request_request_model_search_m.ModelSearch_query.search_query_page,
                 quint32(1));

        // Exhausted: no further request is issued.
        const int calls = fake.requestCount();
        repo.searchMore();
        QTest::qWait(150);
        QCOMPARE(fake.requestCount(), calls);
    }

    // The facade bridges the download lifecycle: downloadStarted fires on the accepted job (with
    // the stringified job id), and rebuildInstalled emits downloadFinished for each id the
    // installed set GAINED (first the baseline snapshot, then exactly the newly-installed id).
    void facadeBridgesDownloadLifecycle() {
        const QString sock = m_tmp.filePath(QStringLiteral("facade.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        ModelRepository repo(&client, nullptr);

        // Construction fires refreshModels + refreshCatalog + refreshCurrent + refreshDownloads
        // (in that order).
        fake.setReplySequence({emptyModelsResponse(), modelCatalogResponse({"model-a"}),
                               nullModelCurrentResponse(), emptyDownloadsResponse()});
        DaemonModelCatalog catalog(&repo);
        QSignalSpy finished(&catalog, &models::IModelCatalog::downloadFinished);
        QSignalSpy started(&catalog, &models::IModelCatalog::downloadStarted);

        // Baseline snapshot: the first catalog reflection announces the pre-installed model.
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 3000);
        QCOMPARE(finished.at(0).at(0).toString(), QStringLiteral("model-a"));

        // The set GROWS by model-b: exactly one more downloadFinished, for the new id only.
        fake.setReplyPayload(modelCatalogResponse({"model-a", "model-b"}));
        repo.refreshCatalog();
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 2, 3000);
        QCOMPARE(finished.at(1).at(0).toString(), QStringLiteral("model-b"));

        // An unchanged set stays quiet.
        repo.refreshCatalog();
        QTest::qWait(200);
        QCOMPARE(finished.count(), 2);

        // downloadFile -> ModelDownloadStarted(7) bridges to downloadStarted("7") (the follow-up
        // downloads snapshot is served an empty list).
        fake.setReplySequence({downloadStartedResponse(7), emptyDownloadsResponse()});
        catalog.downloadFile(QStringLiteral("org/repo"), QStringLiteral("m.gguf"));
        QTRY_COMPARE_WITH_TIMEOUT(started.count(), 1, 3000);
        QCOMPARE(started.at(0).at(0).toString(), QStringLiteral("7"));
    }

    // Constructing the facade against a daemon that ALREADY tracks an in-flight job (the daemon
    // outlives the GUI) populates downloads() from the ctor's own seed — no explicit refresh and
    // no waiting for the next throttled progress event. Also pins the projected multi-file keys
    // (filesDone/filesTotal) the panel's "file x/y" label binds.
    void constructionSeedsExistingDownloads() {
        const QString sock = m_tmp.filePath(QStringLiteral("seed.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        ModelRepository repo(&client, nullptr);

        // Construction fires refreshModels + refreshCatalog + refreshCurrent + refreshDownloads
        // (in that order); the downloads snapshot already carries one live multi-file job.
        fake.setReplySequence(
            {emptyModelsResponse(), modelCatalogResponse({}), nullModelCurrentResponse(),
             downloadStatusResponse(9, download_state_r::download_state_Downloading_tstr_c, 25, 100,
                                    {}, /*filesDone=*/2, /*filesTotal=*/5)});
        DaemonModelCatalog catalog(&repo);
        auto* downloads = qobject_cast<uimodels::VariantListModel*>(catalog.downloads());
        QVERIFY(downloads != nullptr);

        // No repo.refreshDownloads() here: the rows appear off construction alone.
        QTRY_COMPARE_WITH_TIMEOUT(downloads->count(), 1, 3000);
        const QVariantMap row = downloads->at(0);
        QCOMPARE(row.value(QStringLiteral("id")).toString(), QStringLiteral("9"));
        QCOMPARE(row.value(QStringLiteral("state")).toString(), QStringLiteral("downloading"));
        QCOMPARE(row.value(QStringLiteral("progress")).toDouble(), 0.25);
        QCOMPARE(row.value(QStringLiteral("filesDone")).toInt(), 2);
        QCOMPARE(row.value(QStringLiteral("filesTotal")).toInt(), 5);
    }

    // Model-operation failures surface on the facade lastError property (and a fresh operation
    // clears it) — the seam the Models pages + wizard discover dialog render.
    void operationFailedSetsLastError() {
        const QString sock = m_tmp.filePath(QStringLiteral("err.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        ModelRepository repo(&client, nullptr);
        // Construction seeds resolve against an empty models page (decoding the catalog/current
        // probes against the same payload fails silently into lastError; tolerated here).
        fake.setReplyPayload(emptyModelsResponse());
        DaemonModelCatalog catalog(&repo);
        QSignalSpy errChanged(&catalog, &models::IModelCatalog::lastErrorChanged);

        // A search answered with a WRONG response shape decodes to an operation failure.
        catalog.search(QStringLiteral("smol"));
        QTRY_VERIFY_WITH_TIMEOUT(!catalog.lastError().isEmpty(), 3000);
        QVERIFY(errChanged.count() >= 1);

        // The next user-initiated operation clears the stale error immediately.
        fake.setReplyPayload(searchPageResponse(0, {"org/a"}, false));
        catalog.search(QStringLiteral("smol"));
        QVERIFY(catalog.lastError().isEmpty());
    }

    // installedIdFor maps a download row's source (repo + file) to the node's installed CATALOG
    // id — the id space offered-model rows and activate() use. Exact repo+file match first, a
    // repo-only fallback for file-less rows (whose name IS the repo), "" on a miss.
    void installedIdForResolvesInstalledSource() {
        const QString sock = m_tmp.filePath(QStringLiteral("resolve.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        ModelRepository repo(&client, nullptr);

        // Construction: refreshModels + refreshCatalog + refreshCurrent + refreshDownloads. The
        // fixture catalog installs "model-a" sourced from org/repo + m.gguf.
        fake.setReplySequence({emptyModelsResponse(), modelCatalogResponse({"model-a"}),
                               nullModelCurrentResponse(), emptyDownloadsResponse()});
        DaemonModelCatalog catalog(&repo);
        QSignalSpy finished(&catalog, &models::IModelCatalog::downloadFinished);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 3000); // installed set reflected

        QCOMPARE(catalog.installedIdFor(QStringLiteral("org/repo"), QStringLiteral("m.gguf")),
                 QStringLiteral("model-a"));
        // A file-less download row names itself after the repo; the repo-only fallback holds.
        QCOMPARE(catalog.installedIdFor(QStringLiteral("org/repo"), QStringLiteral("org/repo")),
                 QStringLiteral("model-a"));
        QCOMPARE(catalog.installedIdFor(QStringLiteral("org/none"), QStringLiteral("m.gguf")),
                 QString());
        QCOMPARE(catalog.installedIdFor(QString(), QString()), QString());
    }

    // dismissDownload hides a TERMINAL row client-side and the filter survives later
    // downloadsChanged rebuilds; an ACTIVE row refuses dismissal. Also pins the projected
    // downloads row shape (the keys DownloadsPanel renders).
    void dismissHidesTerminalRowsAcrossRebuilds() {
        const QString sock = m_tmp.filePath(QStringLiteral("dismiss.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        ModelRepository repo(&client, nullptr);

        fake.setReplySequence({emptyModelsResponse(), modelCatalogResponse({}),
                               nullModelCurrentResponse(), emptyDownloadsResponse()});
        DaemonModelCatalog catalog(&repo);
        auto* downloads = qobject_cast<uimodels::VariantListModel*>(catalog.downloads());
        QVERIFY(downloads != nullptr);

        // A failed job arrives via the downloads snapshot: full row shape, normalized state.
        fake.setReplyPayload(downloadStatusResponse(
            7, download_state_r::download_state_Failed_tstr_c, 10, 100, "boom"));
        repo.refreshDownloads();
        QTRY_COMPARE_WITH_TIMEOUT(downloads->count(), 1, 3000);
        const QVariantMap row = downloads->at(0);
        QCOMPARE(row.value(QStringLiteral("id")).toString(), QStringLiteral("7"));
        QCOMPARE(row.value(QStringLiteral("name")).toString(), QStringLiteral("m.gguf"));
        QCOMPARE(row.value(QStringLiteral("repo")).toString(), QStringLiteral("org/repo"));
        QCOMPARE(row.value(QStringLiteral("state")).toString(), QStringLiteral("failed"));
        QCOMPARE(row.value(QStringLiteral("error")).toString(), QStringLiteral("boom"));
        QCOMPARE(row.value(QStringLiteral("progress")).toDouble(), 0.1);
        QVERIFY(row.contains(QStringLiteral("sizeLabel")));
        QVERIFY(row.contains(QStringLiteral("downloadedLabel")));

        // Terminal row: dismissible (rebuildDownloads runs synchronously off the direct call).
        catalog.dismissDownload(QStringLiteral("7"));
        QCOMPARE(downloads->count(), 0);

        // The hidden id survives later rebuilds: a progress event for ANOTHER job re-projects
        // the whole list, and job 7 stays filtered.
        repo.applyDownloadProgress(8, QStringLiteral("Downloading"), 50, 200);
        QCOMPARE(downloads->count(), 1);
        QCOMPARE(downloads->at(0).value(QStringLiteral("id")).toString(), QStringLiteral("8"));

        // An ACTIVE row refuses dismissal (pause/cancel are the affordances for live jobs).
        catalog.dismissDownload(QStringLiteral("8"));
        QCOMPARE(downloads->count(), 1);
    }

    // providers() projects the node's REAL ProviderCatalog: local providers are configured as-is
    // (keyless), a key-requiring provider is configured iff some profile bound to its wire
    // selector holds a credential (here: anthropic via the genai-selector "work" profile), and a
    // keyed provider with no credential (daemon_cloud) reports unconfigured.
    void providersProjectRealCatalogAndCredentials() {
        const QString sock = m_tmp.filePath(QStringLiteral("providers.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        ModelRepository models(&client, nullptr);
        ProviderRepository providers(&client, nullptr);
        CredentialRepository credentials(&client, nullptr);
        ProfileRepository profiles(&client, nullptr);

        // Serve the four construction-time model refreshes, then the explicit provider /
        // profile / credential refreshes, in call order.
        fake.setReplySequence({emptyModelsResponse(), modelCatalogResponse({}),
                               nullModelCurrentResponse(), emptyDownloadsResponse(),
                               providerCatalogResponse(), profilesResponse(),
                               credentialsResponse()});
        DaemonModelCatalog catalog(&models, &providers, &credentials, &profiles);

        QSignalSpy provRefreshed(&providers, &ProviderRepository::providersRefreshed);
        QSignalSpy profRefreshed(&profiles, &ProfileRepository::profilesRefreshed);
        QSignalSpy credRefreshed(&credentials, &CredentialRepository::listRefreshed);
        providers.refreshProviders();
        profiles.refreshProfiles();
        credentials.refreshList();
        QTRY_VERIFY_WITH_TIMEOUT(provRefreshed.count() == 1 && profRefreshed.count() == 1 &&
                                     credRefreshed.count() == 1,
                                 3000);

        const QVariantList rows = catalog.providers();
        QCOMPARE(rows.size(), 3);
        QHash<QString, QVariantMap> byId;
        for (const QVariant& v : rows) {
            byId.insert(v.toMap().value(QStringLiteral("id")).toString(), v.toMap());
        }
        QCOMPARE(byId.value(QStringLiteral("llama_cpp")).value(QStringLiteral("kind")).toString(),
                 QStringLiteral("local"));
        QVERIFY(
            byId.value(QStringLiteral("llama_cpp")).value(QStringLiteral("configured")).toBool());
        QCOMPARE(byId.value(QStringLiteral("anthropic")).value(QStringLiteral("kind")).toString(),
                 QStringLiteral("remote"));
        QVERIFY(byId.value(QStringLiteral("anthropic"))
                    .value(QStringLiteral("configured"))
                    .toBool()); // the "work" profile (genai selector) holds a credential
        QVERIFY(!byId.value(QStringLiteral("daemon_cloud"))
                     .value(QStringLiteral("configured"))
                     .toBool()); // keyed, and no daemon_api-selector profile has a key
    }

    // The provider catalog re-offers the LOCAL providers' models when the model facade announces
    // a finished download — the picker un-stales without re-selecting the provider.
    void providerCatalogReoffersOnDownloadFinished() {
        StubCatalog stub;
        DaemonProviderCatalog providerCatalog(nullptr, &stub);
        QSignalSpy offered(&providerCatalog, &models::IProviderCatalog::offeredModelsChanged);
        stub.fireDownloadFinished(QStringLiteral("model-x"));
        QCOMPARE(offered.count(), 2); // llama_cpp + mistral_rs recompose
        QCOMPARE(offered.at(0).at(0).toString(), QStringLiteral("llama_cpp"));
        QCOMPARE(offered.at(1).at(0).toString(), QStringLiteral("mistral_rs"));
    }

private:
    QTemporaryDir m_tmp;
};

QTEST_MAIN(TestDaemonModelCatalog)
#include "tst_daemon_model_catalog.moc"
