// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/semver.h"

#include <QStringList>

namespace update::semver {

namespace {

// Compare two numeric core components; a non-numeric component compares as 0 (it
// never on its own promotes a malformed version to an upgrade).
int compareNumeric(const QString& a, const QString& b) {
    bool okA = false;
    bool okB = false;
    const qulonglong na = a.toULongLong(&okA);
    const qulonglong nb = b.toULongLong(&okB);
    if (!okA || !okB) {
        return 0;
    }
    if (na < nb) {
        return -1;
    }
    if (na > nb) {
        return 1;
    }
    return 0;
}

// Compare one prerelease identifier per SemVer 2.0.0 §11.4: numeric identifiers
// rank below alphanumeric ones and compare numerically; alphanumeric compare by
// ASCII.
int comparePrereleaseId(const QString& a, const QString& b) {
    bool numA = false;
    bool numB = false;
    const qulonglong na = a.toULongLong(&numA);
    const qulonglong nb = b.toULongLong(&numB);
    if (numA && numB) {
        if (na < nb) {
            return -1;
        }
        return na > nb ? 1 : 0;
    }
    if (numA != numB) {
        return numA ? -1 : 1; // numeric < alphanumeric
    }
    return QString::compare(a, b, Qt::CaseSensitive) < 0   ? -1
           : QString::compare(a, b, Qt::CaseSensitive) > 0 ? 1
                                                           : 0;
}

int comparePrerelease(const QString& a, const QString& b) {
    // A missing prerelease outranks a present one (release > prerelease).
    if (a.isEmpty() && b.isEmpty()) {
        return 0;
    }
    if (a.isEmpty()) {
        return 1;
    }
    if (b.isEmpty()) {
        return -1;
    }
    const QStringList idsA = a.split(QLatin1Char('.'));
    const QStringList idsB = b.split(QLatin1Char('.'));
    const qsizetype n = qMin(idsA.size(), idsB.size());
    for (qsizetype i = 0; i < n; ++i) {
        const int c = comparePrereleaseId(idsA.at(i), idsB.at(i));
        if (c != 0) {
            return c;
        }
    }
    // All shared identifiers equal: the longer set has higher precedence.
    if (idsA.size() < idsB.size()) {
        return -1;
    }
    return idsA.size() > idsB.size() ? 1 : 0;
}

} // namespace

QString stripBuildMetadata(const QString& version) {
    QString v = version.trimmed();
    if (v.startsWith(QLatin1Char('v')) || v.startsWith(QLatin1Char('V'))) {
        v = v.mid(1);
    }
    const int plus = static_cast<int>(v.indexOf(QLatin1Char('+')));
    if (plus >= 0) {
        v = v.left(plus);
    }
    return v;
}

int compare(const QString& a, const QString& b) {
    const QString va = stripBuildMetadata(a);
    const QString vb = stripBuildMetadata(b);

    // Split core from prerelease at the first '-'.
    const auto split = [](const QString& v, QString& core, QString& pre) {
        const int dash = static_cast<int>(v.indexOf(QLatin1Char('-')));
        if (dash >= 0) {
            core = v.left(dash);
            pre = v.mid(dash + 1);
        } else {
            core = v;
            pre.clear();
        }
    };
    QString coreA;
    QString preA;
    QString coreB;
    QString preB;
    split(va, coreA, preA);
    split(vb, coreB, preB);

    const QStringList partsA = coreA.split(QLatin1Char('.'));
    const QStringList partsB = coreB.split(QLatin1Char('.'));
    for (int i = 0; i < 3; ++i) {
        const QString pa = i < partsA.size() ? partsA.at(i) : QStringLiteral("0");
        const QString pb = i < partsB.size() ? partsB.at(i) : QStringLiteral("0");
        const int c = compareNumeric(pa, pb);
        if (c != 0) {
            return c;
        }
    }
    return comparePrerelease(preA, preB);
}

bool isUpgrade(const QString& candidate, const QString& current) {
    return compare(candidate, current) > 0;
}

} // namespace update::semver
