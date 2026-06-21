#include "composer_chrome.h"

#include "tui_palette.h"

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
    if (m_turn == nullptr) {
        return spans;
    }

    const QString state = m_turn->turnState();
    if (state == QStringLiteral("error")) {
        const QString msg = m_turn->errorText().isEmpty() ? QStringLiteral("turn failed")
                                                           : m_turn->errorText();
        spans << mkSpan(tpal::warnGlyph() + QStringLiteral(" ") + msg, tpal::statusError());
        return spans;
    }

    if (m_turn->active()) {
        const int secs = m_turn->elapsedMs() / 1000;
        const QString spin = tpal::spinnerFrame(m_spinnerTick);
        if (state == QStringLiteral("stalled")) {
            spans << mkSpan(spin + QStringLiteral(" Still thinking\u2026 ")
                                + QString::number(secs) + QStringLiteral("s"),
                            tpal::warn());
        } else {
            spans << mkSpan(spin + QStringLiteral(" Thinking\u2026 ")
                                + QString::number(secs) + QStringLiteral("s"),
                            tpal::accent());
        }
        spans << mkSpan(QStringLiteral("   "), tpal::muted());
        spans << mkSpan(tpal::stopGlyph() + QStringLiteral(" Esc stop"), tpal::muted());
        return spans;
    }

    // Idle: a dim affordance hint.
    spans << mkSpan(tpal::sendGlyph() + QStringLiteral(" Enter send"), tpal::muted());
    spans << mkSpan(QStringLiteral("  \u00b7  "), tpal::faint());
    spans << mkSpan(tpal::steerGlyph() + QStringLiteral(" Ctrl+Enter steer"), tpal::muted());
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
