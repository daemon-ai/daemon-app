#include "models/mock_model_catalog.h"

#include "appcache/json_cache.h"

#include <QTimer>

namespace models {
namespace {

const QString kCacheFile = QStringLiteral("models.json");


QVariantMap mk(const QString& id, const QString& name, const QString& provider,
               const QString& params, double gib, const QString& quant, const QString& blurb)
{
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("name")] = name;
    m[QStringLiteral("provider")] = provider;
    m[QStringLiteral("params")] = params;
    m[QStringLiteral("sizeGiB")] = gib;
    m[QStringLiteral("quant")] = quant;
    m[QStringLiteral("blurb")] = blurb;
    // Numeric params bucket for the size filter.
    m[QStringLiteral("paramsB")] = params.chopped(1).toDouble(); // "8B" -> 8
    return m;
}

} // namespace

MockModelCatalog::MockModelCatalog(QObject* parent)
    : IModelCatalog(parent)
    , m_discover(new uimodels::VariantListModel(this))
    , m_downloads(new uimodels::VariantListModel(this))
    , m_installed(new uimodels::VariantListModel(this))
    , m_timer(new QTimer(this))
{
    m_catalog = {
        mk(QStringLiteral("llama-3.1-8b-instruct"), QStringLiteral("Llama 3.1 8B Instruct"),
           QStringLiteral("Meta"), QStringLiteral("8B"), 4.7, QStringLiteral("Q4_K_M"),
           QStringLiteral("Balanced general-purpose instruct model.")),
        mk(QStringLiteral("llama-3.1-70b-instruct"), QStringLiteral("Llama 3.1 70B Instruct"),
           QStringLiteral("Meta"), QStringLiteral("70B"), 40.0, QStringLiteral("Q4_K_M"),
           QStringLiteral("High-quality reasoning, needs a big GPU.")),
        mk(QStringLiteral("mistral-7b-instruct"), QStringLiteral("Mistral 7B Instruct"),
           QStringLiteral("Mistral"), QStringLiteral("7B"), 4.1, QStringLiteral("Q4_K_M"),
           QStringLiteral("Fast, lightweight, great default.")),
        mk(QStringLiteral("mixtral-8x7b"), QStringLiteral("Mixtral 8x7B"),
           QStringLiteral("Mistral"), QStringLiteral("47B"), 26.0, QStringLiteral("Q4_K_M"),
           QStringLiteral("Sparse MoE, strong quality/speed tradeoff.")),
        mk(QStringLiteral("qwen2.5-7b-instruct"), QStringLiteral("Qwen2.5 7B Instruct"),
           QStringLiteral("Alibaba"), QStringLiteral("7B"), 4.4, QStringLiteral("Q4_K_M"),
           QStringLiteral("Strong multilingual + coding.")),
        mk(QStringLiteral("qwen2.5-coder-32b"), QStringLiteral("Qwen2.5 Coder 32B"),
           QStringLiteral("Alibaba"), QStringLiteral("32B"), 18.0, QStringLiteral("Q4_K_M"),
           QStringLiteral("Specialised for code generation.")),
        mk(QStringLiteral("gemma-2-9b-it"), QStringLiteral("Gemma 2 9B IT"),
           QStringLiteral("Google"), QStringLiteral("9B"), 5.4, QStringLiteral("Q4_K_M"),
           QStringLiteral("Compact, well-rounded instruct model.")),
        mk(QStringLiteral("phi-3.5-mini"), QStringLiteral("Phi 3.5 Mini"),
           QStringLiteral("Microsoft"), QStringLiteral("4B"), 2.4, QStringLiteral("Q4_K_M"),
           QStringLiteral("Tiny but punches above its weight.")),
        mk(QStringLiteral("deepseek-r1-distill-8b"), QStringLiteral("DeepSeek R1 Distill 8B"),
           QStringLiteral("DeepSeek"), QStringLiteral("8B"), 4.9, QStringLiteral("Q4_K_M"),
           QStringLiteral("Reasoning-distilled, chain-of-thought.")),
    };

    // Seed one installed model (matches the daemon-config default) so the
    // Installed tab and active marker aren't empty on first open.
    QVariantMap seeded = m_catalog.first();
    seeded[QStringLiteral("active")] = true;
    m_installed->upsert(seeded);
    m_currentId = seeded.value(QStringLiteral("id")).toString();

    m_timer->setInterval(250);
    connect(m_timer, &QTimer::timeout, this, &MockModelCatalog::tick);

    // Restore the last-known installed set + active model over the seed (no-op on
    // first run). In-flight downloads are intentionally not persisted.
    const QJsonObject cached = appcache::loadObject(kCacheFile);
    if (cached.contains(QStringLiteral("installed"))) {
        m_installed->setRows(
            appcache::rowsFromJson(cached.value(QStringLiteral("installed")).toArray()));
        m_currentId = cached.value(QStringLiteral("currentId")).toString(m_currentId);
    }

    search(QString(), QString()); // populate Discover with everything
}

