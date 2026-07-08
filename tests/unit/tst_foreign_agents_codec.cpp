// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [integration:foreign-agents] Phase B codec extension: the new wire shapes the foreign-agent
// surfacing depends on, exercised against the freshly-regenerated vendored zcbor codec —
//   - ProfileSpec.foreign_backend (AgentNative / NodeProvider) encode + decode round-trips,
//   - AgentEntry.verification (Verified/Unverified/NotInstalled + absent->NotInstalled default),
//   - SessionDetail.model_selector decode,
//   - Gateway ops (GatewayGet/GatewaySet request arms + GatewayStatus decode),
//   - ProfileInfo provenance (created_by / owner),
//   - CapsReport.max_composed_profiles / max_ephemeral_per_session (present + absent),
//   - the ProfilesChanged node event.
// Health services parity is asserted too (the shape was verified unchanged vs the updated CDDL).

#include "daemon/node_api_codec.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"

#include <memory>
#include <QtTest/QtTest>

using daemonapp::daemon::AgentVerification;
using daemonapp::daemon::DecodedAgentEntry;
using daemonapp::daemon::DecodedCapsReport;
using daemonapp::daemon::DecodedEventsPage;
using daemonapp::daemon::DecodedForeignBackend;
using daemonapp::daemon::DecodedGatewayStatus;
using daemonapp::daemon::DecodedHealth;
using daemonapp::daemon::DecodedNodeEvent;
using daemonapp::daemon::DecodedProfileInfo;
using daemonapp::daemon::DecodedProfileSpec;
using daemonapp::daemon::DecodedSessionDetail;
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
    QByteArray out(8192, Qt::Uninitialized);
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

class TestForeignAgentsCodec : public QObject {
    Q_OBJECT

private slots:
    // --- 1. ProfileSpec.foreign_backend ---------------------------------------------------------
    void foreignBackendNodeProviderEncodes() {
        DecodedProfileSpec spec;
        spec.id = QStringLiteral("fp");
        spec.model = QStringLiteral("gpt-4o");
        spec.engineKind = QStringLiteral("Foreign");
        spec.engineForeignAgent = QStringLiteral("goose");
        spec.hasForeignBackend = true;
        spec.foreignBackend.kind = QStringLiteral("NodeProvider");
        spec.foreignBackend.nodeProvider = QStringLiteral("genai");
        spec.foreignBackend.nodeModel = QStringLiteral("claude-3-5");
        spec.foreignBackend.hasNodeCredentialRef = true;
        spec.foreignBackend.nodeCredentialRef = QStringLiteral("cred-ref");

        const auto req = decodeRequest(NodeApiCodec::encodeProfileCreateRequest(spec));
        QVERIFY(req);
        QCOMPARE(req->api_request_choice, api_request_r::api_request_request_profile_create_m_c);
        const profile_spec& ps = req->api_request_request_profile_create_m.ProfileCreate_spec;
        QVERIFY(ps.profile_spec_foreign_backend_present);
        const foreign_backend_r& fb = ps.profile_spec_foreign_backend.profile_spec_foreign_backend;
        QCOMPARE(fb.foreign_backend_choice, foreign_backend_r::foreign_backend_node_provider_m_c);
        const foreign_backend_node_provider& np = fb.foreign_backend_node_provider_m;
        QCOMPARE(fromZ(np.NodeProvider_model), QStringLiteral("claude-3-5"));
        QCOMPARE(np.NodeProvider_credential_ref_choice,
                 foreign_backend_node_provider::NodeProvider_credential_ref_tstr_c);
        QCOMPARE(fromZ(np.NodeProvider_credential_ref_tstr), QStringLiteral("cred-ref"));
    }

    void foreignBackendAgentNativeOmittedWhenUnset() {
        DecodedProfileSpec spec;
        spec.id = QStringLiteral("core");
        spec.model = QStringLiteral("m");
        // No foreign backend: a Core profile must leave the optional field ABSENT.
        const auto req = decodeRequest(NodeApiCodec::encodeProfileCreateRequest(spec));
        QVERIFY(req);
        QVERIFY(!req->api_request_request_profile_create_m.ProfileCreate_spec
                     .profile_spec_foreign_backend_present);
    }

