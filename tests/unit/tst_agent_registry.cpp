// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [wave2:app-engines] Foreign-agent registry codec (C1/ENG-3): the AgentRegister / AgentRemove /
// AgentCatalog / AgentDiscover op family against the vendored zcbor codec. Register requests decode
// back arm-correct with source=Manual, the protocol tag, and the stdio (program/args/env) OR
// endpoint recipe; installed/version/capabilities stay ABSENT (the node re-probes — ENG-3). The
// AgentCatalog response decodes into DecodedAgentEntry rows carrying the wire protocol (v29).

#include "daemon/node_api_codec.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"

#include <memory>
#include <QtTest/QtTest>

using daemonapp::daemon::DecodedAgentEntry;
using daemonapp::daemon::DecodedAgentRecipeInput;
using daemonapp::daemon::NodeApiCodec;

namespace {

QString fromZ(const zcbor_string& z) {
    return QString::fromUtf8(reinterpret_cast<const char*>(z.value), static_cast<int>(z.len));
}

void setZ(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

std::unique_ptr<api_request_r> decodeRequest(const QByteArray& bytes) {
    auto request = std::make_unique<api_request_r>();
    size_t consumed = 0;
    const int rc =
        cbor_decode_api_request(reinterpret_cast<const uint8_t*>(bytes.constData()),
                                static_cast<size_t>(bytes.size()), request.get(), &consumed);
    return rc == ZCBOR_SUCCESS ? std::move(request) : nullptr;
}

QByteArray encodeResponse(const api_response_r& resp) {
    QByteArray out(4096, Qt::Uninitialized);
    size_t written = 0;
    const int rc = cbor_encode_api_response(reinterpret_cast<uint8_t*>(out.data()),
                                            static_cast<size_t>(out.size()), &resp, &written);
    if (rc != ZCBOR_SUCCESS) {
        return {};
    }
    out.truncate(static_cast<int>(written));
    return out;
}

} // namespace

class TestAgentRegistry : public QObject {
    Q_OBJECT

private slots:
    void catalogRequestArm() {
        const auto req = decodeRequest(NodeApiCodec::encodeAgentCatalogRequest());
        QVERIFY(req);
        QCOMPARE(req->api_request_choice, api_request_r::api_request_request_agent_catalog_m_c);
    }

    void discoverRequestArm() {
        const auto req = decodeRequest(NodeApiCodec::encodeAgentDiscoverRequest());
        QVERIFY(req);
        QCOMPARE(req->api_request_choice, api_request_r::api_request_request_agent_discover_m_c);
    }

    void registerStdioRoundTrips() {
        DecodedAgentRecipeInput in;
        in.name = QStringLiteral("my-claude");
        in.protocol = QStringLiteral("StreamJson");
        in.program = QStringLiteral("claude");
        in.args = {QStringLiteral("--output-format"), QStringLiteral("stream-json")};
        in.env = {{QStringLiteral("KEY"), QStringLiteral("VAL")}};
        const QByteArray bytes = NodeApiCodec::encodeAgentRegisterRequest(in);
        const auto req = decodeRequest(bytes);
        QVERIFY(req);
        QCOMPARE(req->api_request_choice, api_request_r::api_request_request_agent_register_m_c);
        const agent_entry& e = req->api_request_request_agent_register_m.AgentRegister_entry;
        QCOMPARE(fromZ(e.agent_entry_name), QStringLiteral("my-claude"));
        // The client always sends source=Manual (the node forces it regardless).
        QCOMPARE(e.agent_entry_source.agent_source_choice,
                 agent_source_r::agent_source_Manual_tstr_c);
        // Protocol tag (v29): stream-json.
        QVERIFY(e.agent_entry_protocol_present);
        QCOMPARE(e.agent_entry_protocol.agent_entry_protocol.agent_protocol_choice,
                 agent_protocol_r::agent_protocol_StreamJson_tstr_c);
        // installed is NEVER sent — the node re-probes (ENG-3).
        QVERIFY(!e.agent_entry_installed_present);
        // Recipe: program + args + env, no endpoint.
        QVERIFY(e.agent_entry_recipe.agent_recipe_program_present);
        QCOMPARE(fromZ(e.agent_entry_recipe.agent_recipe_program.agent_recipe_program_tstr),
                 QStringLiteral("claude"));
        QVERIFY(e.agent_entry_recipe.agent_recipe_args_present);
        QCOMPARE(e.agent_entry_recipe.agent_recipe_args.agent_recipe_args_tstr_count, size_t{2});
        QCOMPARE(fromZ(e.agent_entry_recipe.agent_recipe_args.agent_recipe_args_tstr[1]),
                 QStringLiteral("stream-json"));
        QVERIFY(e.agent_entry_recipe.agent_recipe_env_present);
        QCOMPARE(e.agent_entry_recipe.agent_recipe_env.agent_recipe_env_kv_pair_m_count, size_t{1});
        const kv_pair& kv = e.agent_entry_recipe.agent_recipe_env.agent_recipe_env_kv_pair_m[0];
        QCOMPARE(fromZ(kv.kv_pair_k), QStringLiteral("KEY"));
        QCOMPARE(fromZ(kv.kv_pair_v), QStringLiteral("VAL"));
        QVERIFY(!e.agent_entry_recipe.agent_recipe_endpoint_present);
    }

