#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace be {
class DocumentStore;
class TranscriptIngest;
}

// TUI-local stand-in for the daemon runtime, replicating the QML mock host in
// Transcript.qml so the inline interactive blocks (clarify / approval) round-trip
// end-to-end. It operates directly on the shared be::DocumentStore + ingest:
//
//   - approval answer  -> updateBlockMetadata(blockId, be::toolApprovalPatch(...));
//                         an approval also drives the tool to a finished ok state.
//   - clarify submit   -> updateBlockMetadata(blockId, be::clarifyAnswerPatch(...))
//                         then ingests a short "proceeding with ..." follow-up.
//
// After mutating the document it emits documentChanged so the host (RootWidget)
// can persist the patched markdown and repaint. A real host would forward the
// answers to the gateway and stream the follow-up turn instead.
class InteractiveTurnHost : public QObject {
    Q_OBJECT

public:
    InteractiveTurnHost(be::DocumentStore *doc, be::TranscriptIngest *ingest,
                        QObject *parent = nullptr);

public slots:
    void onApprovalDecided(const QString &callId, const QString &decision, bool permanent);
    void onClarifySubmitted(const QString &callId, const QString &requestId,
                            const QVariantMap &answers);

signals:
    // The host mutated the document; the owner should persist toMarkdown() and
    // reload the transcript.
    void documentChanged();

private:
    be::DocumentStore *m_doc = nullptr;
    be::TranscriptIngest *m_ingest = nullptr;
};
