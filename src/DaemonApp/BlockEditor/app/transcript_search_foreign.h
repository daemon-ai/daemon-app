#pragma once

#include "core/transcript_search.h"

#include <QtQmlIntegration/qqmlintegration.h>

// Anonymous QML registration for be::TranscriptSearchController (defined in
// be_core, which has no QML dependency). EditorController exposes an instance via
// its `search` property; this foreign type lets QML read that object's
// properties (query/matchCount/currentMatch), call its invokables (next/previous/
// clear/setCurrent), and connect its signals (navigateTo). It is not creatable
// from QML - it only ever lives as a child of an EditorController.
struct TranscriptSearchControllerForeign {
    Q_GADGET
    QML_FOREIGN(be::TranscriptSearchController)
    QML_ANONYMOUS
};
