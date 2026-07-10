// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "dialogs/auth_flow_dialog.h"

#include "auth/auth_flow_controller.h"
#include "palette_dialog.h"
#include "qr/qr_matrix.h"
#include "tui_dialogs.h"

#include <QRect>
#include <QVariantMap>
#include <QVector>
#include <Tui/ZColor.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZPainter.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWindow.h>

// The half-block QR widget: encode the payload once per challenge and paint the cached rows with a
// fixed scanner-safe palette (black modules on a white quiet zone) — identical in every theme,
// since a QR scanner needs the dark modules unambiguously darker than the light ones.
QrCodeWidget::QrCodeWidget(Tui::ZWidget* parent) : Tui::ZWidget(parent) {
    setFocusPolicy(Tui::NoFocus);
    setSizePolicyH(Tui::SizePolicy::Fixed);
    setSizePolicyV(Tui::SizePolicy::Fixed);
}

void QrCodeWidget::setPayload(const QString& payload) {
    const qr::QrMatrix matrix = qr::QrMatrix::encode(payload);
    m_rows = qr::renderHalfBlocks(matrix, /*quietZone=*/4);
    const int w = m_rows.isEmpty() ? 0 : static_cast<int>(m_rows.first().size());
    const int h = static_cast<int>(m_rows.size());
    setMinimumSize(w, h);
    setMaximumSize(w, h);
    update();
}

QSize QrCodeWidget::sizeHint() const {
    const int w = m_rows.isEmpty() ? 0 : static_cast<int>(m_rows.first().size());
    return {w, static_cast<int>(m_rows.size())};
}

void QrCodeWidget::paintEvent(Tui::ZPaintEvent* event) {
    Tui::ZPainter* painter = event->painter();
    const Tui::ZColor dark = Tui::ZColor::fromRgb(0, 0, 0);
    const Tui::ZColor light = Tui::ZColor::fromRgb(255, 255, 255);
    for (int i = 0; i < m_rows.size(); ++i) {
        painter->writeWithColors(0, i, m_rows.at(i), dark, light);
    }
}

AuthFlowDialog::AuthFlowDialog(auth::AuthFlowController* flow, Tui::ZWidget* parent)
    : Tui::ZDialog(parent), m_flow(flow), m_host(parent) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(tr("Sign in"));
    setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    m_status = new Tui::ZLabel(QString(), this);
    layout->addWidget(m_status);
    layout->addSpacing(1);

    m_valueLabel = new Tui::ZLabel(tr("Open this link in your browser (copy it):"), this);
    layout->addWidget(m_valueLabel);
    // A selectable input (not a label) so the terminal user can copy the URL / QR payload.
    m_value = new Tui::ZInputBox(QString(), this);
    layout->addWidget(m_value);

    // The half-block QR render (shown only for a Qr challenge, above the copyable payload).
    m_qr = new QrCodeWidget(this);
    m_qr->setVisible(false);
    {
        auto* qrRow = new Tui::ZHBoxLayout();
        layout->add(qrRow);
        qrRow->addStretch();
        qrRow->addWidget(m_qr);
        qrRow->addStretch();
    }
    layout->addSpacing(1);

    m_pasteLabel = new Tui::ZLabel(tr("Then paste the redirect URL here:"), this);
    layout->addWidget(m_pasteLabel);
    m_callback = new Tui::ZInputBox(QString(), this);
    layout->addWidget(m_callback);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    m_complete = new Tui::ZButton(tr("Complete sign-in"), this);
    m_complete->setDefault(true);
    buttons->addWidget(m_complete);
    m_cancel = new Tui::ZButton(tr("Cancel"), this);
    buttons->addWidget(m_cancel);

    connect(m_complete, &Tui::ZButton::clicked, this, [this] {
        if (m_flow != nullptr) {
            m_flow->submitCallback(m_callback->text());
        }
    });
    connect(m_cancel, &Tui::ZButton::clicked, this, [this] {
        if (m_flow != nullptr && m_flow->active()) {
            m_flow->cancel();
        }
        close();
    });

    if (m_flow != nullptr) {
        connect(m_flow, &auth::AuthFlowController::phaseChanged, this,
                &AuthFlowDialog::syncToPhase);
        connect(m_flow, &auth::AuthFlowController::challengeChanged, this,
                &AuthFlowDialog::syncToPhase);
        connect(m_flow, &auth::AuthFlowController::errorChanged, this,
                &AuthFlowDialog::syncToPhase);
    }
    syncToPhase();
    setGeometry(QRect(0, 0, 76, 14));
}