    void foreignBackendDecodesFromProfileResponse() {
        // Build a Profile response carrying an AgentNative backend + operator provenance.
        QByteArray id = QByteArrayLiteral("fp");
        QByteArray model = QByteArrayLiteral("agent-model");
        QByteArray owner = QByteArrayLiteral("sess-123");
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_profile_m_c;
        response_profile& rp = resp->api_response_response_profile_m;
        rp.response_profile_Profile_choice =
            response_profile::response_profile_Profile_profile_spec_m_c;
        profile_spec& ps = rp.response_profile_Profile_profile_spec_m;
        setZ(ps.profile_spec_id, id);
        setZ(ps.profile_spec_model, model);
        ps.profile_spec_foreign_backend_present = true;
        foreign_backend_r& fb = ps.profile_spec_foreign_backend.profile_spec_foreign_backend;
        fb.foreign_backend_choice = foreign_backend_r::foreign_backend_agent_native_m_c;
        fb.foreign_backend_agent_native_m.AgentNative_model_choice =
            foreign_backend_agent_native::AgentNative_model_tstr_c;
        setZ(fb.foreign_backend_agent_native_m.AgentNative_model_tstr, model);
        ps.profile_spec_created_by_present = true;
        ps.profile_spec_created_by.profile_spec_created_by_choice =
            profile_spec_created_by_r::profile_spec_created_by_author_m_c;
        ps.profile_spec_created_by.profile_spec_created_by_author_m.author_choice =
            author_r::author_operator_tstr_c;
        ps.profile_spec_owner_present = true;
        ps.profile_spec_owner.profile_spec_owner_choice =
            profile_spec_owner_r::profile_spec_owner_tstr_c;
        setZ(ps.profile_spec_owner.profile_spec_owner_tstr, owner);

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        DecodedProfileSpec out;
        bool found = false;
        QVERIFY(NodeApiCodec::decodeProfile(bytes, &out, &found));
        QVERIFY(found);
        QVERIFY(out.hasForeignBackend);
        QCOMPARE(out.foreignBackend.kind, QStringLiteral("AgentNative"));
        QVERIFY(out.foreignBackend.hasAgentNativeModel);
        QCOMPARE(out.foreignBackend.agentNativeModel, QStringLiteral("agent-model"));
        QVERIFY(out.hasCreatedBy);
        QVERIFY(!out.createdByIsAgent);
        QCOMPARE(out.createdBy, QStringLiteral("operator"));
        QVERIFY(out.hasOwner);
        QCOMPARE(out.owner, QStringLiteral("sess-123"));
    }

    // --- 2. AgentEntry.verification -------------------------------------------------------------
    void agentVerificationDecodes() {
        QByteArray n0 = QByteArrayLiteral("gemini");
        QByteArray n1 = QByteArrayLiteral("goose");
        QByteArray n2 = QByteArrayLiteral("legacy"); // omits verification -> NotInstalled default
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_agent_catalog_m_c;
        response_agent_catalog& cat = resp->api_response_response_agent_catalog_m;
        cat.response_agent_catalog_AgentCatalog_agent_entry_m_count = 3;

        agent_entry& e0 = cat.response_agent_catalog_AgentCatalog_agent_entry_m[0];
        e0 = {};
        setZ(e0.agent_entry_name, n0);
        e0.agent_entry_source.agent_source_choice = agent_source_r::agent_source_Builtin_tstr_c;
        e0.agent_entry_verification_present = true;
        e0.agent_entry_verification.agent_entry_verification.agent_verification_choice =
            agent_verification_r::agent_verification_Verified_tstr_c;

        agent_entry& e1 = cat.response_agent_catalog_AgentCatalog_agent_entry_m[1];
        e1 = {};
        setZ(e1.agent_entry_name, n1);
        e1.agent_entry_source.agent_source_choice = agent_source_r::agent_source_Manual_tstr_c;
        e1.agent_entry_verification_present = true;
        e1.agent_entry_verification.agent_entry_verification.agent_verification_choice =
            agent_verification_r::agent_verification_Unverified_tstr_c;

        agent_entry& e2 = cat.response_agent_catalog_AgentCatalog_agent_entry_m[2];
        e2 = {};
        setZ(e2.agent_entry_name, n2);
        e2.agent_entry_source.agent_source_choice = agent_source_r::agent_source_Builtin_tstr_c;
        e2.agent_entry_verification_present = false;

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        QList<DecodedAgentEntry> out;
        QVERIFY(NodeApiCodec::decodeAgentCatalog(bytes, &out));
        QCOMPARE(out.size(), 3);
        QCOMPARE(out[0].verification, AgentVerification::Verified);
        QCOMPARE(out[1].verification, AgentVerification::Unverified);
        QCOMPARE(out[2].verification, AgentVerification::NotInstalled);
    }

