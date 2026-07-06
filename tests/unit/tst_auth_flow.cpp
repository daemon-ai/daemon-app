// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// app-wizard-auth stream: the interactive-auth stack.
//   1. Codec round-trips for the new op families against the vendored zcbor codec: the auth
//      requests decode back arm-correct with their fields (family/params/redirect/bind), and the
//      auth responses decode into the Decoded* views (providers list w/ params schema, begun,
//      completed incl. the bound-profile arm). Same for AcpDiscover, ModelQuantize(s) +
//      QuantizeStatus states, and ModelInspect (the A6 ghost probe's success shape).
//   2. The shared AuthFlowController state machine over MockAuthFlowService: begin ->
//      awaiting_browser -> manual callback -> success (+ succeeded signal); begin/complete
//      failures land in `failed` with the reason; cancel drops the parked flow (AuthCancel);
//      an expired-on-arrival TTL fails closed. Tests run with useSink=false — offscreen runs
//      must never bind ports.

#include "auth/auth_flow_controller.h"
#include "auth/mock_auth_flow_service.h"
#include "daemon/node_api_codec.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"

#include <memory>
#include <QSignalSpy>
#include <QtTest/QtTest>

using auth::AuthFlowController;
using auth::MockAuthFlowService;
using daemonapp::daemon::DecodedAuthBeginResponse;
using daemonapp::daemon::DecodedAuthCompleteResponse;
using daemonapp::daemon::DecodedAuthProviderInfo;
using daemonapp::daemon::DecodedGgufInfo;
using daemonapp::daemon::DecodedQuantizeStatus;
using daemonapp::daemon::NodeApiCodec;