void MockModelCatalog::save() const
{
    QJsonObject obj;
    obj[QStringLiteral("installed")] = appcache::rowsToJson(m_installed->rows());
    obj[QStringLiteral("currentId")] = m_currentId;
    appcache::saveObject(kCacheFile, obj);
}

QObject* MockModelCatalog::discover() const { return m_discover; }
QObject* MockModelCatalog::downloads() const { return m_downloads; }
QObject* MockModelCatalog::installed() const { return m_installed; }

QStringList MockModelCatalog::installedIds() const
{
    QStringList out;
    for (const QVariantMap& row : m_installed->rows()) {
        out << row.value(QStringLiteral("id")).toString();
    }
    return out;
}

QVariantList MockModelCatalog::providers() const
{
    const auto mkp = [](const QString& id, const QString& name, const QString& kind,
                        bool configured) {
        QVariantMap m;
        m[QStringLiteral("id")] = id;
        m[QStringLiteral("name")] = name;
        m[QStringLiteral("kind")] = kind; // "local" | "remote"
        m[QStringLiteral("configured")] = configured;
        return QVariant(m);
    };
    return {
        mkp(QStringLiteral("huggingface"), QStringLiteral("Hugging Face"),
            QStringLiteral("local"), true),
        mkp(QStringLiteral("ollama"), QStringLiteral("Ollama"), QStringLiteral("local"), true),
        mkp(QStringLiteral("openai"), QStringLiteral("OpenAI"), QStringLiteral("remote"), false),
        mkp(QStringLiteral("anthropic"), QStringLiteral("Anthropic"), QStringLiteral("remote"),
            false),
        mkp(QStringLiteral("openrouter"), QStringLiteral("OpenRouter"), QStringLiteral("remote"),
            false),
    };
}

QVariantMap MockModelCatalog::catalogEntry(const QString& modelId) const
{
    for (const QVariantMap& m : m_catalog) {
        if (m.value(QStringLiteral("id")).toString() == modelId) {
            return m;
        }
    }
    return {};
}

void MockModelCatalog::search(const QString& query, const QString& sizeFilter)
{
    const QString q = query.trimmed().toLower();
    QList<QVariantMap> out;
    for (QVariantMap m : m_catalog) {
        const QString hay =
            (m.value(QStringLiteral("name")).toString() + QLatin1Char(' ')
             + m.value(QStringLiteral("provider")).toString() + QLatin1Char(' ')
             + m.value(QStringLiteral("id")).toString())
                .toLower();
        if (!q.isEmpty() && !hay.contains(q)) {
            continue;
        }
        const double pb = m.value(QStringLiteral("paramsB")).toDouble();
        if (sizeFilter == QLatin1String("<=8B") && pb > 8.0) {
            continue;
        }
        if (sizeFilter == QLatin1String(">8B") && pb <= 8.0) {
            continue;
        }
        m[QStringLiteral("installed")] = m_installed->indexOfId(m.value(QStringLiteral("id"))) >= 0;
        out.append(m);
    }
    m_discover->setRows(out);
}