    // --- 3. SessionDetail.model_selector --------------------------------------------------------
    void sessionDetailModelSelectorDecodes() {
        QByteArray sid = QByteArrayLiteral("s1");
        QByteArray optId = QByteArrayLiteral("model");
        QByteArray cur = QByteArrayLiteral("sonnet");
        QByteArray c0id = QByteArrayLiteral("sonnet");
        QByteArray c0lbl = QByteArrayLiteral("Claude Sonnet");
        QByteArray c1id = QByteArrayLiteral("opus");
        QByteArray c1lbl = QByteArrayLiteral("Claude Opus");
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_session_detail_m_c;
        response_session_detail& rsd = resp->api_response_response_session_detail_m;
        rsd.response_session_detail_SessionDetail_choice =
            response_session_detail::response_session_detail_SessionDetail_session_detail_m_c;
        session_detail& d = rsd.response_session_detail_SessionDetail_session_detail_m;
        setZ(d.session_detail_info.session_info_session, sid);
        // Phase E: the AgentNative-vs-NodeProvider fork rides SessionDetail.foreign_backend (there
        // is no `source` on the selector); a resident AgentNative session carries it alongside the
        // advertised Model selector.
        d.session_detail_foreign_backend_present = true;
        foreign_backend_r& fb = d.session_detail_foreign_backend.session_detail_foreign_backend;
        fb.foreign_backend_choice = foreign_backend_r::foreign_backend_agent_native_m_c;
        fb.foreign_backend_agent_native_m.AgentNative_model_choice =
            foreign_backend_agent_native::AgentNative_model_null_m_c;
        d.session_detail_model_selector_present = true;
        d.session_detail_model_selector.session_detail_model_selector_choice =
            session_detail_model_selector_r::session_detail_model_selector_model_selector_m_c;
        model_selector& sel =
            d.session_detail_model_selector.session_detail_model_selector_model_selector_m;
        setZ(sel.model_selector_option_id, optId);
        setZ(sel.model_selector_current, cur);
        sel.model_selector_choices_model_choice_m_count = 2;
        setZ(sel.model_selector_choices_model_choice_m[0].model_choice_id, c0id);
        setZ(sel.model_selector_choices_model_choice_m[0].model_choice_label, c0lbl);
        setZ(sel.model_selector_choices_model_choice_m[1].model_choice_id, c1id);
        setZ(sel.model_selector_choices_model_choice_m[1].model_choice_label, c1lbl);

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        DecodedSessionDetail out;
        bool found = false;
        QVERIFY(NodeApiCodec::decodeSessionDetail(bytes, &out, &found));
        QVERIFY(found);
        QVERIFY(out.hasModelSelector);
        QCOMPARE(out.modelSelector.optionId, QStringLiteral("model"));
        QCOMPARE(out.modelSelector.current, QStringLiteral("sonnet"));
        QCOMPARE(out.modelSelector.choices.size(), 2);
        QCOMPARE(out.modelSelector.choices[0].id, QStringLiteral("sonnet"));
        QCOMPARE(out.modelSelector.choices[1].label, QStringLiteral("Claude Opus"));
        // The facade previously dropped SessionDetail.foreign_backend; Phase E decodes it (the
        // AgentNative arm here) so the composer can fork the model picker on it.
        QVERIFY(out.hasForeignBackend);
        QCOMPARE(out.foreignBackend.kind, QStringLiteral("AgentNative"));
    }

