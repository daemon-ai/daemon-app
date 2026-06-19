#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QRawFont>
#include <QString>
#include <QStringList>
#include <QtTest>

#include <array>

// Validates the bundled font assets so we never ship "tofu" icons again.
//
// Root cause this guards against: FontAwesome's solid and regular faces share
// the family name "Font Awesome 6 Free"; if both are loaded, binding a glyph to
// that family can resolve to Regular (which lacks the solid glyphs). We also use
// FontAwesome Free, where some glyphs are absent - this test pins
// the exact codepoints the UI depends on to the exact bundled file.
class TestFonts : public QObject {
    Q_OBJECT

private:
    // The fonts directory is injected by CMake so the test reads the same files
    // that get embedded into the Theme module.
    static QString fontsDir() { return QStringLiteral(DAEMON_APP_FONTS_DIR); }

    static QRawFont loadRaw(const QString& fileName)
    {
        QFile file(fontsDir() + QLatin1Char('/') + fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        QRawFont raw;
        raw.loadFromData(file.readAll(), 16.0, QFont::PreferDefaultHinting);
        return raw;
    }

private slots:
    void allBundledFontFilesExist()
    {
        const QStringList files = {
            QStringLiteral("InterVariable.ttf"),
            QStringLiteral("fa-solid-900.ttf"),
            QStringLiteral("fa-brands-400.ttf"),
            QStringLiteral("material-symbols-outlined.ttf"),
        };
        for (const QString& f : files) {
            QVERIFY2(QFile::exists(fontsDir() + QLatin1Char('/') + f),
                     qPrintable(QStringLiteral("missing bundled font: ") + f));
        }
    }

    void interLoadsAsInter()
    {
        const QRawFont inter = loadRaw(QStringLiteral("InterVariable.ttf"));
        QVERIFY2(inter.isValid(), "InterVariable.ttf failed to parse");
        QVERIFY2(inter.familyName().contains(QStringLiteral("Inter")),
                 qPrintable(QStringLiteral("unexpected Inter family: ") + inter.familyName()));
    }

    void faSolidLoadsAsFontAwesome()
    {
        const QRawFont fa = loadRaw(QStringLiteral("fa-solid-900.ttf"));
        QVERIFY2(fa.isValid(), "fa-solid-900.ttf failed to parse");
        QVERIFY2(fa.familyName().contains(QStringLiteral("Font Awesome")),
                 qPrintable(QStringLiteral("unexpected FA family: ") + fa.familyName()));
    }

    void faSolidCarriesEveryGlyphTheChromeUses_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<uint>("code");

        struct Glyph {
            const char* name;
            char32_t code;
        };
        // Every standalone icon the UI binds, by FontAwesome 6 Free codepoint.
        static const std::array<Glyph, 19> glyphs = { {
            { "gear", 0xf013 },
            { "magnifying_glass", 0xf002 },
            { "trash", 0xf1f8 },
            { "plus", 0xf067 },
            { "xmark", 0xf00d },
            { "folder", 0xf07b },
            { "tag", 0xf02b },
            { "comments", 0xf086 },
            { "box_archive", 0xf187 },
            { "paper_plane", 0xf1d8 },
            { "ellipsis_h", 0xf141 },
            { "heading", 0xf1dc },
            { "square_check", 0xf14a },
            { "chevron_down", 0xf078 },
            { "list_ul", 0xf0ca },
            { "link", 0xf0c1 },
            { "image", 0xf03e },
            { "circle", 0xf111 },
            { "angles_left", 0xf100 },
        } };
        for (const Glyph& g : glyphs) {
            QTest::newRow(g.name) << QString::fromUtf8(g.name) << static_cast<uint>(g.code);
        }
    }

    void faSolidCarriesEveryGlyphTheChromeUses()
    {
        QFETCH(QString, name);
        QFETCH(uint, code);

        const QRawFont fa = loadRaw(QStringLiteral("fa-solid-900.ttf"));
        QVERIFY(fa.isValid());
        QVERIFY2(fa.supportsCharacter(static_cast<char32_t>(code)),
                 qPrintable(QStringLiteral("FA solid is missing glyph %1 (U+%2)")
                                .arg(name)
                                .arg(code, 4, 16, QLatin1Char('0'))));
    }

    void materialCarriesKanban()
    {
        const QRawFont mt = loadRaw(QStringLiteral("material-symbols-outlined.ttf"));
        QVERIFY2(mt.isValid(), "material-symbols-outlined.ttf failed to parse");
        QVERIFY2(mt.supportsCharacter(static_cast<char32_t>(0xeb7f)),
                 "material symbols missing view_kanban (U+EB7F)");
    }
};

QTEST_MAIN(TestFonts)
#include "tst_fonts.moc"
