// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtTest
import DaemonApp.Controls as Kit
import DaemonApp.Theme

// Kit.Chip behavior: tone -> text/border tint (no dedicated bg token), icon and
// close affordance visibility, clicked()/closeRequested() emission, and the
// non-interactive default (no clicked signal on tap).
Item {
    id: root
    width: 300
    height: 120

    Kit.Chip {
        id: plainChip
        text: "plain"
    }

    Kit.Chip {
        id: fullChip
        y: 30
        text: "engine"
        iconGlyph: FontIcons.fa_robot
        tone: "accent"
        interactive: true
        closable: true
    }

    SignalSpy {
        id: clickSpy
        target: fullChip
        signalName: "clicked"
    }

    SignalSpy {
        id: closeSpy
        target: fullChip
        signalName: "closeRequested"
    }

    SignalSpy {
        id: plainClickSpy
        target: plainChip
        signalName: "clicked"
    }

    TestCase {
        id: tc
        name: "ChipTests"
        when: windowShown

        function test_tones_tint_text_and_border() {
            plainChip.tone = "neutral";
            compare(plainChip.toneColor, Theme.text);
            compare(plainChip.border.color, Theme.border);

            plainChip.tone = "muted";
            compare(plainChip.toneColor, Theme.textMuted);
            compare(plainChip.border.color, Theme.border);

            plainChip.tone = "accent";
            compare(plainChip.toneColor, Theme.accent);
            verify(!Qt.colorEqual(plainChip.border.color, Theme.border));

            plainChip.tone = "danger";
            compare(plainChip.toneColor, Theme.danger);
            verify(!Qt.colorEqual(plainChip.border.color, Theme.border));

            plainChip.tone = "neutral";
        }

        function test_interactive_click_emits() {
            clickSpy.clear();
            mouseClick(fullChip, 8, fullChip.height / 2);
            compare(clickSpy.count, 1);
        }

        function test_close_emits_not_clicked() {
            clickSpy.clear();
            closeSpy.clear();
            // The xmark is the trailing glyph; click near the right edge.
            mouseClick(fullChip, fullChip.width - 10, fullChip.height / 2);
            compare(closeSpy.count, 1);
            compare(clickSpy.count, 0);
        }

        function test_non_interactive_ignores_clicks() {
            plainClickSpy.clear();
            mouseClick(plainChip, plainChip.width / 2, plainChip.height / 2);
            compare(plainClickSpy.count, 0);
        }

        function test_chip_grows_with_content() {
            const bare = plainChip.implicitWidth;
            plainChip.iconGlyph = FontIcons.fa_robot;
            // The RowLayout re-polishes async; wait for the implicit width to grow.
            tryVerify(function() { return plainChip.implicitWidth > bare; }, 1000);
            plainChip.iconGlyph = "";
        }
    }
}