void AuthFlowDialog::syncToPhase() {
    if (m_flow == nullptr) {
        return;
    }
    const QString phase = m_flow->phase();
    const QString kind = m_flow->challengeKind();
    const bool onChallenge = phase == QStringLiteral("challenge");
    const bool redirect = onChallenge && kind == QStringLiteral("redirect");
    const bool form = onChallenge && kind == QStringLiteral("form");
    const bool qr = onChallenge && kind == QStringLiteral("qr");
    const bool message = onChallenge && kind == QStringLiteral("message");

    if (phase == QStringLiteral("beginning")) {
        m_status->setText(tr("Preparing sign-in…"));
    } else if (phase == QStringLiteral("stepping")) {
        m_status->setText(tr("Finishing sign-in…"));
    } else if (phase == QStringLiteral("success")) {
        m_status->setText(tr("Signed in as %1").arg(m_flow->accountLabel()));
        // Terminal state: the panel's job is done; leave a beat for the label then close.
        close();
        return;
    } else if (phase == QStringLiteral("failed")) {
        m_status->setText(tr("Sign-in failed: %1").arg(m_flow->error()));
    } else if (phase == QStringLiteral("cancelled")) {
        m_status->setText(tr("Sign-in cancelled"));
    } else if (redirect) {
        m_status->setText(m_flow->sinkListening()
                              ? tr("Waiting for your browser to finish… (or paste below)")
                              : tr("Complete the sign-in in your browser, then paste the "
                                   "redirect URL below."));
    } else if (form) {
        m_status->setText(m_flow->formTitle().isEmpty() ? tr("Enter the requested details.")
                                                        : m_flow->formTitle());
    } else if (qr) {
        m_status->setText(tr("Scan the payload below with your other device. Waiting for "
                             "approval…"));
    } else if (message) {
        m_status->setText(m_flow->messageText());
    } else {
        m_status->setText(QString());
    }

    // The value box carries the Redirect URL or the (copyable) Qr payload; nothing for
    // Form/Message.
    const QString value =
        redirect ? m_flow->authorizationUrl() : (qr ? m_flow->qrPayload() : QString());
    m_value->setText(value);
    m_value->setVisible(!value.isEmpty());

    // A Qr challenge renders the payload as half-blocks (the SAME matrix the GUI Kit.QrCode paints)
    // above the copyable payload text. If the payload cannot encode, the widget stays hidden and
    // the text stands in (stated degradation).
    if (qr) {
        m_qr->setPayload(m_flow->qrPayload());
    }
    m_qr->setVisible(qr && m_qr->hasCode());

    if (redirect) {
        m_valueLabel->setText(tr("Open this link in your browser (copy it):"));
    } else if (qr) {
        m_valueLabel->setText(tr("Scan the code with your other device, or copy the payload:"));
    }
    m_valueLabel->setVisible(!value.isEmpty());
    m_pasteLabel->setVisible(redirect);
    m_callback->setVisible(redirect);
    m_complete->setVisible(redirect);
    m_complete->setEnabled(redirect);

    // Grow the panel to fit the QR (a device-link code can be ~30+ terminal rows); otherwise keep
    // the compact sign-in size.
    if (qr && m_qr->hasCode()) {
        const QSize hint = m_qr->sizeHint();
        setGeometry(QRect(0, 0, qMax(76, hint.width() + 6), hint.height() + 12));
    } else {
        setGeometry(QRect(0, 0, 76, 14));
    }

    // A Form challenge is collected in-flow via prompts; kick the chain once per challenge.
    if (form && !m_formActive) {
        m_formActive = true;
        promptFormFields(m_flow->formFields(), 0, {});
    }
    if (!form) {
        m_formActive = false;
    }
}

void AuthFlowDialog::promptFormFields(const QVariantList& fields, int index,
                                      const QVariantMap& collected) {
    if (m_flow == nullptr) {
        return;
    }
    if (index >= fields.size()) {
        m_flow->submitFields(collected);
        return;
    }
    const QVariantMap field = fields.at(index).toMap();
    const QString key = field.value(QStringLiteral("key")).toString();
    QString label = field.value(QStringLiteral("label")).toString();
    if (label.isEmpty()) {
        label = key;
    }
    // Enriched field shape: mask secrets (kind == "Password") and prefill the node-supplied
    // default, matching the GUI's generic form.
    const bool masked =
        field.value(QStringLiteral("kind")).toString() == QStringLiteral("Password");
    const QString prefill = field.value(QStringLiteral("default")).toString();
    auto* prompt = new TextPromptDialog(label, prefill, masked, m_host);
    connect(prompt, &TextPromptDialog::submitted, this,
            [this, fields, index, collected, key](const QString& text) {
                QVariantMap next = collected;
                if (!text.trimmed().isEmpty()) {
                    next[key] = text.trimmed();
                }
                promptFormFields(fields, index + 1, next);
            });
    connect(prompt, &TextPromptDialog::canceled, this, [this] {
        if (m_flow != nullptr && m_flow->active()) {
            m_flow->cancel();
        }
        close();
    });
}

