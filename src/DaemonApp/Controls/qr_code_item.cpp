// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "qr_code_item.h"

#include <QPainter>

namespace daemonapp::controls {

QrCode::QrCode(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setImplicitWidth(200);
    setImplicitHeight(200);
}

void QrCode::setPayload(const QString& payload) {
    if (m_payload == payload) {
        return;
    }
    m_payload = payload;
    m_matrix = qr::QrMatrix::encode(m_payload);
    emit payloadChanged();
    update();
}

void QrCode::setDarkColor(const QColor& color) {
    if (m_darkColor == color) {
        return;
    }
    m_darkColor = color;
    emit darkColorChanged();
    update();
}

void QrCode::setLightColor(const QColor& color) {
    if (m_lightColor == color) {
        return;
    }
    m_lightColor = color;
    emit lightColorChanged();
    update();
}

void QrCode::setQuietZone(int modules) {
    const int clamped = modules > 0 ? modules : 0;
    if (m_quietZone == clamped) {
        return;
    }
    m_quietZone = clamped;
    emit quietZoneChanged();
    update();
}

void QrCode::paint(QPainter* painter) {
    // The quiet zone is a light band framing the code, so fill the whole item with the light color
    // even when there is nothing to draw (an invalid payload leaves a clean light square).
    const qreal w = width();
    const qreal h = height();
    painter->fillRect(QRectF(0, 0, w, h), m_lightColor);
    if (!m_matrix.isValid()) {
        return;
    }

    const int modules = m_matrix.size() + (2 * m_quietZone); // grid side, incl. quiet zone
    // Integer module pixels keep the finder patterns crisp (no seams from fractional rects); center
    // the resulting grid in the item.
    const int px = static_cast<int>(qMin(w, h)) / modules;
    if (px <= 0) {
        return; // too small to render a scannable code
    }
    const int drawn = px * modules;
    const qreal originX = (w - drawn) / 2.0;
    const qreal originY = (h - drawn) / 2.0;

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_darkColor);
    for (int y = 0; y < m_matrix.size(); ++y) {
        for (int x = 0; x < m_matrix.size(); ++x) {
            if (!m_matrix.module(x, y)) {
                continue;
            }
            const qreal rx = originX + (static_cast<qreal>(x + m_quietZone) * px);
            const qreal ry = originY + (static_cast<qreal>(y + m_quietZone) * px);
            painter->drawRect(QRectF(rx, ry, px, px));
        }
    }
}

} // namespace daemonapp::controls