    // A NodeProvider foreign session carries foreign_backend but NO advertised model_selector
    // (its choices are the node catalog): the decoder surfaces the backend, selector absent.
    void sessionDetailNodeProviderBackendDecodes() {
        QByteArray sid = QByteArrayLiteral("s2");
        QByteArray model = QByteArrayLiteral("gpt-5.3");
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_session_detail_m_c;
        response_session_detail& rsd = resp->api_response_response_session_detail_m;
        rsd.response_session_detail_SessionDetail_choice =
            response_session_detail::response_session_detail_SessionDetail_session_detail_m_c;
        session_detail& d = rsd.response_session_detail_SessionDetail_session_detail_m;
        setZ(d.session_detail_info.session_info_session, sid);
        d.session_detail_foreign_backend_present = true;
        foreign_backend_r& fb = d.session_detail_foreign_backend.session_detail_foreign_backend;
        fb.foreign_backend_choice = foreign_backend_r::foreign_backend_node_provider_m_c;
        foreign_backend_node_provider& np = fb.foreign_backend_node_provider_m;
        np.NodeProvider_provider.provider_selector_choice =
            provider_selector_r::provider_selector_genai_tstr_c;
        setZ(np.NodeProvider_model, model);
        np.NodeProvider_credential_ref_choice =
            foreign_backend_node_provider::NodeProvider_credential_ref_null_m_c;
        d.session_detail_model_selector_present = false;

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        DecodedSessionDetail out;
        bool found = false;
        QVERIFY(NodeApiCodec::decodeSessionDetail(bytes, &out, &found));
        QVERIFY(found);
        QVERIFY(out.hasForeignBackend);
        QCOMPARE(out.foreignBackend.kind, QStringLiteral("NodeProvider"));
        QCOMPARE(out.foreignBackend.nodeModel, QStringLiteral("gpt-5.3"));
        QVERIFY(!out.hasModelSelector);
    }

    // --- 4. Gateway ops -------------------------------------------------------------------------
    void gatewayGetRequestArm() {
        const auto req = decodeRequest(NodeApiCodec::encodeGatewayGetRequest());
        QVERIFY(req);
        QCOMPARE(req->api_request_choice, api_request_r::api_request_request_gateway_get_m_c);
    }

    void gatewaySetRequestRoundTrips() {
        const auto withAddr = decodeRequest(
            NodeApiCodec::encodeGatewaySetRequest(true, QStringLiteral("0.0.0.0:9999")));
        QVERIFY(withAddr);
        QCOMPARE(withAddr->api_request_choice, api_request_r::api_request_request_gateway_set_m_c);
        const request_gateway_set& set = withAddr->api_request_request_gateway_set_m;
        QVERIFY(set.GatewaySet_enabled);
        QVERIFY(set.GatewaySet_addr_present);
        QCOMPARE(set.GatewaySet_addr.GatewaySet_addr_choice,
                 GatewaySet_addr_r::GatewaySet_addr_tstr_c);
        QCOMPARE(fromZ(set.GatewaySet_addr.GatewaySet_addr_tstr), QStringLiteral("0.0.0.0:9999"));

        // An empty addr omits the optional field.
        const auto noAddr = decodeRequest(NodeApiCodec::encodeGatewaySetRequest(false));
        QVERIFY(noAddr);
        QVERIFY(!noAddr->api_request_request_gateway_set_m.GatewaySet_enabled);
        QVERIFY(!noAddr->api_request_request_gateway_set_m.GatewaySet_addr_present);
    }