AuthFlowLauncher::AuthFlowLauncher(auth::AuthFlowController* flow, Tui::ZWidget* host)
    : QObject(host), m_flow(flow), m_host(host) {}

// [waveB:app-v30] CON-15: jump straight to `family`'s param prompts (a provider-row sign-in). If
// the family is not offered by the node, fall back to the generic family pick.
void AuthFlowLauncher::openForFamily(const QString& family) {
    if (m_flow == nullptr || family.isEmpty()) {
        open();
        return;
    }
    m_flow->refreshProviders();
    for (const QVariant& v : m_flow->providers()) {
        const QVariantMap p = v.toMap();
        if (p.value(QStringLiteral("family")).toString() == family) {
            promptParams(p);
            return;
        }
    }
    open();
}

void AuthFlowLauncher::open() {
    if (m_flow == nullptr) {
        emit finished();
        return;
    }
    m_flow->refreshProviders();
    const QVariantList providers = m_flow->providers();
    if (providers.isEmpty()) {
        // No interactive-auth family offered (node without auth factories): nothing to launch.
        emit finished();
        return;
    }
    if (providers.size() == 1) {
        promptParams(providers.first().toMap());
        return;
    }
    if (m_familyPick == nullptr) {
        m_familyPick = new PaletteDialog(tr("Sign in — pick a provider"), m_host);
        connect(m_familyPick, &PaletteDialog::canceled, this, &AuthFlowLauncher::finished);
    }
    disconnect(m_familyPick, &PaletteDialog::activated, this, nullptr);
    connect(m_familyPick, &PaletteDialog::activated, this, [this](const QString& family) {
        for (const QVariant& v : m_flow->providers()) {
            const QVariantMap p = v.toMap();
            if (p.value(QStringLiteral("family")).toString() == family) {
                promptParams(p);
                return;
            }
        }
        emit finished();
    });
    QVector<PaletteDialog::Item> items;
    items.reserve(providers.size());
    for (const QVariant& v : providers) {
        const QVariantMap p = v.toMap();
        items.push_back({p.value(QStringLiteral("family")).toString(),
                         p.value(QStringLiteral("name")).toString(),
                         p.value(QStringLiteral("flowKind")).toString()});
    }
    m_familyPick->setItems(items);
    m_familyPick->openCentered();
}

void AuthFlowLauncher::promptParams(const QVariantMap& provider) {
    const QString family = provider.value(QStringLiteral("family")).toString();
    if (family.isEmpty()) {
        emit finished();
        return;
    }
    promptNextParam(family, provider.value(QStringLiteral("params")).toList(), 0, {});
}

void AuthFlowLauncher::promptNextParam(const QString& family, const QVariantList& fields, int index,
                                       const QVariantMap& collected) {
    if (index >= fields.size()) {
        launch(family, collected);
        return;
    }
    const QVariantMap field = fields.at(index).toMap();
    const QString key = field.value(QStringLiteral("key")).toString();
    QString label = field.value(QStringLiteral("label")).toString();
    if (label.isEmpty()) {
        label = key;
    }
    // Enriched begin-schema field: mask secrets and prefill the node-supplied default (parity with
    // the GUI picker's schema-driven param fields).
    const bool masked =
        field.value(QStringLiteral("kind")).toString() == QStringLiteral("Password");
    const QString prefill = field.value(QStringLiteral("default")).toString();
    auto* prompt = new TextPromptDialog(label, prefill, masked, m_host);
    connect(prompt, &TextPromptDialog::submitted, this,
            [this, family, fields, index, collected, key](const QString& text) {
                QVariantMap next = collected;
                if (!text.trimmed().isEmpty()) {
                    next[key] = text.trimmed();
                }
                promptNextParam(family, fields, index + 1, next);
            });
    connect(prompt, &TextPromptDialog::canceled, this, &AuthFlowLauncher::finished);
}

void AuthFlowLauncher::launch(const QString& family, const QVariantMap& params) {
    auto* panel = new AuthFlowDialog(m_flow, m_host);
    connect(panel, &Tui::ZDialog::rejected, this, &AuthFlowLauncher::finished);
    // Sink allowed: a TUI user may still have a local browser; the paste box always works.
    m_flow->start(family, params);
    panel->setFocus();
}
