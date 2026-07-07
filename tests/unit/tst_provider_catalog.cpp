// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_provider_catalog.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "models/mock_provider_catalog.h"
#include "models/provider_model_compose.h"
#include "wire_mux_fixture.h"

#include <memory>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::DaemonProviderCatalog;
using daemonapp::daemon::DecodedModelDescriptor;
using daemonapp::daemon::DecodedProviderDescriptor;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::daemon::ProviderRepository;
using daemonapp::test::WireMuxServer;

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

// A ProviderCatalog response with Daemon Cloud (daemon_cloud/daemon_api, requires_key, base URL),
// Anthropic (cloud/genai, requires_key), and Llama.cpp (local/llama_cpp). The buffers must outlive
// the encode call, so they live in this scope until encodeResponse returns.
QByteArray providerCatalogResponse() {
    const QByteArray idA = "daemon_cloud", nameA = "Daemon Cloud",
                     baseA = "https://api.daemon.ai/api/v1";
    const QByteArray idB = "anthropic", nameB = "Anthropic";
    const QByteArray idC = "llama_cpp", nameC = "Llama.cpp";
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_provider_catalog_m_c;
    response_provider_catalog& pc = resp->api_response_response_provider_catalog_m;
    pc.response_provider_catalog_ProviderCatalog_provider_descriptor_m_count = 3;

    provider_descriptor& d0 = pc.response_provider_catalog_ProviderCatalog_provider_descriptor_m[0];
    setZ(d0.provider_descriptor_id, idA);
    setZ(d0.provider_descriptor_display_name, nameA);
    d0.provider_descriptor_kind.provider_kind_wire_choice =
        provider_kind_wire_r::provider_kind_wire_daemon_cloud_tstr_c;
    d0.provider_descriptor_wire_selector.provider_selector_choice =
        provider_selector_r::provider_selector_daemon_api_tstr_c;
    d0.provider_descriptor_requires_key = true;
    d0.provider_descriptor_supports_model_discovery = true;
    d0.provider_descriptor_default_base_url_choice =
        provider_descriptor::provider_descriptor_default_base_url_tstr_c;
    setZ(d0.provider_descriptor_default_base_url_tstr, baseA);

    provider_descriptor& d1 = pc.response_provider_catalog_ProviderCatalog_provider_descriptor_m[1];
    setZ(d1.provider_descriptor_id, idB);
    setZ(d1.provider_descriptor_display_name, nameB);
    d1.provider_descriptor_kind.provider_kind_wire_choice =
        provider_kind_wire_r::provider_kind_wire_cloud_tstr_c;
    d1.provider_descriptor_wire_selector.provider_selector_choice =
        provider_selector_r::provider_selector_genai_tstr_c;
    d1.provider_descriptor_requires_key = true;
    d1.provider_descriptor_supports_model_discovery = true;
    d1.provider_descriptor_default_base_url_choice =
        provider_descriptor::provider_descriptor_default_base_url_null_m_c;
    // [waveB:app-v30] CON-15: Anthropic advertises a generic sign-in (family + node label).
    static const QByteArray signFam = "anthropic_oauth", signLabel = "Sign in with Anthropic";
    d1.provider_descriptor_sign_in_present = true;
    d1.provider_descriptor_sign_in.provider_descriptor_sign_in_choice =
        provider_descriptor_sign_in_r::provider_descriptor_sign_in_provider_sign_in_m_c;
    setZ(d1.provider_descriptor_sign_in.provider_descriptor_sign_in_provider_sign_in_m
             .provider_sign_in_family,
         signFam);
    setZ(d1.provider_descriptor_sign_in.provider_descriptor_sign_in_provider_sign_in_m
             .provider_sign_in_label,
         signLabel);

    provider_descriptor& d2 = pc.response_provider_catalog_ProviderCatalog_provider_descriptor_m[2];
    setZ(d2.provider_descriptor_id, idC);
    setZ(d2.provider_descriptor_display_name, nameC);
    d2.provider_descriptor_kind.provider_kind_wire_choice =
        provider_kind_wire_r::provider_kind_wire_local_tstr_c;
    d2.provider_descriptor_wire_selector.provider_selector_choice =
        provider_selector_r::provider_selector_llama_cpp_tstr_c;
    d2.provider_descriptor_requires_key = false;
    d2.provider_descriptor_supports_model_discovery = true;
    d2.provider_descriptor_default_base_url_choice =
        provider_descriptor::provider_descriptor_default_base_url_null_m_c;
    return encodeResponse(*resp);
}

