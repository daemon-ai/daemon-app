#include "composer_chrome.h"

#include "tui_palette.h"

#include "composer_session_controller.h"
#include "turn_controller.h"

#include <Tui/ZColor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

#include <QRect>
#include <QSize>

namespace {

Span mkSpan(const QString& text, const Tui::ZColor& fg)
{
    return Span { text, fg, tpal::bg(), {} };
}

} // namespace

ComposerChrome::ComposerChrome(Tui::ZWidget* parent) : Tui::ZWidget(parent)
{
    setMaximumSize(Tui::tuiMaxSize, 1);
    setSizePolicyV(Tui::SizePolicy::Fixed);
    setSizePolicyH(Tui::SizePolicy::Expanding);
    m_spinner.setInterval(120);
    connect(&m_spinner, &QTimer::timeout, this, [this] {
        ++m_spinnerTick;
        update();
    });
}

void ComposerChrome::setTurn(TurnController* turn)
{
    m_turn = turn;
    if (m_turn != nullptr) {
        const auto repaint = [this] { update(); };
        connect(m_turn, &TurnController::turnStateChanged, this, repaint);
        connect(m_turn, &TurnController::elapsedMsChanged, this, repaint);
        connect(m_turn, &TurnController::errorTextChanged, this, repaint);
        connect(m_turn, &TurnController::activeChanged, this, [this] {
            syncSpinner();
            update();
        });
    }
    syncSpinner();
    update();
}

void ComposerChrome::setSession(ComposerSessionController* session)
{
    m_session = session;
    if (m_session != nullptr) {
        connect(m_session, &ComposerSessionController::currentModelChanged, this,
                [this] { update(); });
        connect(m_session, &ComposerSessionController::modesChanged, this,
                [this] { update(); });
        connect(m_session, &ComposerSessionController::reverseSearchChanged, this,
                [this] { update(); });
    }
    update();
}

QSize ComposerChrome::sizeHint() const
{
    return { 40, 1 };
}

void ComposerChrome::syncSpinner()
{
    if (m_turn != nullptr && m_turn->active()) {
        if (!m_spinner.isActive()) {
            m_spinner.start();
        }
    } else {
        m_spinner.stop();
    }
}

QVector<Span> ComposerChrome::buildSpans() const
{
    QVector<Span> spans;

    // Reverse incremental history search owns the chrome line while active, mirroring
    // readline's "(reverse-i-search)`query':" prompt (red when the query has no match).
    if (m_session != nullptr && m_session->reverseSearching()) {
        const bool found = m_session->reverseSearchFound();
        const Tui::ZColor label = found ? tpal::accent() : tpal::warn();
        spans << mkSpan(found ? tr("(reverse-i-search)`")
                              : tr("(failed reverse-i-search)`"),
                        label);
        spans << mkSpan(m_session->reverseSearchQuery(), tpal::fg());
        spans << mkSpan(QStringLiteral("': "), label);
        spans << mkSpan(tr("Enter accept  \u00b7  Ctrl+R next  \u00b7  Esc cancel"),
                        tpal::faint());
        return spans;
    }

    if (m_turn == nullptr) {
        return spans;
    }

    const QString state = m_turn->turnState();
    if (state == QStringLiteral("error")) {
        const QString msg = m_turn->errorText().isEmpty() ? tr("turn failed")
                                                           : m_turn->errorText();
        spans << mkSpan(tpal::warnGlyph() + QStringLiteral(" ") + msg, tpal::statusError());
        return spans;
    }

    if (m_turn->active()) {
        const int secs = m_turn->elapsedMs() / 1000;
        const QString spin = tpal::spinnerFrame(m_spinnerTick);
        if (state == QStringLiteral("stalled")) {
            spans << mkSpan(spin + tr(" Still thinking\u2026 %1s").arg(secs),
                            tpal::warn());
        } else {
            spans << mkSpan(spin + tr(" Thinking\u2026 %1s").arg(secs),
                            tpal::accent());
        }
        spans << mkSpan(QStringLiteral("   "), tpal::muted());
        spans << mkSpan(tpal::stopGlyph() + tr(" Esc stop"), tpal::muted());
        return spans;
    }

    // Idle: a dim affordance hint, plus the shared current-model name (model pill).
    spans << mkSpan(tpal::sendGlyph() + tr(" Enter send"), tpal::muted());
    spans << mkSpan(QStringLiteral("  \u00b7  "), tpal::faint());
    spans << mkSpan(tpal::steerGlyph() + tr(" Ctrl+Enter steer"), tpal::muted());
    if (m_session != nullptr && !m_session->currentModel().isEmpty()) {
        spans << mkSpan(QStringLiteral("  \u00b7  "), tpal::faint());
        spans << mkSpan(m_session->currentModel(), tpal::muted());
        // Compact mode badges: reasoning level (unless off) + Fast/Verbose flags.
        QStringList badges;
        const QString effort = m_session->reasoningEffort();
        if (effort != QStringLiteral("off")) {
            badges << (QStringLiteral("r:") + effort.left(1));
        }
        if (m_session->fastMode()) {
            badges << QStringLiteral("fast");
        }
        if (m_session->verbose()) {
            badges << QStringLiteral("verbose");
        }
        if (!badges.isEmpty()) {
            spans << mkSpan(QStringLiteral("  ["), tpal::faint());
            spans << mkSpan(badges.join(QStringLiteral(" ")), tpal::accent());
            spans << mkSpan(QStringLiteral("]"), tpal::faint());
        }
    }
    return spans;
}

void ComposerChrome::paintEvent(Tui::ZPaintEvent* event)
{
    Tui::ZPainter* p = event->painter();
    p->clear(tpal::fg(), tpal::bg());

    const int w = geometry().width();
    int x = 0;
    for (const Span& s : buildSpans()) {
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
}
