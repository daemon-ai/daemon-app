// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "status_bar_view.h"

#include "presentation/display_presenter.h"
#include "status_bar_model.h"
#include "tui_palette.h"

#include <QRect>
#include <QSize>
#include <Tui/ZColor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

namespace {

Span mkSpan(const QString& text, const Tui::ZColor& fg) {
    return Span{text, fg, tpal::surfaceAlt(), {}};
}

int spansWidth(const QVector<Span>& spans) {
    int n = 0;
    for (const Span& s : spans) {
        n += static_cast<int>(s.text.size());
    }
    return n;
}

QString separator() {
    return QStringLiteral("  \u00b7  ");
}

} // namespace

StatusBarView::StatusBarView(Tui::ZWidget* parent) : Tui::ZWidget(parent) {
    setMaximumSize(Tui::tuiMaxSize, 1);
    setSizePolicyV(Tui::SizePolicy::Fixed);
    setSizePolicyH(Tui::SizePolicy::Expanding);
    m_spinner.setInterval(120);
    connect(&m_spinner, &QTimer::timeout, this, [this] {
        ++m_spinnerTick;
        update();
    });
}

void StatusBarView::setModel(StatusBarModel* model) {
    m_model = model;
    if (m_model != nullptr) {
        const auto repaint = [this] { update(); };
        connect(m_model, &StatusBarModel::gatewayStateChanged, this, repaint);
        connect(m_model, &StatusBarModel::agentsDetailChanged, this, repaint);
        connect(m_model, &StatusBarModel::contextChanged, this, repaint);
        connect(m_model, &StatusBarModel::usageChanged, this, repaint);
        connect(m_model, &StatusBarModel::rateChanged, this, repaint);
        connect(m_model, &StatusBarModel::turnElapsedChanged, this, repaint);
        connect(m_model, &StatusBarModel::sessionElapsedChanged, this, repaint);
        connect(m_model, &StatusBarModel::appVersionChanged, this, repaint);
        connect(m_model, &StatusBarModel::updateVersionChanged, this, repaint);
        connect(m_model, &StatusBarModel::busyChanged, this, [this] {
            syncSpinner();
            update();
        });
    }
    syncSpinner();
    update();
}

void StatusBarView::setPendingApprovals(int count) {
    if (m_pendingApprovals == count) {
        return;
    }
    m_pendingApprovals = count;
    update();
}

QSize StatusBarView::sizeHint() const {
    return {40, 1};
}

void StatusBarView::syncSpinner() {
    if (m_model != nullptr && m_model->busy()) {
        if (!m_spinner.isActive()) {
            m_spinner.start();
        }
    } else {
        m_spinner.stop();
    }
}

QVector<Span> StatusBarView::buildLeft() const {
    QVector<Span> spans;
    if (m_model == nullptr) {
        return spans;
    }
    const QString tone = m_model->gatewayTone();
    const bool alert = m_model->gatewayOffline() || m_model->gatewayDegraded();
    const Tui::ZColor gw = tpal::gatewayToneColor(tone);
    spans << mkSpan(tpal::gatewayGlyph(alert) + tr(" Gateway "), gw)
          << mkSpan(DisplayPresenter::enumLabelFor(QStringLiteral("connection"),
                                                   m_model->gatewayState()),
                    gw);

    spans << mkSpan(separator(), tpal::muted());

    const QString detail = m_model->agentsDetail();
    const bool failed = m_model->agentsFailed() > 0;
    const Tui::ZColor agentsFg = failed ? tpal::statusError() : tpal::faint();
    spans << mkSpan(tpal::agentsGlyph() + tr(" Agents "), agentsFg)
          << mkSpan(detail.isEmpty() ? tr("idle") : detail, agentsFg);

    // TOOL-8: pending-approvals gate count (parity with the GUI sidebar badge). Only shown when
    // there is at least one gate waiting; the warn tone matches the approval-gate accent.
    if (m_pendingApprovals > 0) {
        spans << mkSpan(separator(), tpal::muted());
        spans << mkSpan(QStringLiteral("\u26a0 ") + tr("Approvals ") +
                            QString::number(m_pendingApprovals),
                        tpal::warn());
    }
    return spans;
}