// A ProviderModels page (wire v25 model-page) with two genai-selector cloud models (one carrying
// a display_name), on the last page (no `next`).
QByteArray providerModelsResponse() {
    const QByteArray id0 = "claude-3-5-sonnet-latest", name0 = "Claude 3.5 Sonnet";
    const QByteArray id1 = "claude-3-opus-latest";
    auto resp = std::make_unique<api_response_r>();
    resp->api_response_choice = api_response_r::api_response_response_provider_models_m_c;
    model_page& pm =
        resp->api_response_response_provider_models_m.response_provider_models_ProviderModels;
    pm.model_page_items_model_descriptor_m_count = 2;
    pm.model_page_next_present = false;

    model_descriptor& m0 = pm.model_page_items_model_descriptor_m[0];
    setZ(m0.model_descriptor_id, id0);
    m0.model_descriptor_provider.provider_selector_choice =
        provider_selector_r::provider_selector_genai_tstr_c;
    m0.model_descriptor_display_name_present = true;
    m0.model_descriptor_display_name.model_descriptor_display_name_choice =
        model_descriptor_display_name_r::model_descriptor_display_name_tstr_c;
    setZ(m0.model_descriptor_display_name.model_descriptor_display_name_tstr, name0);
    m0.model_descriptor_context_length_choice =
        model_descriptor::model_descriptor_context_length_null_m_c;
    m0.model_descriptor_input_price_micros_per_mtok_choice =
        model_descriptor::model_descriptor_input_price_micros_per_mtok_null_m_c;
    m0.model_descriptor_output_price_micros_per_mtok_choice =
        model_descriptor::model_descriptor_output_price_micros_per_mtok_null_m_c;
    m0.model_descriptor_local = false;

    model_descriptor& m1 = pm.model_page_items_model_descriptor_m[1];
    setZ(m1.model_descriptor_id, id1);
    m1.model_descriptor_provider.provider_selector_choice =
        provider_selector_r::provider_selector_genai_tstr_c;
    m1.model_descriptor_display_name_present = false;
    m1.model_descriptor_context_length_choice =
        model_descriptor::model_descriptor_context_length_null_m_c;
    m1.model_descriptor_input_price_micros_per_mtok_choice =
        model_descriptor::model_descriptor_input_price_micros_per_mtok_null_m_c;
    m1.model_descriptor_output_price_micros_per_mtok_choice =
        model_descriptor::model_descriptor_output_price_micros_per_mtok_null_m_c;
    m1.model_descriptor_local = false;
    return encodeResponse(*resp);
}

} // namespace

