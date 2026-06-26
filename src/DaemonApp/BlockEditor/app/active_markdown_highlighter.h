#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

namespace be::app {

class ActiveMarkdownHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit ActiveMarkdownHighlighter(QObject* parent = nullptr);

    void setContext(int blockType, int headingLevel);

protected:
    void highlightBlock(const QString& text) override;

private:
    void applyInlineFormats(const QString& text);
    void applyRegexFormat(const QString& text, const QRegularExpression& regex,
                          const QTextCharFormat& contentFormat,
                          const QTextCharFormat& delimiterFormat);
    void applyLinkFormats(const QString& text);

    int m_blockType = 0;
    int m_headingLevel = 0;
    QTextCharFormat m_markerFormat;
    QTextCharFormat m_headingFormat;
    QTextCharFormat m_strongFormat;
    QTextCharFormat m_emphasisFormat;
    QTextCharFormat m_codeFormat;
    QTextCharFormat m_linkFormat;
};

} // namespace be::app
