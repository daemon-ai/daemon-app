#include "diagram/text/text_measurer.h"

#include <QFontMetricsF>
#include <QTextOption>

namespace be::diagram {

namespace {

// Run the line-breaking loop for a QTextLayout and return the laid-out size.
// When maxWidth is set, lines wrap at that width; otherwise each natural line is
// kept on one line.
QSizeF layoutLines(QTextLayout& layout, std::optional<qreal> maxWidth) {
    layout.beginLayout();
    qreal height = 0.0;
    qreal widest = 0.0;
    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid()) {
            break;
        }
        if (maxWidth) {
            line.setLineWidth(*maxWidth);
        } else {
            line.setLineWidth(1.0e7);
        }
        line.setPosition(QPointF(0.0, height));
        height += line.height();
        widest = qMax(widest, line.naturalTextWidth());
    }
    layout.endLayout();
    return QSizeF(widest, height);
}

} // namespace

TextMetrics TextMeasurer::measure(const QString& text, const QFont& font,
                                  std::optional<qreal> maxWidth) const {
    if (text.isEmpty()) {
        const QFontMetricsF fm(font);
        return TextMetrics{0.0, fm.height(), 0};
    }

    QTextLayout layout(text, font);
    QTextOption option = layout.textOption();
    option.setWrapMode(maxWidth ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    layout.setTextOption(option);

    const QSizeF size = layoutLines(layout, maxWidth);
    return TextMetrics{size.width(), size.height(), layout.lineCount()};
}

std::shared_ptr<QTextLayout> TextMeasurer::layout(const QString& text, const QFont& font,
                                                  std::optional<qreal> maxWidth) const {
    auto layout = std::make_shared<QTextLayout>(text, font);
    QTextOption option = layout->textOption();
    option.setWrapMode(maxWidth ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    option.setAlignment(Qt::AlignHCenter);
    layout->setTextOption(option);
    layoutLines(*layout, maxWidth);
    return layout;
}

} // namespace be::diagram
