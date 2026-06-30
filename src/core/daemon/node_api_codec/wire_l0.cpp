// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// The multiplexed socket envelope (wire L0; daemon-sync-protocol-spec.md S2). Hand-coded canonical
// CBOR (not zcbor-generated): the frame encoders wrap already-encoded request bytes, and
// decodeWireFrame slices the inner ApiResponse bytes back out, via the codec_detail CBOR
// primitives.

#include "daemon/node_api_codec/internal.h"

namespace daemonapp::daemon {

using namespace codec_detail;
QByteArray NodeApiCodec::encodeHelloFrame() {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "Hello");
    b.append(static_cast<char>(0xA2)); // map(2): wire_version, features
    cborAppendText(b, "wire_version");
    cborAppendUint(b, kWireVersion);
    cborAppendText(b, "features");
    b.append(static_cast<char>(0x82)); // array(2)
    cborAppendText(b, "mux");
    cborAppendText(b, "stream");
    return b;
}

QByteArray NodeApiCodec::encodeCallFrame(quint64 id, const QByteArray& requestCbor) {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "Call");
    b.append(static_cast<char>(0xA2)); // map(2): id, req
    cborAppendText(b, "id");
    cborAppendUint(b, id);
    cborAppendText(b, "req");
    b.append(requestCbor); // req is the final key, so the daemon decodes it as the trailing item
    return b;
}

QByteArray NodeApiCodec::encodeOpenFrame(quint64 id, const QByteArray& requestCbor) {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "Open");
    b.append(static_cast<char>(0xA2)); // map(2): id, req
    cborAppendText(b, "id");
    cborAppendUint(b, id);
    cborAppendText(b, "req");
    b.append(requestCbor);
    return b;
}

QByteArray NodeApiCodec::encodeCancelFrame(quint64 id) {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "Cancel");
    b.append(static_cast<char>(0xA1)); // map(1): id
    cborAppendText(b, "id");
    cborAppendUint(b, id);
    return b;
}

QByteArray NodeApiCodec::encodeAuthStartFrame(const QString& mechanism, const QByteArray& initial) {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "AuthStart");
    b.append(static_cast<char>(0xA2)); // map(2): mechanism, initial
    cborAppendText(b, "mechanism");
    cborAppendTextLen(b, mechanism.toUtf8());
    cborAppendText(b, "initial");
    cborAppendBytesAsUintArray(b, initial); // `[* uint]`, NOT a bstr (frozen contract)
    return b;
}

QByteArray NodeApiCodec::encodeAuthStepFrame(const QByteArray& data) {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "AuthStep");
    b.append(static_cast<char>(0xA1)); // map(1): data
    cborAppendText(b, "data");
    cborAppendBytesAsUintArray(b, data);
    return b;
}

QByteArray NodeApiCodec::encodeAuthResumeFrame(const QString& token) {
    QByteArray b;
    b.append(static_cast<char>(0xA1)); // map(1)
    cborAppendText(b, "AuthResume");
    b.append(static_cast<char>(0xA1)); // map(1): token
    cborAppendText(b, "token");
    cborAppendTextLen(b, token.toUtf8());
    return b;
}

