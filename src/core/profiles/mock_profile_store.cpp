#include "profiles/mock_profile_store.h"

#include "appcache/json_cache.h"

namespace profiles {
namespace {

const QString kCacheFile = QStringLiteral("profiles.json");


QVariantMap mkProfile(const QString& id, const QString& name, const QString& model,
                      const QString& description, const QString& systemPrompt,
                      const QStringList& skills, const QStringList& tools, bool isDefault)
{
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("name")] = name;
    m[QStringLiteral("model")] = model;
    m[QStringLiteral("description")] = description;
    m[QStringLiteral("systemPrompt")] = systemPrompt;
    m[QStringLiteral("skills")] = skills;
    m[QStringLiteral("tools")] = tools;
    m[QStringLiteral("isDefault")] = isDefault;
    return m;
}

} // namespace

MockProfileStore::MockProfileStore(QObject* parent)
    : IProfileStore(parent)
    , m_profiles(new uimodels::VariantListModel(this))
{
    m_profiles->upsert(mkProfile(
        QStringLiteral("prof-1"), QStringLiteral("General Assistant"),
        QStringLiteral("llama-3.1-8b-instruct"),
        QStringLiteral("Balanced everyday agent."),
        QStringLiteral("You are a helpful, concise assistant."),
        { QStringLiteral("web-search"), QStringLiteral("summarize") },
        { QStringLiteral("read"), QStringLiteral("search") }, true));
    m_profiles->upsert(mkProfile(
        QStringLiteral("prof-2"), QStringLiteral("Coder"),
        QStringLiteral("qwen2.5-coder-32b"),
        QStringLiteral("Code-focused agent with shell + edit."),
        QStringLiteral("You are an expert software engineer. Prefer minimal diffs."),
        { QStringLiteral("code-review"), QStringLiteral("refactor") },
        { QStringLiteral("read"), QStringLiteral("write"), QStringLiteral("shell"),
          QStringLiteral("search") }, false));
    m_profiles->upsert(mkProfile(
        QStringLiteral("prof-3"), QStringLiteral("Researcher"),
        QStringLiteral("mixtral-8x7b"),
        QStringLiteral("Long-form research + citations."),
        QStringLiteral("You are a meticulous research analyst. Cite sources."),
        { QStringLiteral("web-search"), QStringLiteral("summarize"), QStringLiteral("cite") },
        { QStringLiteral("read"), QStringLiteral("search"), QStringLiteral("browse") }, false));
    m_defaultId = QStringLiteral("prof-1");
    m_nextId = 4;

    // Restore the last-known snapshot over the seed (no-op on first run).
    const QJsonObject cached = appcache::loadObject(kCacheFile);
    if (cached.contains(QStringLiteral("rows"))) {
        m_profiles->setRows(appcache::rowsFromJson(cached.value(QStringLiteral("rows")).toArray()));
        m_defaultId = cached.value(QStringLiteral("defaultId")).toString(m_defaultId);
        m_nextId = cached.value(QStringLiteral("nextId")).toInt(m_nextId);
    }
}

void MockProfileStore::save() const
{
    QJsonObject obj;
    obj[QStringLiteral("rows")] = appcache::rowsToJson(m_profiles->rows());
    obj[QStringLiteral("defaultId")] = m_defaultId;
    obj[QStringLiteral("nextId")] = m_nextId;
    appcache::saveObject(kCacheFile, obj);
}

QObject* MockProfileStore::profiles() const { return m_profiles; }

QVariantMap MockProfileStore::profile(const QString& id) const
{
    const int row = m_profiles->indexOfId(id);
    return row >= 0 ? m_profiles->at(row) : QVariantMap{};
}

QStringList MockProfileStore::profileNames() const
{
    QStringList out;
    for (const QVariantMap& row : m_profiles->rows()) {
        out << row.value(QStringLiteral("name")).toString();
    }
    return out;
}