// Guards the provider/model discovery seams: the codec facade round-trips ProviderCatalog /
// ProviderModels (incl. the id-string provider dimension, wire_selector, default_base_url, and the
// new model display_name), the shared local-compose helper + MockProviderCatalog shape, and the
// DaemonProviderCatalog over the WireMuxServer fixture.
class TestProviderCatalog : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { QVERIFY(m_tmp.isValid()); }

    // Codec round-trip: a ProviderCatalog response decodes to the descriptor list with the id
    // (discovery key) distinct from the wire_selector (ProviderSelector), plus kind/requires_key/
    // default_base_url.
    void providerCatalogRoundTrips() {
        const QByteArray resp = providerCatalogResponse();
        QVERIFY(!resp.isEmpty());
        QCOMPARE(NodeApiCodec::responseKind(resp),
                 daemonapp::daemon::ApiResponseKind::ProviderCatalog);
        QList<DecodedProviderDescriptor> out;
        QVERIFY(NodeApiCodec::decodeProviderCatalog(resp, &out));
        QCOMPARE(out.size(), 3);
        QCOMPARE(out.at(0).id, QStringLiteral("daemon_cloud"));
        QCOMPARE(out.at(0).kind, QStringLiteral("daemon_cloud"));
        QCOMPARE(out.at(0).wireSelector, QStringLiteral("daemon_api"));
        QVERIFY(out.at(0).requiresKey);
        QVERIFY(out.at(0).hasDefaultBaseUrl);
        QCOMPARE(out.at(0).defaultBaseUrl, QStringLiteral("https://api.daemon.ai/api/v1"));
        // Anthropic: a genai-selector cloud vendor disambiguated by its id, requires a key.
        QCOMPARE(out.at(1).id, QStringLiteral("anthropic"));
        QCOMPARE(out.at(1).kind, QStringLiteral("cloud"));
        QCOMPARE(out.at(1).wireSelector, QStringLiteral("genai"));
        QVERIFY(out.at(1).requiresKey);
        QVERIFY(!out.at(1).hasDefaultBaseUrl);
        QCOMPARE(out.at(2).id, QStringLiteral("llama_cpp"));
        QCOMPARE(out.at(2).kind, QStringLiteral("local"));
        QCOMPARE(out.at(2).wireSelector, QStringLiteral("llama_cpp"));
        // [waveB:app-v30] CON-15: the optional generic sign-in decodes (family + label); providers
        // without one carry none.
        QVERIFY(!out.at(0).hasSignIn);
        QVERIFY(out.at(1).hasSignIn);
        QCOMPARE(out.at(1).signInFamily, QStringLiteral("anthropic_oauth"));
        QCOMPARE(out.at(1).signInLabel, QStringLiteral("Sign in with Anthropic"));
        QVERIFY(!out.at(2).hasSignIn);
    }

    // [waveB:app-v30] CON-15: DaemonProviderCatalog projects signInFamily/signInLabel onto the row
    // (empty for providers with no sign-in); the picker keys its button off signInFamily.
    void catalogRowCarriesSignIn() {
        const QString sock = m_tmp.filePath(QStringLiteral("signin.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(providerCatalogResponse());

        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        ProviderRepository repo(&client, nullptr);
        DaemonProviderCatalog catalog(&repo, nullptr);

        QSignalSpy providersChanged(&catalog, &models::IProviderCatalog::providersChanged);
        repo.refreshProviders();
        QTRY_COMPARE_WITH_TIMEOUT(providersChanged.count() >= 1, true, 3000);
        const QVariantMap anthropic = catalog.descriptorFor(QStringLiteral("anthropic"));
        QCOMPARE(anthropic.value(QStringLiteral("signInFamily")).toString(),
                 QStringLiteral("anthropic_oauth"));
        QCOMPARE(anthropic.value(QStringLiteral("signInLabel")).toString(),
                 QStringLiteral("Sign in with Anthropic"));
        const QVariantMap cloud = catalog.descriptorFor(QStringLiteral("daemon_cloud"));
        QVERIFY(cloud.value(QStringLiteral("signInFamily")).toString().isEmpty());
    }

    // Codec round-trip: ProviderModels decodes with the optional display_name preserved (and the
    // id written verbatim, which the profile persists as ProfileSpec.model).
    void providerModelsRoundTrips() {
        const QByteArray resp = providerModelsResponse();
        QVERIFY(!resp.isEmpty());
        QCOMPARE(NodeApiCodec::responseKind(resp),
                 daemonapp::daemon::ApiResponseKind::ProviderModels);
        QList<DecodedModelDescriptor> out;
        QVERIFY(NodeApiCodec::decodeProviderModels(resp, &out));
        QCOMPARE(out.size(), 2);
        QCOMPARE(out.at(0).id, QStringLiteral("claude-3-5-sonnet-latest"));
        QVERIFY(out.at(0).hasDisplayName);
        QCOMPARE(out.at(0).displayName, QStringLiteral("Claude 3.5 Sonnet"));
        QCOMPARE(out.at(1).id, QStringLiteral("claude-3-opus-latest"));
        QVERIFY(!out.at(1).hasDisplayName);
    }

    // The pure local-compose helper: installed models served by the provider (engine-normalized),
    // then a trailing "Discover More Models" row; a fresh machine yields just the discover row.
    void localComposeInstalledPlusDiscover() {
        const QVariantList empty = models::composeLocalOfferedModels(
            {}, QStringLiteral("llama_cpp"), QStringLiteral("llama_cpp"));
        QCOMPARE(empty.size(), 1);
        QCOMPARE(empty.at(0).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("discover"));

        QList<QVariantMap> rows;
        QVariantMap a;
        a[QStringLiteral("id")] = QStringLiteral("llama-3-8b.gguf");
        a[QStringLiteral("name")] = QStringLiteral("Llama 3 8B");
        a[QStringLiteral("provider")] =
            QStringLiteral("llama"); // engine name -> llama_cpp selector
        rows.append(a);
        QVariantMap b; // a mistral.rs model must NOT match the llama_cpp provider
        b[QStringLiteral("id")] = QStringLiteral("mistral-7b");
        b[QStringLiteral("provider")] = QStringLiteral("mistral_rs");
        rows.append(b);

        const QVariantList out = models::composeLocalOfferedModels(
            rows, QStringLiteral("llama_cpp"), QStringLiteral("llama_cpp"));
        QCOMPARE(out.size(), 2); // the one llama model + the discover row
        QCOMPARE(out.at(0).toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("llama-3-8b.gguf"));
        QVERIFY(out.at(0).toMap().value(QStringLiteral("local")).toBool());
        QCOMPARE(out.at(1).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("discover"));
    }

    // A vision-projector companion (mmproj artifact, `projector: true`) is NEVER offered as a
    // chat model; rows without the marker key (all pre-marker callers) keep composing as before.
    void localComposeExcludesProjectorRows() {
        QList<QVariantMap> rows;
        QVariantMap chat; // no `projector` key at all — the pre-marker row shape
        chat[QStringLiteral("id")] = QStringLiteral("llama-3-8b.gguf");
        chat[QStringLiteral("name")] = QStringLiteral("Llama 3 8B");
        chat[QStringLiteral("provider")] = QStringLiteral("llama");
        rows.append(chat);
        QVariantMap projector; // the node's marker on an installed mmproj companion
        projector[QStringLiteral("id")] = QStringLiteral("mmproj-llama-3-8b.gguf");
        projector[QStringLiteral("name")] = QStringLiteral("Llama 3 8B (projector)");
        projector[QStringLiteral("provider")] = QStringLiteral("llama");
        projector[QStringLiteral("projector")] = true;
        rows.append(projector);
        QVariantMap marked; // an explicit false marker stays offerable
        marked[QStringLiteral("id")] = QStringLiteral("llama-3-70b.gguf");
        marked[QStringLiteral("name")] = QStringLiteral("Llama 3 70B");
        marked[QStringLiteral("provider")] = QStringLiteral("llama");
        marked[QStringLiteral("projector")] = false;
        rows.append(marked);

        const QVariantList out = models::composeLocalOfferedModels(
            rows, QStringLiteral("llama_cpp"), QStringLiteral("llama_cpp"));
        QCOMPARE(out.size(), 3); // both chat models + the discover row; no projector
        QCOMPARE(out.at(0).toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("llama-3-8b.gguf"));
        QCOMPARE(out.at(1).toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("llama-3-70b.gguf"));
        QCOMPARE(out.at(2).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("discover"));
        for (const QVariant& v : out) {
            QVERIFY(v.toMap().value(QStringLiteral("id")).toString() !=
                    QStringLiteral("mmproj-llama-3-8b.gguf"));
        }
    }

    // The mock provider catalog never offers Mock, defaults include Daemon Cloud, and a cloud
    // provider lists models after refreshModels().
    void mockCatalogExcludesMockAndListsCloud() {
        models::MockProviderCatalog cat;
        const QVariantList providers = cat.providers();
        QVERIFY(!providers.isEmpty());
        bool sawDaemonCloud = false;
        for (const QVariant& v : providers) {
            const QVariantMap m = v.toMap();
            QVERIFY(m.value(QStringLiteral("id")).toString() != QStringLiteral("mock"));
            QVERIFY(m.value(QStringLiteral("wireSelector")).toString() != QStringLiteral("mock"));
            if (m.value(QStringLiteral("id")).toString() == QStringLiteral("daemon_cloud")) {
                sawDaemonCloud = true;
            }
        }
        QVERIFY(sawDaemonCloud);

        QSignalSpy changed(&cat, &models::IProviderCatalog::offeredModelsChanged);
        cat.refreshModels(QStringLiteral("anthropic"), QString(), QStringLiteral("sk-test"));
        QCOMPARE(changed.count(), 1);
        QVERIFY(!cat.offeredModels(QStringLiteral("anthropic")).isEmpty());
    }

    // DaemonProviderCatalog over the wire: a ProviderCatalog answer populates providers() (Mock
    // excluded), and a ProviderModels answer populates a cloud provider's offeredModels() with the
    // ids persisted verbatim.
    void daemonCatalogOverMux() {
        const QString sock = m_tmp.filePath(QStringLiteral("providers.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(providerCatalogResponse());

        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        ProviderRepository repo(&client, nullptr);
        // No IModelCatalog needed for the cloud path.
        DaemonProviderCatalog catalog(&repo, nullptr);

        QSignalSpy providersChanged(&catalog, &models::IProviderCatalog::providersChanged);
        repo.refreshProviders();
        QTRY_COMPARE_WITH_TIMEOUT(providersChanged.count() >= 1, true, 3000);
        const QVariantList providers = catalog.providers();
        QCOMPARE(providers.size(), 3);
        QCOMPARE(providers.at(0).toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("daemon_cloud"));

        // Now list Anthropic's models (the WireMuxServer replies with the configured payload).
        fake.setReplyPayload(providerModelsResponse());
        QSignalSpy modelsChanged(&catalog, &models::IProviderCatalog::offeredModelsChanged);
        catalog.refreshModels(QStringLiteral("anthropic"), QString(), QStringLiteral("sk-test"));
        QTRY_COMPARE_WITH_TIMEOUT(modelsChanged.count() >= 1, true, 3000);
        const QVariantList models = catalog.offeredModels(QStringLiteral("anthropic"));
        QCOMPARE(models.size(), 2);
        QCOMPARE(models.at(0).toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("claude-3-5-sonnet-latest"));
        QCOMPARE(models.at(0).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("Claude 3.5 Sonnet"));
    }

private:
    QTemporaryDir m_tmp;
};

QTEST_MAIN(TestProviderCatalog)
#include "tst_provider_catalog.moc"