    void registerEndpointRoundTrips() {
        DecodedAgentRecipeInput in;
        in.name = QStringLiteral("remote");
        in.protocol = QStringLiteral("Acp");
        in.endpoint = QStringLiteral("tcp://host:9999");
        const QByteArray bytes = NodeApiCodec::encodeAgentRegisterRequest(in);
        const auto req = decodeRequest(bytes);
        QVERIFY(req);
        const agent_entry& e = req->api_request_request_agent_register_m.AgentRegister_entry;
        QCOMPARE(e.agent_entry_protocol.agent_entry_protocol.agent_protocol_choice,
                 agent_protocol_r::agent_protocol_Acp_tstr_c);
        QVERIFY(!e.agent_entry_recipe.agent_recipe_program_present);
        QVERIFY(e.agent_entry_recipe.agent_recipe_endpoint_present);
        QCOMPARE(fromZ(e.agent_entry_recipe.agent_recipe_endpoint.agent_recipe_endpoint_tstr),
                 QStringLiteral("tcp://host:9999"));
    }

    void removeRoundTrips() {
        const auto req =
            decodeRequest(NodeApiCodec::encodeAgentRemoveRequest(QStringLiteral("goose")));
        QVERIFY(req);
        QCOMPARE(req->api_request_choice, api_request_r::api_request_request_agent_remove_m_c);
        QCOMPARE(fromZ(req->api_request_request_agent_remove_m.AgentRemove_name),
                 QStringLiteral("goose"));
    }

    void decodeCatalogReadsProtocol() {
        // Two rows: a curated ACP builtin (installed, versioned) + a manual stream-json
        // (uninstalled).
        QByteArray n0 = QByteArrayLiteral("gemini");
        QByteArray v0 = QByteArrayLiteral("0.4");
        QByteArray n1 = QByteArrayLiteral("my-claude");
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_agent_catalog_m_c;
        response_agent_catalog& cat = resp->api_response_response_agent_catalog_m;
        cat.response_agent_catalog_AgentCatalog_agent_entry_m_count = 2;

        agent_entry& e0 = cat.response_agent_catalog_AgentCatalog_agent_entry_m[0];
        e0 = {};
        setZ(e0.agent_entry_name, n0);
        e0.agent_entry_source.agent_source_choice = agent_source_r::agent_source_Builtin_tstr_c;
        e0.agent_entry_protocol_present = true;
        e0.agent_entry_protocol.agent_entry_protocol.agent_protocol_choice =
            agent_protocol_r::agent_protocol_Acp_tstr_c;
        e0.agent_entry_installed_present = true;
        e0.agent_entry_installed.agent_entry_installed = true;
        e0.agent_entry_version_present = true;
        e0.agent_entry_version.agent_entry_version_choice =
            agent_entry_version_r::agent_entry_version_tstr_c;
        setZ(e0.agent_entry_version.agent_entry_version_tstr, v0);

        agent_entry& e1 = cat.response_agent_catalog_AgentCatalog_agent_entry_m[1];
        e1 = {};
        setZ(e1.agent_entry_name, n1);
        e1.agent_entry_source.agent_source_choice = agent_source_r::agent_source_Manual_tstr_c;
        e1.agent_entry_protocol_present = true;
        e1.agent_entry_protocol.agent_entry_protocol.agent_protocol_choice =
            agent_protocol_r::agent_protocol_StreamJson_tstr_c;
        e1.agent_entry_installed_present = true;
        e1.agent_entry_installed.agent_entry_installed = false;

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        QList<DecodedAgentEntry> out;
        QVERIFY(NodeApiCodec::decodeAgentCatalog(bytes, &out));
        QCOMPARE(out.size(), 2);
        QCOMPARE(out[0].name, QStringLiteral("gemini"));
        QCOMPARE(out[0].source, QStringLiteral("Builtin"));
        QCOMPARE(out[0].protocol, QStringLiteral("Acp"));
        QVERIFY(out[0].installed);
        QCOMPARE(out[0].version, QStringLiteral("0.4"));
        QCOMPARE(out[1].name, QStringLiteral("my-claude"));
        QCOMPARE(out[1].source, QStringLiteral("Manual"));
        QCOMPARE(out[1].protocol, QStringLiteral("StreamJson"));
        QVERIFY(!out[1].installed);
    }
};

QTEST_MAIN(TestAgentRegistry)
#include "tst_agent_registry.moc"