void MockModelCatalog::download(const QString& modelId)
{
    if (m_installed->indexOfId(modelId) >= 0) {
        return; // already installed
    }
    const QString jobId = QStringLiteral("job-%1").arg(m_nextJob++);
    QVariantMap entry = catalogEntry(modelId);
    QVariantMap job;
    job[QStringLiteral("id")] = jobId;
    job[QStringLiteral("modelId")] = modelId;
    job[QStringLiteral("name")] = entry.value(QStringLiteral("name"));
    job[QStringLiteral("sizeGiB")] = entry.value(QStringLiteral("sizeGiB"));
    job[QStringLiteral("progress")] = 0.0;
    job[QStringLiteral("state")] = QStringLiteral("downloading");
    m_downloads->upsert(job);
    m_jobProgress[jobId] = 0.0;
    m_jobPaused[jobId] = false;
    m_jobModel[jobId] = modelId;
    if (!m_timer->isActive()) {
        m_timer->start();
    }
}

void MockModelCatalog::pauseDownload(const QString& jobId)
{
    if (!m_jobModel.contains(jobId)) {
        return;
    }
    m_jobPaused[jobId] = true;
    const int row = m_downloads->indexOfId(jobId);
    if (row >= 0) {
        QVariantMap j = m_downloads->at(row);
        j[QStringLiteral("state")] = QStringLiteral("paused");
        m_downloads->upsert(j);
    }
}

void MockModelCatalog::resumeDownload(const QString& jobId)
{
    if (!m_jobModel.contains(jobId)) {
        return;
    }
    m_jobPaused[jobId] = false;
    const int row = m_downloads->indexOfId(jobId);
    if (row >= 0) {
        QVariantMap j = m_downloads->at(row);
        j[QStringLiteral("state")] = QStringLiteral("downloading");
        m_downloads->upsert(j);
    }
    if (!m_timer->isActive()) {
        m_timer->start();
    }
}

void MockModelCatalog::cancelDownload(const QString& jobId)
{
    m_jobProgress.remove(jobId);
    m_jobPaused.remove(jobId);
    m_jobModel.remove(jobId);
    m_downloads->removeById(jobId);
}

void MockModelCatalog::tick()
{
    QStringList finished;
    for (auto it = m_jobProgress.begin(); it != m_jobProgress.end(); ++it) {
        const QString& jobId = it.key();
        if (m_jobPaused.value(jobId)) {
            continue;
        }
        it.value() = qMin(1.0, it.value() + 0.07);
        const int row = m_downloads->indexOfId(jobId);
        if (row >= 0) {
            QVariantMap j = m_downloads->at(row);
            j[QStringLiteral("progress")] = it.value();
            j[QStringLiteral("state")] =
                it.value() >= 1.0 ? QStringLiteral("done") : QStringLiteral("downloading");
            m_downloads->upsert(j);
        }
        if (it.value() >= 1.0) {
            finished.append(jobId);
        }
    }

    for (const QString& jobId : finished) {
        const QString modelId = m_jobModel.value(jobId);
        m_jobProgress.remove(jobId);
        m_jobPaused.remove(jobId);
        m_jobModel.remove(jobId);
        m_downloads->removeById(jobId);

        QVariantMap entry = catalogEntry(modelId);
        entry[QStringLiteral("active")] = false;
        m_installed->upsert(entry);
        search(QString(), QString()); // refresh installed flags in Discover
        emit downloadFinished(modelId);
    }
    if (!finished.isEmpty()) {
        save(); // a newly installed model is durable state
    }

    if (m_jobProgress.isEmpty()) {
        m_timer->stop();
    }
}

void MockModelCatalog::activate(const QString& modelId)
{
    if (m_installed->indexOfId(modelId) < 0 || m_currentId == modelId) {
        return;
    }
    // Clear the previous active flag, set the new one.
    for (const QVariantMap& row : m_installed->rows()) {
        const QString id = row.value(QStringLiteral("id")).toString();
        const bool shouldBeActive = (id == modelId);
        if (row.value(QStringLiteral("active")).toBool() != shouldBeActive) {
            QVariantMap updated = row;
            updated[QStringLiteral("active")] = shouldBeActive;
            m_installed->upsert(updated);
        }
    }
    m_currentId = modelId;
    save();
    emit currentChanged();
}

void MockModelCatalog::remove(const QString& modelId)
{
    m_installed->removeById(modelId);
    if (m_currentId == modelId) {
        m_currentId.clear();
        emit currentChanged();
    }
    save();
    search(QString(), QString());
}

} // namespace models
