#pragma once
// Generic reflection helpers over G1's generated Q_GADGET entities — value equality and a
// canonical field dump, derived from the Q_GADGET meta-object so no per-field (and no
// generated/) code is needed. immer::diff hard-codes std::equal_to<value_type>, so entity
// value equality (model.h's operator==) routes through gadget_equal here; the canonical dump
// backs the §12/E4 "sorted store dump" comparisons and the §4.4 journal-replay property test.

#include <QMetaProperty>
#include <QString>
#include <QVariant>

namespace mirror::reflect {

// Field-by-field equality via the gadget meta-object. Two entities of the same generated type
// are equal iff every Q_PROPERTY compares equal as a QVariant.
template <typename T>
inline bool gadget_equal(const T& lhs, const T& rhs) noexcept {
    const QMetaObject& mo = T::staticMetaObject;
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        const QMetaProperty p = mo.property(i);
        if (p.readOnGadget(&lhs) != p.readOnGadget(&rhs)) {
            return false;
        }
    }
    return true;
}

// Canonical, stable, human-diffable dump of one entity: "field=repr|field=repr|...". Property
// order is the declaration order the generator emits (deterministic), so two dumps are
// byte-comparable. Used to assert snapshot equality without a bespoke per-type serializer.
template <typename T>
inline QString gadget_dump(const T& e) {
    const QMetaObject& mo = T::staticMetaObject;
    QString out;
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        const QMetaProperty p = mo.property(i);
        if (i > mo.propertyOffset()) {
            out += QLatin1Char('|');
        }
        out += QString::fromLatin1(p.name());
        out += QLatin1Char('=');
        out += p.readOnGadget(&e).toString();
    }
    return out;
}

} // namespace mirror::reflect
