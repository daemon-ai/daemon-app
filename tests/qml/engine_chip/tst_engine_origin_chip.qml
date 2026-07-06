// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtTest
import DaemonApp.Controls as Kit

// [wave2:app-engines] EngineOriginChip (C5): the no-noise contract. With no EngineIdentity facade
// in context (as in this harness) it must render nothing regardless of sessionId — the approvals
// card can drop it in unconditionally without native rows sprouting a chip. Also guards that the
// component imports + loads cleanly (a QML error here fails the test at load).
Item {
    id: root
    width: 200
    height: 80

    Kit.EngineOriginChip {
        id: chip
        sessionId: "s-native-or-unknown"
    }

    TestCase {
        id: tc
        name: "EngineOriginChipTests"
        when: windowShown

        function test_no_facade_renders_nothing() {
            // info resolves to { engine: "", label: "" } without EngineIdentity -> invisible.
            verify(!chip.visible);
            compare(chip.info.engine, "");
        }

        function test_empty_session_stays_invisible() {
            chip.sessionId = "";
            verify(!chip.visible);
        }
    }
}