    void gatewayStatusDecodes() {
        QByteArray addr = QByteArrayLiteral("127.0.0.1:8080");
        QByteArray err = QByteArrayLiteral("bind failed");
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_gateway_status_m_c;
        gateway_status& s =
            resp->api_response_response_gateway_status_m.response_gateway_status_GatewayStatus;
        s.gateway_status_enabled = true;
        s.gateway_status_listening = false;
        s.gateway_status_addr_choice = gateway_status::gateway_status_addr_tstr_c;
        setZ(s.gateway_status_addr_tstr, addr);
        s.gateway_status_last_error_choice = gateway_status::gateway_status_last_error_tstr_c;
        setZ(s.gateway_status_last_error_tstr, err);

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        QCOMPARE(NodeApiCodec::responseKind(bytes),
                 daemonapp::daemon::ApiResponseKind::GatewayStatus);
        DecodedGatewayStatus out;
        QVERIFY(NodeApiCodec::decodeGatewayStatus(bytes, &out));
        QVERIFY(out.enabled);
        QVERIFY(!out.listening);
        QVERIFY(out.hasAddr);
        QCOMPARE(out.addr, QStringLiteral("127.0.0.1:8080"));
        QVERIFY(out.hasLastError);
        QCOMPARE(out.lastError, QStringLiteral("bind failed"));
    }

    // --- 5. ProfileInfo provenance --------------------------------------------------------------
    void profileInfoProvenanceDecodes() {
        QByteArray id0 = QByteArrayLiteral("default");
        QByteArray id1 = QByteArrayLiteral("agent/s9/helper");
        QByteArray model = QByteArrayLiteral("m");
        QByteArray agent = QByteArrayLiteral("s9");
        QByteArray owner = QByteArrayLiteral("s9");
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_profiles_m_c;
        response_profiles& profs = resp->api_response_response_profiles_m;
        profs.response_profiles_Profiles_profile_info_m_count = 2;

        profile_info& p0 = profs.response_profiles_Profiles_profile_info_m[0];
        p0 = {};
        setZ(p0.profile_info_id, id0);
        setZ(p0.profile_info_model, model);
        p0.profile_info_is_active = true;
        // No provenance -> operator-authored default (both absent).

        profile_info& p1 = profs.response_profiles_Profiles_profile_info_m[1];
        p1 = {};
        setZ(p1.profile_info_id, id1);
        setZ(p1.profile_info_model, model);
        p1.profile_info_created_by_present = true;
        p1.profile_info_created_by.profile_info_created_by_choice =
            profile_info_created_by_r::profile_info_created_by_author_m_c;
        p1.profile_info_created_by.profile_info_created_by_author_m.author_choice =
            author_r::author_agent_m_c;
        setZ(p1.profile_info_created_by.profile_info_created_by_author_m.author_agent_m
                 .author_agent_agent,
             agent);
        p1.profile_info_owner_present = true;
        p1.profile_info_owner.profile_info_owner_choice =
            profile_info_owner_r::profile_info_owner_tstr_c;
        setZ(p1.profile_info_owner.profile_info_owner_tstr, owner);

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        QList<DecodedProfileInfo> out;
        QVERIFY(NodeApiCodec::decodeProfiles(bytes, &out));
        QCOMPARE(out.size(), 2);
        QVERIFY(!out[0].hasCreatedBy);
        QVERIFY(!out[0].hasOwner);
        QVERIFY(out[1].hasCreatedBy);
        QVERIFY(out[1].createdByIsAgent);
        QCOMPARE(out[1].createdBy, QStringLiteral("s9"));
        QVERIFY(out[1].hasOwner);
        QCOMPARE(out[1].owner, QStringLiteral("s9"));
    }