QVector<Span> StatusBarView::buildRight() const {
    QVector<Span> spans;
    if (m_model == nullptr) {
        return spans;
    }

    if (m_model->busy()) {
        spans << mkSpan(tpal::spinnerFrame(m_spinnerTick) + QStringLiteral(" ") +
                            m_model->turnElapsed(),
                        tpal::accent())
              << mkSpan(separator(), tpal::muted());
    }

    // Context: label + colored ASCII gauge (fill vs track).
    spans << mkSpan(tpal::contextGlyph() + QStringLiteral(" ") + m_model->contextLabel() +
                        QStringLiteral(" "),
                    tpal::muted());
    const QString bar = m_model->contextBar();
    for (const QChar ch : bar) {
        Tui::ZColor fg = tpal::muted();
        if (ch == QChar(0x2588)) { // █ filled
            fg = tpal::gaugeFill();
        } else if (ch == QChar(0x2591)) { // ░ track
            fg = tpal::gaugeTrack();
        }
        spans << mkSpan(QString(ch), fg);
    }

    // Session usage + spend (fed by the turn's usage events).
    if (m_model->usdCost() > 0.0 || m_model->tokensOut() > 0) {
        spans << mkSpan(separator(), tpal::muted());
        QString usage = m_model->costLabel();
        const int tok = m_model->tokensIn() + m_model->tokensOut();
        if (tok > 0) {
            usage += QStringLiteral(" ") + m_model->abbrev(tok) + tr(" tok");
        }
        spans << mkSpan(usage, tpal::muted());
    }

    // Provider rate-limit window (remaining/limit), shown only once fed.
    const QString rate = m_model->rateLabel();
    if (!rate.isEmpty()) {
        spans << mkSpan(separator(), tpal::muted());
        spans << mkSpan(QStringLiteral("\u29d7 ") + rate, tpal::muted());
    }

    spans << mkSpan(separator(), tpal::muted());
    spans << mkSpan(tpal::sessionGlyph() + QStringLiteral(" ") + m_model->sessionElapsed(),
                    tpal::muted());

    // An available update surfaces as an accented segment (the TUI analog of the
    // GUI banner); the palette command "Check for updates" acts on it.
    if (!m_model->updateVersion().isEmpty()) {
        spans << mkSpan(separator(), tpal::muted());
        spans << mkSpan(QStringLiteral("\u2913 ") + tr("update v") + m_model->updateVersion(),
                        tpal::accent());
    }

    spans << mkSpan(separator(), tpal::muted());
    spans << mkSpan(tpal::versionGlyph() + m_model->appVersion(), tpal::muted());
    return spans;
}

void StatusBarView::paintEvent(Tui::ZPaintEvent* event) {
    Tui::ZPainter* p = event->painter();
    p->clear(tpal::fg(), tpal::surfaceAlt());

    const int w = geometry().width();
    const QVector<Span> left = buildLeft();
    const QVector<Span> right = buildRight();

    int x = 0;
    for (const Span& s : left) {
        if (x >= w) {
            break;
        }
        QString text = s.text;
        if (x + static_cast<int>(text.size()) > w) {
            text = text.left(w - x);
        }
        p->writeWithColors(x, 0, text, s.fg, s.bg);
        x += static_cast<int>(s.text.size());
    }

    const int rightW = spansWidth(right);
    int rx = qMax(x + 1, w - rightW);
    for (const Span& s : right) {
        if (rx >= w) {
            break;
        }
        QString text = s.text;
        if (rx + static_cast<int>(text.size()) > w) {
            text = text.left(w - rx);
        }
        p->writeWithColors(rx, 0, text, s.fg, s.bg);
        rx += static_cast<int>(s.text.size());
    }
}
