#include "file_finder_model.h"

#include "fs/ifs_service.h"

#include <QSet>

#include <algorithm>

namespace files {
namespace {
constexpr int kMaxFiles = 20000;     // index cap (Lite XL-style budget)
constexpr int kMaxConcurrent = 8;    // outstanding open() requests
constexpr int kMaxResults = 200;     // ranked rows shown
const QChar kSep(0x1f);

QString recentKey(const QString& rootId, const QString& path)
{
    return rootId + kSep + path;
}
} // namespace

FileFinderModel::FileFinderModel(QObject* parent)
    : QAbstractListModel(parent)
{
    fs::registerFsMetatypes();
}

FileFinderModel::~FileFinderModel() = default;

QObject* FileFinderModel::service() const
{
    return m_service;
}

void FileFinderModel::setService(QObject* service)
{
    auto* svc = qobject_cast<fs::IFsService*>(service);
    if (svc == m_service)
        return;
    if (m_service)
        m_service->disconnect(this);
    m_service = svc;
    emit serviceChanged();
    if (m_service) {
        connect(m_service, &fs::IFsService::listed, this, &FileFinderModel::onListed);
        connect(m_service, &fs::IFsService::rootsChanged, this, &FileFinderModel::onRootsChanged);
        m_service->listRoots();
    }
}

void FileFinderModel::setQuery(const QString& query)
{
    if (m_query == query)
        return;
    m_query = query;
    emit queryChanged();
    rerank();
}

int FileFinderModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_results.size());
}

QVariant FileFinderModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_results.size())
        return {};
    const File& f = m_index.at(m_results.at(index.row()));
    switch (role) {
    case Qt::DisplayRole:
    case PathRole:
        return f.path;
    case NameRole:
        return f.name;
    case RootIdRole:
        return f.rootId;
    default:
        return {};
    }
}

QHash<int, QByteArray> FileFinderModel::roleNames() const
{
    return {
        { PathRole, "path" },
        { NameRole, "name" },
        { RootIdRole, "rootId" },
    };
}

void FileFinderModel::onRootsChanged(const QList<fs::FsRoot>& roots)
{
    // Start a fresh index walk from each root's top directory. (Seeding here,
    // not in rebuildIndex(), avoids a listRoots/rebuild recursion.)
    beginResetModel();
    m_index.clear();
    m_results.clear();
    endResetModel();
    m_dirQueue.clear();
    m_pendingDirs.clear();
    m_seenKeys.clear();
    m_inFlight = 0;
    m_filesSeen = 0;
    if (!m_indexing) {
        m_indexing = true;
        emit indexingChanged();
    }
    for (const fs::FsRoot& r : roots)
        enqueueDir(r.id, QString());
    if (roots.isEmpty()) {
        m_indexing = false;
        emit indexingChanged();
    }
}

void FileFinderModel::rebuildIndex()
{
    // Re-request roots; the walk (re)starts in onRootsChanged.
    if (m_service)
        m_service->listRoots();
}

void FileFinderModel::enqueueDir(const QString& rootId, const QString& dir)
{
    if (m_filesSeen >= kMaxFiles)
        return;
    m_dirQueue.enqueue(qMakePair(rootId, dir));
    pump();
}

void FileFinderModel::pump()
{
    if (!m_service)
        return;
    while (m_inFlight < kMaxConcurrent && !m_dirQueue.isEmpty() && m_filesSeen < kMaxFiles) {
        const QPair<QString, QString> d = m_dirQueue.dequeue();
        m_pendingDirs.insert(recentKey(d.first, d.second));
        ++m_inFlight;
        m_service->open(d.first, d.second);
    }
    const bool done = m_dirQueue.isEmpty() && m_inFlight == 0;
    if (done && m_indexing) {
        m_indexing = false;
        emit indexingChanged();
        rerank();
    }
}

void FileFinderModel::onListed(const QString& rootId, const QString& dir,
                               const QList<fs::FsEntry>& entries)
{
    // A `listed` may be ours (from the index walk) or from another consumer
    // (e.g. the tree browsing). Either way, fold its files into the index; only
    // our own requests drive the walk queue.
    const QString requestKey = recentKey(rootId, dir);
    const bool ownedResponse = m_pendingDirs.remove(requestKey);
    if (ownedResponse && m_inFlight > 0)
        --m_inFlight;

    for (const fs::FsEntry& e : entries) {
        if (e.ignored)
            continue;
        if (e.isDir && ownedResponse) {
            enqueueDir(rootId, e.path);
        } else if (m_filesSeen < kMaxFiles) {
            const QString k = recentKey(rootId, e.path);
            if (!m_seenKeys.contains(k)) {
                m_seenKeys.insert(k);
                m_index.push_back(File{ rootId, e.path, e.name });
                ++m_filesSeen;
            }
        }
    }
    pump();
    Q_UNUSED(dir);
}

