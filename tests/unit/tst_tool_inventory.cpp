// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [wave2:app-approvals-safety] D2: wire + facade guard for the read-only tool inventory (ToolList
// -> Tools, wire v29). Encodes a Tools response with the vendored codec, decodes it back through
// NodeApiCodec::decodeTools, and asserts the enabled/requires projection; also checks the mock
// facade's row contract
// (name/description/enabled/requires/requirementLabel/requirementIsCredential).

#include "daemon/node_api_codec.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "tools/mock_tool_inventory.h"
#include "uimodels/variant_list_model.h"

#include <QtTest/QtTest>
#include <vector>

using daemonapp::daemon::DecodedToolInfo;
using daemonapp::daemon::NodeApiCodec;

namespace {

void setStr(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

// Encode a Tools response carrying `tools` (name, description, enabled, requires-or-empty).
QByteArray encodeToolsResponse(const std::vector<DecodedToolInfo>& tools,
                               std::vector<QByteArray>& scratch) {
    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_tools_m_c;
    response_tools& out = resp.api_response_response_tools_m;
    out.response_tools_Tools_tool_info_m_count = tools.size();
    for (size_t i = 0; i < tools.size(); ++i) {
        tool_info& ti = out.response_tools_Tools_tool_info_m[i];
        scratch.push_back(tools[i].name.toUtf8());
        setStr(ti.tool_info_name, scratch.back());
        if (tools[i].description.isEmpty()) {
            ti.tool_info_description_present = false;
        } else {
            ti.tool_info_description_present = true;
            ti.tool_info_description.tool_info_description_choice =
                tool_info_description_r::tool_info_description_tstr_c;
            scratch.push_back(tools[i].description.toUtf8());
            setStr(ti.tool_info_description.tool_info_description_tstr, scratch.back());
        }
        ti.tool_info_enabled = tools[i].enabled;
        if (tools[i].requires_.isEmpty()) {
            ti.tool_info_requires_present = false;
        } else {
            ti.tool_info_requires_present = true;
            ti.tool_info_requires.tool_info_requires_choice =
                tool_info_requires_r::tool_info_requires_tstr_c;
            scratch.push_back(tools[i].requires_.toUtf8());
            setStr(ti.tool_info_requires.tool_info_requires_tstr, scratch.back());
        }
    }
    QByteArray buf(8192, Qt::Uninitialized);
    size_t written = 0;
    const int rc = cbor_encode_api_response(reinterpret_cast<uint8_t*>(buf.data()),
                                            static_cast<size_t>(buf.size()), &resp, &written);
    if (rc != ZCBOR_SUCCESS) {
        return {};
    }
    buf.truncate(static_cast<int>(written));
    return buf;
}

} // namespace

class TestToolInventory : public QObject {
    Q_OBJECT

private slots:
    // A Tools response decodes into the enabled/requires projection, honestly carrying an unmet
    // requirement for a disabled tool and none for an enabled one.
    void decodesEnabledAndRequires() {
        std::vector<DecodedToolInfo> tools;
        DecodedToolInfo fs;
        fs.name = QStringLiteral("fs");
        fs.description = QStringLiteral("files");
        fs.enabled = true;
        tools.push_back(fs);
        DecodedToolInfo web;
        web.name = QStringLiteral("web_search");
        web.description = QStringLiteral("search");
        web.enabled = false;
        web.requires_ = QStringLiteral("tavily");
        tools.push_back(web);

        std::vector<QByteArray> scratch;
        const QByteArray cbor = encodeToolsResponse(tools, scratch);
        QVERIFY(!cbor.isEmpty());

        QList<DecodedToolInfo> decoded;
        QVERIFY(NodeApiCodec::decodeTools(cbor, &decoded));
        QCOMPARE(decoded.size(), 2);
        QCOMPARE(decoded[0].name, QStringLiteral("fs"));
        QVERIFY(decoded[0].enabled);
        QVERIFY(decoded[0].requires_.isEmpty());
        QCOMPARE(decoded[1].name, QStringLiteral("web_search"));
        QVERIFY(!decoded[1].enabled);
        QCOMPARE(decoded[1].requires_, QStringLiteral("tavily"));
    }

    // The mock facade projects rows with the full display contract the QML/TUI bind to, including
    // a credential-shaped requirement flag (drives the "Set up…" deep-link) for a keyed tool.
    void mockFacadeRowContract() {
        tools::MockToolInventory inv;
        auto* model = qobject_cast<uimodels::VariantListModel*>(inv.tools());
        QVERIFY(model != nullptr);
        QVERIFY(inv.count() > 0);
        const QList<QVariantMap> rows = model->rows();

        bool sawEnabled = false;
        bool sawCredentialRequirement = false;
        for (const QVariantMap& r : rows) {
            QVERIFY(r.contains(QStringLiteral("name")));
            QVERIFY(r.contains(QStringLiteral("enabled")));
            QVERIFY(r.contains(QStringLiteral("requirementIsCredential")));
            if (r.value(QStringLiteral("enabled")).toBool()) {
                sawEnabled = true;
            }
            if (r.value(QStringLiteral("requirementIsCredential")).toBool()) {
                sawCredentialRequirement = true;
                // A credential-gated tool must be disabled and name its requirement.
                QVERIFY(!r.value(QStringLiteral("enabled")).toBool());
                QVERIFY(!r.value(QStringLiteral("requirementLabel")).toString().isEmpty());
            }
        }
        QVERIFY(sawEnabled);
        QVERIFY(sawCredentialRequirement);
    }

    // [waveB:app-v30] D4: the enable/disable intent encodes (and enable vs disable differ on the
    // wire — the bool rides the request).
    void toolSetEnabledEncodes() {
        const QByteArray on = NodeApiCodec::encodeToolSetEnabledRequest(QStringLiteral("fs"), true);
        const QByteArray off =
            NodeApiCodec::encodeToolSetEnabledRequest(QStringLiteral("fs"), false);
        QVERIFY(!on.isEmpty());
        QVERIFY(!off.isEmpty());
        QVERIFY(on != off);
    }

    // [waveB:app-v30] D4: the mock facade's setEnabled flips an ungated tool; a build/credential-
    // gated tool stays disabled (the node is authoritative — a gated tool cannot be force-enabled).
    void mockSetEnabledRespectsGating() {
        tools::MockToolInventory inv;
        auto* model = qobject_cast<uimodels::VariantListModel*>(inv.tools());
        QVERIFY(model != nullptr);

        const auto enabledOf = [&](const QString& name) -> bool {
            for (const QVariantMap& r : model->rows()) {
                if (r.value(QStringLiteral("name")).toString() == name) {
                    return r.value(QStringLiteral("enabled")).toBool();
                }
            }
            return false;
        };

        // An ungated tool ("fs") toggles freely.
        QVERIFY(enabledOf(QStringLiteral("fs")));
        inv.setEnabled(QStringLiteral("fs"), false);
        QVERIFY(!enabledOf(QStringLiteral("fs")));
        inv.setEnabled(QStringLiteral("fs"), true);
        QVERIFY(enabledOf(QStringLiteral("fs")));

        // A gated tool ("web_search", requires tavily) cannot be force-enabled.
        QVERIFY(!enabledOf(QStringLiteral("web_search")));
        inv.setEnabled(QStringLiteral("web_search"), true);
        QVERIFY(!enabledOf(QStringLiteral("web_search")));
    }
};

QTEST_GUILESS_MAIN(TestToolInventory)
#include "tst_tool_inventory.moc"