bool NodeApiCodec::decodeWireFrame(const QByteArray& frameCbor, DecodedWireFrame* out) {
    if (out == nullptr) {
        return false;
    }
    *out = DecodedWireFrame{};
    const auto* p = reinterpret_cast<const uchar*>(frameCbor.constData());
    const qsizetype n = frameCbor.size();
    qsizetype i = 0;
    quint8 major = 0;
    quint64 arg = 0;
    if (!cborReadHead(p, n, i, major, arg) || major != 5 || arg != 1) {
        return false; // outer must be map(1) { <tag>: ... }
    }
    QByteArray tag;
    if (!cborReadText(p, n, i, &tag)) {
        return false;
    }
    if (!cborReadHead(p, n, i, major, arg) || major != 5) {
        return false; // inner map
    }
    const quint64 innerFields = arg;
    if (tag == "Hello") {
        out->kind = WireFrameKind::Hello;
        // Inner map is {wire_version, features}. Surface the capability list so the client can gate
        // optional surfaces (e.g. versioning). Parse by key; the only non-array value
        // (wire_version) is a uint whose head cborReadHead consumes whole.
        for (quint64 field = 0; field < innerFields; ++field) {
            QByteArray hkey;
            if (!cborReadText(p, n, i, &hkey)) {
                break;
            }
            if (hkey == "features" || hkey == "auth_mechanisms") {
                quint8 fmaj = 0;
                quint64 farg = 0;
                if (!cborReadHead(p, n, i, fmaj, farg) || fmaj != 4) {
                    break; // expected an array
                }
                QStringList& sink = (hkey == "features") ? out->features : out->authMechanisms;
                for (quint64 k = 0; k < farg; ++k) {
                    QByteArray item;
                    if (!cborReadText(p, n, i, &item)) {
                        break;
                    }
                    sink.append(QString::fromUtf8(item));
                }
            } else if (!cborReadHead(p, n, i, major, arg)) {
                break; // wire_version (uint) or another scalar
            }
        }
        return true;
    }
    // The auth (SASL) frames do not carry an `id`; handle them before the id-leading frames below.
    if (tag == "AuthChallenge") {
        out->kind = WireFrameKind::AuthChallenge;
        QByteArray key;
        return cborReadText(p, n, i, &key) && key == "data" &&
               cborReadBytesFromUintArray(p, n, i, &out->authData);
    }
    if (tag == "AuthError") {
        out->kind = WireFrameKind::AuthError;
        QByteArray key;
        QByteArray reason;
        if (!cborReadText(p, n, i, &key) || key != "reason" || !cborReadText(p, n, i, &reason)) {
            return false;
        }
        out->authReason = QString::fromUtf8(reason);
        return true;
    }
    if (tag == "AuthOk") {
        out->kind = WireFrameKind::AuthOk;
        QByteArray key;
        QByteArray token;
        if (!cborReadText(p, n, i, &key) || key != "token" || !cborReadText(p, n, i, &token)) {
            return false;
        }
        out->authToken = QString::fromUtf8(token);
        if (!cborReadText(p, n, i, &key) || key != "principal") {
            return false;
        }
        // principal-view: a map of {user_id, username, roles, capabilities}; parse by key.
        quint8 pmaj = 0;
        quint64 pfields = 0;
        if (!cborReadHead(p, n, i, pmaj, pfields) || pmaj != 5) {
            return false;
        }
        for (quint64 f = 0; f < pfields; ++f) {
            QByteArray pkey;
            if (!cborReadText(p, n, i, &pkey)) {
                return false;
            }
            if (pkey == "user_id" || pkey == "username") {
                QByteArray val;
                if (!cborReadText(p, n, i, &val)) {
                    return false;
                }
                (pkey == "user_id" ? out->principal.userId : out->principal.username) =
                    QString::fromUtf8(val);
            } else if (pkey == "roles" || pkey == "capabilities") {
                quint8 amaj = 0;
                quint64 alen = 0;
                if (!cborReadHead(p, n, i, amaj, alen) || amaj != 4) {
                    return false;
                }
                QStringList& sink =
                    (pkey == "roles") ? out->principal.roles : out->principal.capabilities;
                for (quint64 k = 0; k < alen; ++k) {
                    QByteArray item;
                    if (!cborReadText(p, n, i, &item)) {
                        return false;
                    }
                    sink.append(QString::fromUtf8(item));
                }
            } else if (!cborReadHead(p, n, i, major, arg)) {
                return false; // unknown scalar field (forward-compat): consume its head
            }
        }
        return true;
    }
    // Every other frame leads with "id" (declaration order). Read it.
    QByteArray key;
    if (!cborReadText(p, n, i, &key) || key != "id") {
        return false;
    }
    if (!cborReadHead(p, n, i, major, arg) || major != 0) {
        return false;
    }
    out->id = arg;
    if (tag == "Reply" || tag == "Item") {
        if (!cborReadText(p, n, i, &key) || key != "res") {
            return false;
        }
        // "res" is the final field, so the remainder of the frame is the inner ApiResponse item.
        out->payload = frameCbor.mid(i);
        out->kind = (tag == "Item") ? WireFrameKind::Item : WireFrameKind::Reply;
        return true;
    }
    if (tag == "End") {
        out->kind = WireFrameKind::End;
        if (innerFields >= 2 && cborReadText(p, n, i, &key) && key == "error") {
            // null (0xF6) means a clean close; anything else (or a missing value) is an error.
            out->hasError = (i >= n) || (p[i] != 0xF6);
        }
        return true;
    }
    if (tag == "Reset") {
        out->kind = WireFrameKind::Reset;
        if (cborReadText(p, n, i, &key) && key == "epoch" && cborReadHead(p, n, i, major, arg)) {
            out->epoch = arg;
        }
        if (cborReadText(p, n, i, &key) && key == "head_seq" && cborReadHead(p, n, i, major, arg)) {
            out->headSeq = arg;
        }
        return true;
    }
    return false;
}

} // namespace daemonapp::daemon