    // --- 6. CapsReport new ceilings -------------------------------------------------------------
    void capsReportNewCeilingsDecode() {
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_caps_m_c;
        caps_report& c = resp->api_response_response_caps_m.response_caps_Caps;
        c.caps_report_orchestrate_max_depth = 3;
        c.caps_report_orchestrate_max_fanout = 5;
        c.caps_report_max_composed_profiles_present = true;
        c.caps_report_max_composed_profiles.caps_report_max_composed_profiles = 7;
        c.caps_report_max_ephemeral_per_session_present = true;
        c.caps_report_max_ephemeral_per_session.caps_report_max_ephemeral_per_session = 9;

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        DecodedCapsReport out;
        QVERIFY(NodeApiCodec::decodeCaps(bytes, &out));
        QCOMPARE(out.orchestrateMaxDepth, 3u);
        QCOMPARE(out.orchestrateMaxFanout, 5u);
        QCOMPARE(out.maxComposedProfiles, 7u);
        QCOMPARE(out.maxEphemeralPerSession, 9u);
    }

    void capsReportOmittedCeilingsAreZero() {
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_caps_m_c;
        caps_report& c = resp->api_response_response_caps_m.response_caps_Caps;
        c.caps_report_orchestrate_max_depth = 2;
        c.caps_report_orchestrate_max_fanout = 4;
        c.caps_report_max_composed_profiles_present = false;
        c.caps_report_max_ephemeral_per_session_present = false;

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        DecodedCapsReport out;
        QVERIFY(NodeApiCodec::decodeCaps(bytes, &out));
        QCOMPARE(out.maxComposedProfiles, 0u);
        QCOMPARE(out.maxEphemeralPerSession, 0u);
    }

    // --- 7. ProfilesChanged node event ----------------------------------------------------------
    void profilesChangedEventDecodes() {
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_events_page_m_c;
        events_page& page =
            resp->api_response_response_events_page_m.response_events_page_EventsPage;
        page.events_page_events_node_event_m_count = 1;
        node_event_r& ev = page.events_page_events_node_event_m[0];
        ev.node_event_choice = node_event_r::node_event_profiles_changed_m_c;
        ev.node_event_profiles_changed_m.ProfilesChanged_rev = 42;
        page.events_page_next_cursor = 100;
        page.events_page_head_cursor = 100;

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        DecodedEventsPage out;
        QVERIFY(NodeApiCodec::decodeEventsPage(bytes, &out));
        QCOMPARE(out.events.size(), 1);
        QCOMPARE(out.events[0].kind, DecodedNodeEvent::Kind::ProfilesChanged);
        QCOMPARE(out.events[0].rev, quint64{42});
    }

    // --- 8. Health services parity (shape unchanged vs the updated CDDL) ------------------------
    void healthServicesDecode() {
        QByteArray n0 = QByteArrayLiteral("gateway");
        QByteArray d0 = QByteArrayLiteral("listening");
        auto resp = std::make_unique<api_response_r>();
        resp->api_response_choice = api_response_r::api_response_response_health_m_c;
        health_report& report = resp->api_response_response_health_m.response_health_Health;
        report.health_report_all_ok = true;
        report.health_report_services_service_health_m_count = 1;
        service_health& svc = report.health_report_services_service_health_m[0];
        svc = {};
        setZ(svc.service_health_name, n0);
        svc.service_health_ok = true;
        svc.service_health_restarts = 0;
        svc.service_health_detail_choice = service_health::service_health_detail_tstr_c;
        setZ(svc.service_health_detail_tstr, d0);

        const QByteArray bytes = encodeResponse(*resp);
        QVERIFY(!bytes.isEmpty());
        DecodedHealth out;
        QVERIFY(NodeApiCodec::decodeHealth(bytes, &out));
        QVERIFY(out.allOk);
        QCOMPARE(out.services.size(), 1);
        QCOMPARE(out.services[0].name, QStringLiteral("gateway"));
        QVERIFY(out.services[0].ok);
        QCOMPARE(out.services[0].detail, QStringLiteral("listening"));
    }
};

QTEST_MAIN(TestForeignAgentsCodec)
#include "tst_foreign_agents_codec.moc"
