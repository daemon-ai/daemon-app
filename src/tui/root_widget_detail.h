// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>

class TabModel;
namespace persistence {
class ISessionStore;
}

// Free helpers shared across the RootWidget translation units (split out of the
// former root_widget.cpp anonymous namespace so each TU can call them). Kept in a
// named namespace rather than a per-TU anonymous one so there is a single
// definition (root_widget_detail.cpp) instead of one copy per including TU.
namespace rwdetail {

// Best-effort desktop notification from the TUI: notify-send when present
// (reliable on Linux desktops), plus an OSC 9 escape as a terminal-native
// fallback. The BEL urgency hint is rung separately by the caller.
void emitDesktopNotification(const QString& title, const QString& body);

// A short tab title for a session: the first non-empty content line (heading
// markers stripped, capped), falling back to a generic label.
QString titleForContent(const QString& markdown);

// The canonical tab title for `sessionId`: the store's title when set, else the
// content-derived fallback above. The single resolution rule shared by the
// open paths and the store-change refresh below.
QString resolveSessionTabTitle(const persistence::ISessionStore* store, const QString& sessionId);

// Re-resolve the title of every open TRANSCRIPT tab from the store (page/file
// tabs keep their labels). Connected to ISessionStore::changed(): the node
// auto-titles a session from its first user message and the roster refetch
// lands that title in the cache WITHOUT a content change, so an open tab would
// otherwise keep its stale "New session" fallback forever (GUI parity: the
// SessionStore.changed() -> _resolveTitle() Connections in TranscriptPage.qml).
void refreshTranscriptTabTitles(TabModel* tabs, const persistence::ISessionStore* store);

// The static markdown shown for a (non-transcript) page tab.
QString pageMarkdown(int kind);

// [integrations wire v38] Activation dispatch (A4): open (or focus) a native Chat
// tab for a room/DM `(transport, conversation)`, returning its stable tab id. The
// single shared rule both the sidebar's conversationActivated wiring (GUI Main.qml
// + this TUI RootWidget) route through, replacing today's open-Channels fallback.
// `label` is the tab title; empty falls back to the conversation id.
int openConversationTab(TabModel* tabs, const QString& transport, const QString& conversation,
                        const QString& label = {});

// The attachment kind for a picked workspace file: "image" for image
// extensions, else "file" - the same heuristic the GUI composer's drag-drop
// applies (Composer.qml DropArea). Folders are not pickable through the
// finder-backed picker, so "folder" never originates here.
QString attachmentKindForName(const QString& name);

} // namespace rwdetail
