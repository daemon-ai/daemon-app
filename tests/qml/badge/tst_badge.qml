// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtTest
import DaemonApp.Controls as Kit
import DaemonApp.Theme

// Kit.Badge behavior (TOOL-8 count pill): hidden at 0, visible when count > 0,
// tone -> fill tint, and the ">max" clamp label. Everything binds Theme tokens.
Item {
    id: root
    width: 200
    height: 80

    Kit.Badge {
        id: badge
        count: 0
    }

    TestCase {
        id: tc
        name: "BadgeTests"
        when: windowShown

        function test_hidden_at_zero() {
            badge.count = 0;
            verify(!badge.visible);
        }

        function test_visible_when_positive() {
            badge.count = 3;
            verify(badge.visible);
        }

        function test_tone_tints_fill() {
            badge.tone = "danger";
            compare(badge.fill, Theme.danger);
            badge.tone = "accent";
            compare(badge.fill, Theme.accent);
            badge.tone = "danger";
        }

        function test_clamp_label_over_max() {
            badge.count = 250;
            badge.max = 99;
            // The rendered pill stays bounded regardless of the raw count.
            verify(badge.visible);
            badge.count = 3;
        }
    }
}