QVariantList MockProfileStore::availableSkills() const
{
    const auto mk = [](const QString& id, const QString& name, const QString& desc) {
        QVariantMap m;
        m[QStringLiteral("id")] = id;
        m[QStringLiteral("name")] = name;
        m[QStringLiteral("description")] = desc;
        return QVariant(m);
    };
    return {
        mk(QStringLiteral("web-search"), QStringLiteral("Web search"),
           QStringLiteral("Query the web for current information.")),
        mk(QStringLiteral("summarize"), QStringLiteral("Summarize"),
           QStringLiteral("Condense long documents.")),
        mk(QStringLiteral("cite"), QStringLiteral("Citations"),
           QStringLiteral("Attach source citations to claims.")),
        mk(QStringLiteral("code-review"), QStringLiteral("Code review"),
           QStringLiteral("Review diffs for bugs and style.")),
        mk(QStringLiteral("refactor"), QStringLiteral("Refactor"),
           QStringLiteral("Restructure code safely.")),
        mk(QStringLiteral("translate"), QStringLiteral("Translate"),
           QStringLiteral("Translate between languages.")),
    };
}

QVariantList MockProfileStore::availableTools() const
{
    const auto mk = [](const QString& id, const QString& name, const QString& desc) {
        QVariantMap m;
        m[QStringLiteral("id")] = id;
        m[QStringLiteral("name")] = name;
        m[QStringLiteral("description")] = desc;
        return QVariant(m);
    };
    return {
        mk(QStringLiteral("read"), QStringLiteral("Read files"),
           QStringLiteral("Read workspace files.")),
        mk(QStringLiteral("write"), QStringLiteral("Write files"),
           QStringLiteral("Create and edit files.")),
        mk(QStringLiteral("shell"), QStringLiteral("Shell"),
           QStringLiteral("Run shell commands.")),
        mk(QStringLiteral("search"), QStringLiteral("Search"),
           QStringLiteral("Grep / find across the workspace.")),
        mk(QStringLiteral("browse"), QStringLiteral("Browse"),
           QStringLiteral("Fetch and read web pages.")),
    };
}

QString MockProfileStore::createProfile(const QString& name)
{
    const QString id = QStringLiteral("prof-%1").arg(m_nextId++);
    m_profiles->upsert(mkProfile(id, name.isEmpty() ? QStringLiteral("New profile") : name,
                                 QStringLiteral("llama-3.1-8b-instruct"), QString(),
                                 QStringLiteral("You are a helpful assistant."),
                                 { QStringLiteral("web-search") },
                                 { QStringLiteral("read"), QStringLiteral("search") }, false));
    save();
    emit changed();
    return id;
}

void MockProfileStore::updateProfile(const QString& id, const QVariantMap& fields)
{
    const int row = m_profiles->indexOfId(id);
    if (row < 0) {
        return;
    }
    QVariantMap p = m_profiles->at(row);
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        p.insert(it.key(), it.value());
    }
    m_profiles->upsert(p);
    save();
    emit changed();
}

void MockProfileStore::remove(const QString& id)
{
    m_profiles->removeById(id);
    if (m_defaultId == id) {
        m_defaultId = m_profiles->count() > 0
            ? m_profiles->at(0).value(QStringLiteral("id")).toString()
            : QString();
    }
    save();
    emit changed();
}

void MockProfileStore::setDefault(const QString& id)
{
    if (m_profiles->indexOfId(id) < 0 || m_defaultId == id) {
        return;
    }
    for (const QVariantMap& row : m_profiles->rows()) {
        const QString rid = row.value(QStringLiteral("id")).toString();
        const bool shouldDefault = (rid == id);
        if (row.value(QStringLiteral("isDefault")).toBool() != shouldDefault) {
            QVariantMap updated = row;
            updated[QStringLiteral("isDefault")] = shouldDefault;
            m_profiles->upsert(updated);
        }
    }
    m_defaultId = id;
    save();
    emit changed();
}

} // namespace profiles