bool FileFinderModel::fuzzyScore(const QString& text, const QString& pattern, int* score)
{
    if (pattern.isEmpty()) {
        if (score)
            *score = 0;
        return true;
    }
    int si = static_cast<int>(text.size()) - 1;
    int pi = static_cast<int>(pattern.size()) - 1;
    int s = 0;
    int run = 0;
    while (si >= 0 && pi >= 0) {
        const QChar sc = text.at(si);
        const QChar pc = pattern.at(pi);
        if (sc.toLower() == pc.toLower()) {
            s += 1 + run * 8;
            if (sc == pc)
                s += 1;
            ++run;
            --pi;
            --si;
        } else {
            run = 0;
            s -= 1;
            --si;
        }
    }
    if (pi >= 0)
        return false; // pattern not fully consumed -> no match
    s -= (si + 1);    // prefer matches nearer the start of the path
    if (score)
        *score = s;
    return true;
}

void FileFinderModel::noteRecent(const QString& rootId, const QString& path)
{
    const QString k = recentKey(rootId, path);
    m_recents.removeAll(k);
    m_recents.prepend(k);
    while (m_recents.size() > 50)
        m_recents.removeLast();
    rerank();
}

void FileFinderModel::rerank()
{
    beginResetModel();
    m_results.clear();
    if (m_query.trimmed().isEmpty()) {
        // Recents first, then index order, capped.
        QSet<int> used;
        for (const QString& rk : std::as_const(m_recents)) {
            for (int i = 0; i < m_index.size(); ++i) {
                if (recentKey(m_index[i].rootId, m_index[i].path) == rk) {
                    m_results.push_back(i);
                    used.insert(i);
                    break;
                }
            }
            if (m_results.size() >= kMaxResults)
                break;
        }
        for (int i = 0; i < m_index.size() && m_results.size() < kMaxResults; ++i) {
            if (!used.contains(i))
                m_results.push_back(i);
        }
    } else {
        QList<QPair<int, int>> scored; // (score, index)
        for (int i = 0; i < m_index.size(); ++i) {
            int sc = 0;
            if (fuzzyScore(m_index[i].path, m_query, &sc))
                scored.push_back(qMakePair(sc, i));
        }
        std::stable_sort(scored.begin(), scored.end(),
                         [](const QPair<int, int>& a, const QPair<int, int>& b) {
                             return a.first > b.first;
                         });
        for (int i = 0; i < scored.size() && i < kMaxResults; ++i)
            m_results.push_back(scored[i].second);
    }
    endResetModel();
}

// ---------------------------------------------------------------------------
// SearchResultsModel
// ---------------------------------------------------------------------------

SearchResultsModel::SearchResultsModel(QObject* parent)
    : QAbstractListModel(parent)
{
    fs::registerFsMetatypes();
}

SearchResultsModel::~SearchResultsModel() = default;

QObject* SearchResultsModel::service() const
{
    return m_service;
}

void SearchResultsModel::setService(QObject* service)
{
    auto* svc = qobject_cast<fs::IFsService*>(service);
    if (svc == m_service)
        return;
    if (m_service)
        m_service->disconnect(this);
    m_service = svc;
    emit serviceChanged();
    if (m_service)
        connect(m_service, &fs::IFsService::searchResults, this,
                &SearchResultsModel::onSearchResults);
}

int SearchResultsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_hits.size());
}

QVariant SearchResultsModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_hits.size())
        return {};
    const fs::FsSearchHit& h = m_hits.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case PreviewRole:
        return h.preview;
    case PathRole:
        return h.path;
    case LineRole:
        return h.line;
    case ColumnRole:
        return h.column;
    case RootIdRole:
        return m_rootId;
    default:
        return {};
    }
}

QHash<int, QByteArray> SearchResultsModel::roleNames() const
{
    return {
        { PathRole, "path" },     { LineRole, "line" },        { ColumnRole, "column" },
        { PreviewRole, "preview" }, { RootIdRole, "rootId" },
    };
}

void SearchResultsModel::search(const QString& rootId, const QString& query, const QVariantMap& opts)
{
    if (!m_service)
        return;
    m_rootId = rootId;
    m_query = query;
    emit queryChanged();
    beginResetModel();
    m_hits.clear();
    endResetModel();
    m_searching = true;
    emit searchingChanged();
    m_service->search(rootId, query, opts);
}

void SearchResultsModel::onSearchResults(const QString& rootId, const QString& query,
                                         const QList<fs::FsSearchHit>& hits, bool complete)
{
    if (rootId != m_rootId || query != m_query)
        return;
    beginResetModel();
    m_hits = hits;
    endResetModel();
    if (complete && m_searching) {
        m_searching = false;
        emit searchingChanged();
    }
}

} // namespace files
