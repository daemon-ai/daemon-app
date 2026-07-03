// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtTest
import DaemonApp.Pages

// Regression coverage for the REAL DownloadsPanel (the surface that rendered
// blank in production): rows present in ModelCatalog.downloads MUST render
// (visible + non-zero height), the panel self-hides when empty, activeOnly
// collapses terminal rows, the multi-file "file x/y" label, and the
// "Use this model" path resolving a catalog id via ModelCatalog.installedIdFor.
// Rows are pinned deterministically through MockModelCatalog.seedDownloadRows
// (which freezes the mock's ticking simulation); one test drives the real
// downloadFile/pauseDownload API instead.
TestCase {
    id: tc
    name: "DownloadsPanel"
    width: 640
    height: 480
    // TestCase defaults to visible:false, which makes every child EFFECTIVELY
    // invisible regardless of its own bindings; the panel's self-hide contract
    // (visible / implicitHeight) is exactly what this test asserts, so the
    // scene must be genuinely visible.
    visible: true
    when: windowShown

    // A full-shape downloads row (every key the panel binds); `extra` overrides.
    function mkRow(id, state, progress, extra) {
        var row = {
            "id": id,
            "modelId": "model-" + id,
            "name": "model-" + id + ".gguf",
            "repo": "org/model-" + id,
            "state": state,
            "progress": progress,
            "sizeLabel": "4.0 GiB",
            "downloadedLabel": "1.0 GiB",
            "filesDone": 0,
            "filesTotal": 1,
            "error": ""
        };
        if (extra !== undefined)
            for (var k in extra)
                row[k] = extra[k];
        return row;
    }

    // Full management surface (discover dialog / Models hub Downloads tab).
    DownloadsPanel {
        id: panel
        width: 400
    }

    // The wizard/picker strip: only live rows may render.
    DownloadsPanel {
        id: activePanel
        width: 400
        activeOnly: true
    }

    // Discover-dialog mode: done rows offer "Use this model".
    DownloadsPanel {
        id: usePanel
        width: 400
        showUseAction: true
    }

    SignalSpy {
        id: startedSpy
        target: ModelCatalog
        signalName: "downloadStarted"
    }

    SignalSpy {
        id: useSpy
        target: usePanel
        signalName: "useModelRequested"
    }

    function init() {
        ModelCatalog.seedDownloadRows([]);
        startedSpy.clear();
        useSpy.clear();
    }

    // (a) Rows in the facade render: the panel un-hides and takes height.
    function test_active_rows_render() {
        ModelCatalog.seedDownloadRows([
            mkRow("1", "downloading", 0.4, undefined),
            mkRow("2", "queued", 0.0, undefined)
        ]);
        tryCompare(ModelCatalog.downloads, "count", 2);
        tryVerify(function() { return panel.visible; }, 2000,
                  "panel visible with active rows");
        verify(panel.implicitHeight > 0, "panel takes height with active rows");
        // The active-only strip renders live rows too.
        tryVerify(function() { return activePanel.visible; }, 2000,
                  "active-only strip visible with live rows");
        verify(activePanel.implicitHeight > 0, "strip takes height with live rows");
    }

    // (b) No rows: the panel hides itself (no reserved height in embedders).
    function test_empty_hides() {
        compare(ModelCatalog.downloads.count, 0, "seeded empty");
        tryVerify(function() { return !panel.visible; }, 2000,
                  "panel hidden with no rows");
        // Positioner collapse lands on the next polish pass.
        tryCompare(panel, "implicitHeight", 0);
    }

    // (c) activeOnly: terminal rows (done/failed/cancelled) never render; the
    // panel collapses when ONLY terminal rows exist — while the full panel
    // still shows them (dismiss/retry/use affordances live there).
    function test_active_only_hides_terminal_rows() {
        ModelCatalog.seedDownloadRows([
            mkRow("1", "done", 1.0, undefined),
            mkRow("2", "failed", 0.3, { "error": "boom" }),
            mkRow("3", "cancelled", 0.1, undefined)
        ]);
        tryCompare(ModelCatalog.downloads, "count", 3);
        tryVerify(function() { return panel.visible; }, 2000,
                  "full panel renders terminal rows");
        tryVerify(function() { return !activePanel.visible; }, 2000,
                  "active-only strip hides when only terminal rows exist");
        tryCompare(activePanel, "implicitHeight", 0);
    }

    // Multi-file jobs surface "file x/y" on the active progress row; a
    // single-file job keeps the label hidden.
    function test_files_progress_label() {
        ModelCatalog.seedDownloadRows([
            mkRow("1", "downloading", 0.4, { "filesDone": 2, "filesTotal": 5 })
        ]);
        tryVerify(function() { return panel.visible; }, 2000, "panel visible");
        var label = findChild(panel, "filesProgressLabel");
        verify(label !== null, "files label instantiated");
        tryCompare(label, "visible", true);
        compare(label.text, "file 2/5", "x/y from filesDone/filesTotal");

        ModelCatalog.seedDownloadRows([
            mkRow("2", "downloading", 0.4, undefined)
        ]);
        tryVerify(function() {
            var l = findChild(panel, "filesProgressLabel");
            return l !== null && !l.visible;
        }, 2000, "single-file row hides the label");
    }

    // (d) "Use this model" on a done row resolves the installed CATALOG id via
    // ModelCatalog.installedIdFor(repo, name) and emits useModelRequested.
    function test_use_action_resolves_installed_id() {
        // The mock seeds llama-3.1-8b-instruct as installed; its "repo" IS the id.
        compare(ModelCatalog.installedIdFor("llama-3.1-8b-instruct", "any.gguf"),
                "llama-3.1-8b-instruct", "installedIdFor resolves an installed repo");
        compare(ModelCatalog.installedIdFor("org/never-installed", "x.gguf"), "",
                "installedIdFor misses cleanly");

        ModelCatalog.seedDownloadRows([
            mkRow("1", "done", 1.0, {
                "repo": "llama-3.1-8b-instruct",
                "name": "llama-3.1-8b-instruct"
            })
        ]);
        tryVerify(function() { return usePanel.visible; }, 2000,
                  "showUseAction panel renders the done row");
        var btn = findChild(usePanel, "useModelButton");
        verify(btn !== null, "Use this model button instantiated");
        tryCompare(btn, "visible", true);
        btn.clicked(); // drives the real onClicked resolution path
        compare(useSpy.count, 1, "useModelRequested emitted");
        compare(useSpy.signalArguments[0][0], "llama-3.1-8b-instruct",
                "resolved to the installed catalog id");
    }

    // The mock's REAL download API also renders: downloadFile enqueues a job
    // row (paused immediately for determinism) that the panel shows.
    function test_real_download_api_renders() {
        ModelCatalog.downloadFile("mistral-7b-instruct",
                                  "Mistral-7B-Instruct-Q4_K_M.gguf", "llama");
        compare(startedSpy.count, 1, "downloadStarted fired");
        var jobId = startedSpy.signalArguments[0][0];
        ModelCatalog.pauseDownload(jobId); // freeze before the first tick
        tryCompare(ModelCatalog.downloads, "count", 1);
        var row = ModelCatalog.downloads.at(0);
        compare(row.state, "paused", "job pinned in a live state");
        compare(row.filesTotal, 1, "mock rows carry the multi-file keys");
        tryVerify(function() { return panel.visible; }, 2000,
                  "panel renders the real-API job row");
        tryVerify(function() { return activePanel.visible; }, 2000,
                  "paused row counts as live in the active-only strip");
    }
}