namespace {

void setZ(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

QString fromZ(const zcbor_string& z) {
    return QString::fromUtf8(reinterpret_cast<const char*>(z.value), static_cast<int>(z.len));
}

// Decode client-encoded request bytes with the generated request decoder (the node-side view),
// so an encoder test proves the full arm + field shape, not just "some bytes came out".
std::unique_ptr<api_request_r> decodeRequest(const QByteArray& bytes) {
    auto request = std::make_unique<api_request_r>();
    size_t consumed = 0;
    const int rc =
        cbor_decode_api_request(reinterpret_cast<const uint8_t*>(bytes.constData()),
                                static_cast<size_t>(bytes.size()), request.get(), &consumed);
    if (rc != ZCBOR_SUCCESS) {
        return nullptr;
    }
    return request;
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

} // namespace

class TstAuthFlow : public QObject {
    Q_OBJECT

private slots:
    // --- codec: auth requests ------------------------------------------------------------
    void authProvidersRequestRoundTrips() {
        const auto request = decodeRequest(NodeApiCodec::encodeAuthProvidersRequest());
        QVERIFY(request);
        QCOMPARE(request->api_request_choice,
                 api_request_r::api_request_request_auth_providers_m_c);
    }

    void authBeginRequestRoundTrips() {
        QVariantMap params;
        params[QStringLiteral("homeserver")] = QStringLiteral("https://matrix.example.org");
        const QByteArray bytes = NodeApiCodec::encodeAuthBeginRequest(
            QStringLiteral("matrix"), params, QStringLiteral("http://127.0.0.1:1234/cb"),
            QStringLiteral("agent-1"));
        const auto request = decodeRequest(bytes);
        QVERIFY(request);
        QCOMPARE(request->api_request_choice, api_request_r::api_request_request_auth_begin_m_c);
        const auth_begin_request& begin =
            request->api_request_request_auth_begin_m.request_auth_begin_AuthBegin;
        QCOMPARE(fromZ(begin.auth_begin_request_family), QStringLiteral("matrix"));
        QCOMPARE(begin.params_tstrtstr_count, static_cast<size_t>(1));
        QCOMPARE(fromZ(begin.params_tstrtstr[0].auth_begin_request_params_tstrtstr_key),
                 QStringLiteral("homeserver"));
        QCOMPARE(fromZ(begin.params_tstrtstr[0].params_tstrtstr),
                 QStringLiteral("https://matrix.example.org"));
        QCOMPARE(fromZ(begin.auth_begin_request_redirect_uri),
                 QStringLiteral("http://127.0.0.1:1234/cb"));
        QVERIFY(begin.auth_begin_request_bind_present);
        const auth_bind_request& bind =
            begin.auth_begin_request_bind.auth_begin_request_bind_auth_bind_request_m;
        QCOMPARE(fromZ(bind.auth_bind_request_profile), QStringLiteral("agent-1"));
    }

    void authBeginRequestOmitsEmptyBind() {
        const QByteArray bytes = NodeApiCodec::encodeAuthBeginRequest(
            QStringLiteral("matrix"), {}, QStringLiteral("urn:ietf:wg:oauth:2.0:oob"));
        const auto request = decodeRequest(bytes);
        QVERIFY(request);
        const auth_begin_request& begin =
            request->api_request_request_auth_begin_m.request_auth_begin_AuthBegin;
        QCOMPARE(begin.params_tstrtstr_count, static_cast<size_t>(0));
        QVERIFY(!begin.auth_begin_request_bind_present);
    }

    void authCompleteAndCancelRoundTrip() {
        const auto complete = decodeRequest(NodeApiCodec::encodeAuthCompleteRequest(
            QStringLiteral("flow-7"), QStringLiteral("loginToken=abc")));
        QVERIFY(complete);
        QCOMPARE(complete->api_request_choice,
                 api_request_r::api_request_request_auth_complete_m_c);
        const auth_complete_request& body =
            complete->api_request_request_auth_complete_m.request_auth_complete_AuthComplete;
        QCOMPARE(fromZ(body.auth_complete_request_flow_id), QStringLiteral("flow-7"));
        QCOMPARE(fromZ(body.auth_complete_request_callback), QStringLiteral("loginToken=abc"));

        const auto cancel =
            decodeRequest(NodeApiCodec::encodeAuthCancelRequest(QStringLiteral("flow-7")));
        QVERIFY(cancel);
        QCOMPARE(cancel->api_request_choice, api_request_r::api_request_request_auth_cancel_m_c);
        QCOMPARE(fromZ(cancel->api_request_request_auth_cancel_m.AuthCancel_flow_id),
                 QStringLiteral("flow-7"));
    }

    // --- codec: auth responses -----------------------------------------------------------
    void authProvidersResponseDecodes() {
        const QByteArray family = "matrix", display = "Matrix (SSO)", key = "homeserver",
                         label = "Homeserver";
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_auth_providers_m_c;
        response_auth_providers& providers = resp->api_response_response_auth_providers_m;
        providers.response_auth_providers_AuthProviders_auth_provider_info_m_count = 1;
        auth_provider_info& info =
            providers.response_auth_providers_AuthProviders_auth_provider_info_m[0];
        setZ(info.auth_provider_info_family, family);
        info.auth_provider_info_flow_kind.auth_flow_kind_choice =
            auth_flow_kind_r::auth_flow_kind_MatrixSso_tstr_c;
        setZ(info.auth_provider_info_display_name, display);
        info.auth_provider_info_params_schema_auth_param_field_m_count = 1;
        auth_param_field& field = info.auth_provider_info_params_schema_auth_param_field_m[0];
        setZ(field.auth_param_field_key, key);
        setZ(field.auth_param_field_label, label);
        field.auth_param_field_required = true;

        QList<DecodedAuthProviderInfo> out;
        QVERIFY(NodeApiCodec::decodeAuthProviders(encodeResponse(*resp), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out[0].family, QStringLiteral("matrix"));
        QCOMPARE(out[0].flowKind, QStringLiteral("MatrixSso"));
        QCOMPARE(out[0].displayName, QStringLiteral("Matrix (SSO)"));
        QCOMPARE(out[0].paramsSchema.size(), 1);
        QCOMPARE(out[0].paramsSchema[0].key, QStringLiteral("homeserver"));
        QVERIFY(out[0].paramsSchema[0].required);
        QCOMPARE(NodeApiCodec::responseKind(encodeResponse(*resp)),
                 daemonapp::daemon::ApiResponseKind::AuthProviders);
    }

    void authBegunResponseDecodes() {
        const QByteArray flow = "flow-1", url = "https://hs/_matrix/sso?x=1",
                         redirect = "http://127.0.0.1:9/cb";
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_auth_begun_m_c;
        auth_begin_response& begun =
            resp->api_response_response_auth_begun_m.response_auth_begun_AuthBegun;
        setZ(begun.auth_begin_response_flow_id, flow);
        setZ(begun.auth_begin_response_authorization_url, url);
        setZ(begun.auth_begin_response_redirect_uri, redirect);
        begun.auth_begin_response_expires_at = 1234567890;
        begun.auth_begin_response_flow_kind.auth_flow_kind_choice =
            auth_flow_kind_r::auth_flow_kind_OAuth2Pkce_tstr_c;

        DecodedAuthBeginResponse out;
        QVERIFY(NodeApiCodec::decodeAuthBegun(encodeResponse(*resp), &out));
        QCOMPARE(out.flowId, QStringLiteral("flow-1"));
        QCOMPARE(out.authorizationUrl, QStringLiteral("https://hs/_matrix/sso?x=1"));
        QCOMPARE(out.expiresAt, static_cast<quint64>(1234567890));
        QCOMPARE(out.flowKind, QStringLiteral("OAuth2Pkce"));
    }

    void authCompletedResponseDecodes() {
        const QByteArray cred = "cred/matrix", labelBytes = "@u:hs.org",
                         instance = "matrix/@u:hs.org", bound = "agent-1";
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_auth_completed_m_c;
        auth_complete_response& completed =
            resp->api_response_response_auth_completed_m.response_auth_completed_AuthCompleted;
        setZ(completed.auth_complete_response_credential_ref, cred);
        setZ(completed.auth_complete_response_account_label, labelBytes);
        setZ(completed.auth_complete_response_transport_instance, instance);
        completed.auth_complete_response_bound_profile_choice =
            auth_complete_response::auth_complete_response_bound_profile_tstr_c;
        setZ(completed.auth_complete_response_bound_profile_tstr, bound);

        DecodedAuthCompleteResponse out;
        QVERIFY(NodeApiCodec::decodeAuthCompleted(encodeResponse(*resp), &out));
        QCOMPARE(out.credentialRef, QStringLiteral("cred/matrix"));
        QCOMPARE(out.accountLabel, QStringLiteral("@u:hs.org"));
        QCOMPARE(out.transportInstance, QStringLiteral("matrix/@u:hs.org"));
        QVERIFY(out.hasBoundProfile);
        QCOMPARE(out.boundProfile, QStringLiteral("agent-1"));
    }

    // --- codec: AgentDiscover + quantize + inspect (the wizard vertical's ops) --------------
    void agentDiscoverRequestRoundTrips() {
        const auto request = decodeRequest(NodeApiCodec::encodeAgentDiscoverRequest());
        QVERIFY(request);
        QCOMPARE(request->api_request_choice,
                 api_request_r::api_request_request_agent_discover_m_c);
    }

    void modelQuantizeRequestsRoundTrip() {
        const auto quantize = decodeRequest(NodeApiCodec::encodeModelQuantizeRequest(
            QStringLiteral("org/repo"), QStringLiteral("Q4_K_M"), QStringLiteral("weights.gguf")));
        QVERIFY(quantize);
        QCOMPARE(quantize->api_request_choice,
                 api_request_r::api_request_request_model_quantize_m_c);
        const model_quantize_args& args =
            quantize->api_request_request_model_quantize_m.request_model_quantize_ModelQuantize;
        QCOMPARE(fromZ(args.model_quantize_args_repo), QStringLiteral("org/repo"));
        QCOMPARE(fromZ(args.model_quantize_args_target_quant), QStringLiteral("Q4_K_M"));
        QCOMPARE(args.model_quantize_args_source_file_choice,
                 model_quantize_args::model_quantize_args_source_file_tstr_c);
        QCOMPARE(fromZ(args.model_quantize_args_source_file_tstr), QStringLiteral("weights.gguf"));
        QCOMPARE(args.model_quantize_args_revision_choice,
                 model_quantize_args::model_quantize_args_revision_null_m_c);

        const auto quantizes = decodeRequest(NodeApiCodec::encodeModelQuantizesRequest());
        QVERIFY(quantizes);
        QCOMPARE(quantizes->api_request_choice,
                 api_request_r::api_request_request_model_quantizes_m_c);

        const auto inspect =
            decodeRequest(NodeApiCodec::encodeModelInspectRequest(QStringLiteral("model-1")));
        QVERIFY(inspect);
        QCOMPARE(inspect->api_request_choice, api_request_r::api_request_request_model_inspect_m_c);
        QCOMPARE(fromZ(inspect->api_request_request_model_inspect_m.ModelInspect_id),
                 QStringLiteral("model-1"));
    }

    void modelQuantizeResponsesDecode() {
        auto started = std::make_unique<api_response_r>();
        started->api_response_choice =
            api_response_r::api_response_response_model_quantize_started_m_c;
        started->api_response_response_model_quantize_started_m
            .response_model_quantize_started_ModelQuantizeStarted = 42;
        quint64 id = 0;
        QVERIFY(NodeApiCodec::decodeModelQuantizeStarted(encodeResponse(*started), &id));
        QCOMPARE(id, static_cast<quint64>(42));

        const QByteArray repo = "org/repo", source = "weights.gguf", target = "Q4_K_M",
                         err = "quantize binary missing";
        auto jobs = std::make_unique<api_response_r>();
        jobs->api_response_choice = api_response_r::api_response_response_model_quantizes_m_c;
        response_model_quantizes& body = jobs->api_response_response_model_quantizes_m;
        body.response_model_quantizes_ModelQuantizes_quantize_status_m_count = 1;
        quantize_status& job = body.response_model_quantizes_ModelQuantizes_quantize_status_m[0];
        job.quantize_status_id = 42;
        setZ(job.quantize_status_repo, repo);
        setZ(job.quantize_status_source_file, source);
        setZ(job.quantize_status_target_quant, target);
        job.quantize_status_state.quantize_state_choice =
            quantize_state_r::quantize_state_Failed_tstr_c;
        job.quantize_status_output_path_choice =
            quantize_status::quantize_status_output_path_null_m_c;
        job.quantize_status_model_id_choice = quantize_status::quantize_status_model_id_null_m_c;
        job.quantize_status_error_choice = quantize_status::quantize_status_error_tstr_c;
        setZ(job.quantize_status_error_tstr, err);

        QList<DecodedQuantizeStatus> out;
        QVERIFY(NodeApiCodec::decodeModelQuantizes(encodeResponse(*jobs), &out));
        QCOMPARE(out.size(), 1);
        QCOMPARE(out[0].id, static_cast<quint64>(42));
        QCOMPARE(out[0].repo, QStringLiteral("org/repo"));
        QCOMPARE(out[0].state, QStringLiteral("Failed"));
        QCOMPARE(out[0].error, QStringLiteral("quantize binary missing"));
    }

    void modelInspectResponseDecodes() {
        const QByteArray arch = "llama", nameBytes = "SmolLM2", fileType = "Q4_K_M";
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_model_inspect_m_c;
        gguf_info& info =
            resp->api_response_response_model_inspect_m.response_model_inspect_ModelInspect;
        info.gguf_info_architecture_choice = gguf_info::gguf_info_architecture_tstr_c;
        setZ(info.gguf_info_architecture_tstr, arch);
        info.gguf_info_name_choice = gguf_info::gguf_info_name_tstr_c;
        setZ(info.gguf_info_name_tstr, nameBytes);
        info.gguf_info_file_type_choice = gguf_info::gguf_info_file_type_tstr_c;
        setZ(info.gguf_info_file_type_tstr, fileType);
        info.gguf_info_context_length_choice = gguf_info::gguf_info_context_length_uint_c;
        info.gguf_info_context_length_uint = 8192;
        info.gguf_info_block_count_choice = gguf_info::gguf_info_block_count_null_m_c;
        info.gguf_info_quantization_version_choice =
            gguf_info::gguf_info_quantization_version_null_m_c;
        info.gguf_info_parameter_count_choice = gguf_info::gguf_info_parameter_count_null_m_c;
        info.gguf_info_size_bytes = 123456;

        DecodedGgufInfo out;
        QVERIFY(NodeApiCodec::decodeModelInspect(encodeResponse(*resp), &out));
        QCOMPARE(out.architecture, QStringLiteral("llama"));
        QCOMPARE(out.name, QStringLiteral("SmolLM2"));
        QVERIFY(out.hasContextLength);
        QCOMPARE(out.contextLength, static_cast<quint32>(8192));
        QCOMPARE(out.sizeBytes, static_cast<quint64>(123456));
    }

    // --- the shared AuthFlowController state machine (mock seam; no ports, no browser) ----
    void controllerHappyPath() {
        MockAuthFlowService service;
        AuthFlowController controller(&service);
        QSignalSpy succeeded(&controller, &AuthFlowController::succeeded);

        QVariantMap params;
        params[QStringLiteral("homeserver")] = QStringLiteral("hs.example.org");
        controller.start(QStringLiteral("matrix"), params, QString(), /*useSink=*/false);
        QCOMPARE(controller.phase(), QStringLiteral("beginning"));
        QTRY_COMPARE(controller.phase(), QStringLiteral("awaiting_browser"));
        QVERIFY(controller.authorizationUrl().contains(QStringLiteral("hs.example.org")));
        QVERIFY(!controller.sinkListening()); // useSink=false must never bind a port

        controller.submitCallback(QStringLiteral("loginToken=tok123"));
        QCOMPARE(controller.phase(), QStringLiteral("completing"));
        QTRY_COMPARE(controller.phase(), QStringLiteral("success"));
        QCOMPARE(controller.accountLabel(), QStringLiteral("@mock:example.org"));
        QCOMPARE(succeeded.count(), 1);
    }

    void controllerBeginFailure() {
        MockAuthFlowService service;
        AuthFlowController controller(&service);
        service.failNextBegin(QStringLiteral("homeserver unreachable"));
        controller.start(QStringLiteral("matrix"), {}, QString(), /*useSink=*/false);
        QTRY_COMPARE(controller.phase(), QStringLiteral("failed"));
        QCOMPARE(controller.error(), QStringLiteral("homeserver unreachable"));
    }

    void controllerCompleteFailureThenRetry() {
        MockAuthFlowService service;
        AuthFlowController controller(&service);
        service.failNextComplete(QStringLiteral("flow expired"));
        controller.start(QStringLiteral("matrix"), {}, QString(), /*useSink=*/false);
        QTRY_COMPARE(controller.phase(), QStringLiteral("awaiting_browser"));
        controller.submitCallback(QStringLiteral("loginToken=stale"));
        QTRY_COMPARE(controller.phase(), QStringLiteral("failed"));
        QCOMPARE(controller.error(), QStringLiteral("flow expired"));

        // Retry: a fresh start supersedes the failed flow and can succeed.
        controller.start(QStringLiteral("matrix"), {}, QString(), /*useSink=*/false);
        QTRY_COMPARE(controller.phase(), QStringLiteral("awaiting_browser"));
        controller.submitCallback(QStringLiteral("loginToken=fresh"));
        QTRY_COMPARE(controller.phase(), QStringLiteral("success"));
    }

    void controllerCancelDropsFlow() {
        MockAuthFlowService service;
        AuthFlowController controller(&service);
        controller.start(QStringLiteral("matrix"), {}, QString(), /*useSink=*/false);
        QTRY_COMPARE(controller.phase(), QStringLiteral("awaiting_browser"));
        controller.cancel();
        QCOMPARE(controller.phase(), QStringLiteral("cancelled"));
        QVERIFY(service.lastCancelled().startsWith(QStringLiteral("mock-flow-")));
        // A cancelled flow's late callback must not resurrect it.
        controller.submitCallback(QStringLiteral("loginToken=late"));
        QCOMPARE(controller.phase(), QStringLiteral("cancelled"));
    }

    void controllerExpiredOnArrivalFailsClosed() {
        MockAuthFlowService service;
        AuthFlowController controller(&service);
        service.setNextExpiresAt(1); // 1970: expired long before arrival
        controller.start(QStringLiteral("matrix"), {}, QString(), /*useSink=*/false);
        QTRY_COMPARE(controller.phase(), QStringLiteral("failed"));
        QVERIFY(controller.error().contains(QStringLiteral("expired")));
        QVERIFY(!service.lastCancelled().isEmpty()); // the parked flow was dropped remotely
    }
};

QTEST_GUILESS_MAIN(TstAuthFlow)
#include "tst_auth_flow.moc"
