#include "daemon/daemon_model_catalog.h"

#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"

namespace models {

DaemonModelCatalog::DaemonModelCatalog(daemonapp::daemon::ModelRepository* models, QObject* parent)
    : IModelCatalog(parent), m_models(models), m_discover(new uimodels::VariantListModel(this)),
      m_downloads(new uimodels::VariantListModel(this)),
      m_installed(new uimodels::VariantListModel(this)) {
    if (m_models != nullptr) {
        connect(m_models, &daemonapp::daemon::ModelRepository::modelsRefreshed, this,
                &DaemonModelCatalog::rebuild);
        connect(m_models, &daemonapp::daemon::ModelRepository::currentRefreshed, this, [this] {
            rebuild();
            emit currentChanged();
        });
    }
}

QObject* DaemonModelCatalog::discover() const {
    return m_discover;
}
QObject* DaemonModelCatalog::downloads() const {
    return m_downloads;
}
QObject* DaemonModelCatalog::installed() const {
    return m_installed;
}

QString DaemonModelCatalog::currentModelId() const {
    if (!m_pickedId.isEmpty()) {
        return m_pickedId;
    }
    return (m_models != nullptr && m_models->hasCurrent()) ? m_models->currentModel().id
                                                           : QString();
}

QStringList DaemonModelCatalog::installedIds() const {
    QStringList out;
    if (m_models != nullptr) {
        for (const daemonapp::daemon::DecodedModelDescriptor& d : m_models->models()) {
            out << d.id;
        }
    }
    return out;
}

QVariantList DaemonModelCatalog::providers() const {
    const auto mkp = [](const QString& id, const QString& name, const QString& kind) {
        QVariantMap m;
        m[QStringLiteral("id")] = id;
        m[QStringLiteral("name")] = name;
        m[QStringLiteral("kind")] = kind; // "local" | "remote"
        m[QStringLiteral("configured")] = false;
        return QVariant(m);
    };
    return {
        mkp(QStringLiteral("anthropic"), QStringLiteral("Anthropic"), QStringLiteral("remote")),
        mkp(QStringLiteral("openai"), QStringLiteral("OpenAI"), QStringLiteral("remote")),
        mkp(QStringLiteral("openrouter"), QStringLiteral("OpenRouter"), QStringLiteral("remote")),
        mkp(QStringLiteral("google"), QStringLiteral("Google"), QStringLiteral("remote")),
    };
}

void DaemonModelCatalog::search(const QString& query, const QString& sizeFilter) {
    Q_UNUSED(query)
    Q_UNUSED(sizeFilter)
    // Re-discover from the daemon; filtering is applied in the projection if needed later.
    if (m_models != nullptr) {
        m_models->refreshModels();
    }
}

void DaemonModelCatalog::download(const QString& modelId) {
    Q_UNUSED(modelId) // cloud models are usable without a local download
}
void DaemonModelCatalog::pauseDownload(const QString& jobId) {
    Q_UNUSED(jobId)
}
void DaemonModelCatalog::resumeDownload(const QString& jobId) {
    Q_UNUSED(jobId)
}
void DaemonModelCatalog::cancelDownload(const QString& jobId) {
    Q_UNUSED(jobId)
}

void DaemonModelCatalog::activate(const QString& modelId) {
    if (m_pickedId == modelId) {
        return;
    }
    m_pickedId = modelId;
    rebuild();
    emit currentChanged();
}

void DaemonModelCatalog::remove(const QString& modelId) {
    Q_UNUSED(modelId) // cloud models are not locally removable
}

void DaemonModelCatalog::rebuild() {
    if (m_models == nullptr) {
        return;
    }
    const QString current = currentModelId();
    QList<QVariantMap> rows;
    for (const daemonapp::daemon::DecodedModelDescriptor& d : m_models->models()) {
        QVariantMap row;
        row[QStringLiteral("id")] = d.id;
        row[QStringLiteral("name")] = d.id;
        row[QStringLiteral("provider")] = d.provider;
        row[QStringLiteral("params")] = QString();
        row[QStringLiteral("sizeGiB")] = 0.0;
        row[QStringLiteral("quant")] = QString();
        row[QStringLiteral("blurb")] = QString();
        row[QStringLiteral("paramsB")] = 0.0;
        row[QStringLiteral("installed")] = true;
        row[QStringLiteral("active")] = (d.id == current);
        rows.append(row);
    }
    // Cloud models are immediately usable: surface them as the "installed" set the composer model
    // picker + the onboarding model card bind to. Discovery mirrors the same set.
    m_installed->setRows(rows);
    m_discover->setRows(rows);
}

} // namespace models
