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
            if (hkey == "features") {
                quint8 fmaj = 0;
                quint64 farg = 0;
                if (!cborReadHead(p, n, i, fmaj, farg) || fmaj != 4) {
                    break; // expected an array
                }
                for (quint64 k = 0; k < farg; ++k) {
                    QByteArray feat;
                    if (!cborReadText(p, n, i, &feat)) {
                        break;
                    }
                    out->features.append(QString::fromUtf8(feat));
                }
            } else if (!cborReadHead(p, n, i, major, arg)) {
                break; // wire_version (uint) or another scalar
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
