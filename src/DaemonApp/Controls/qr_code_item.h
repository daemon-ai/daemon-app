// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "qr/qr_matrix.h"

#include <QColor>
#include <QQuickPaintedItem>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

namespace daemonapp::controls {

// [integrations wire v38] Kit.QrCode — the Daemon Kit QR control (DaemonApp.Controls,
// `Kit.QrCode`): a QQuickPaintedItem that encodes `payload` to a qr::QrMatrix and paints the module
// grid with a quiet zone. It renders the SAME matrix the TUI auth panel projects to half-blocks, so
// the generic auth component's QR challenge looks identical on both surfaces. Nothing
// protocol-specific: it takes an opaque payload string (the node's AuthChallenge.qr payload) and
// draws it.
//
// Colors default to the QR spec (black modules on a white quiet zone) — high-contrast and scanner-
// safe in every theme; a caller may re-tint via darkColor/lightColor but MUST keep dark darker than
// light or scanners fail. The surrounding card/border is themed by the QML consumer.
class QrCode : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString payload READ payload WRITE setPayload NOTIFY payloadChanged)
    Q_PROPERTY(QColor darkColor READ darkColor WRITE setDarkColor NOTIFY darkColorChanged)
    Q_PROPERTY(QColor lightColor READ lightColor WRITE setLightColor NOTIFY lightColorChanged)
    Q_PROPERTY(int quietZone READ quietZone WRITE setQuietZone NOTIFY quietZoneChanged)
    // False when `payload` is empty or cannot be encoded (too long): the consumer then shows the
    // copyable payload text / redirect instead of an empty box.
    Q_PROPERTY(bool valid READ valid NOTIFY payloadChanged)

public:
    explicit QrCode(QQuickItem* parent = nullptr);

    [[nodiscard]] QString payload() const { return m_payload; }
    void setPayload(const QString& payload);

    [[nodiscard]] QColor darkColor() const { return m_darkColor; }
    void setDarkColor(const QColor& color);

    [[nodiscard]] QColor lightColor() const { return m_lightColor; }
    void setLightColor(const QColor& color);

    [[nodiscard]] int quietZone() const { return m_quietZone; }
    void setQuietZone(int modules);

    [[nodiscard]] bool valid() const { return m_matrix.isValid(); }

    void paint(QPainter* painter) override;

signals:
    void payloadChanged();
    void darkColorChanged();
    void lightColorChanged();
    void quietZoneChanged();

private:
    QString m_payload;
    QColor m_darkColor{Qt::black};
    QColor m_lightColor{Qt::white};
    int m_quietZone = 4; // QR spec default quiet zone (modules)
    qr::QrMatrix m_matrix;
};

} // namespace daemonapp::controls
