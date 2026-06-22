// SPDX-License-Identifier: BSL-1.0

// Vendored verbatim from Tui Widgets at the flake-pinned revision
// 31ccc7af4ff47ec209e8e7b3508988fec5d2e2e9 (src/Tui/ListNode_p.h). Tui Widgets
// does not install its private headers; the TUI needs ZListViewPrivate's layout
// (via ZWidget_p.h) to read the otherwise-inaccessible scroll position so mouse
// clicks map to the correct row when the sidebar tree is scrolled. Keep this in
// lockstep with the tuiwidgets rev pinned in flake.nix.

#ifndef TUIWIDGETS_LISTNODE_P_INCLUDED
#define TUIWIDGETS_LISTNODE_P_INCLUDED

#include <QtGlobal>
#include <Tui/tuiwidgets_internal.h>

TUIWIDGETS_NS_START

template <typename Tag>
struct ListTrait;

template <typename T>
class ListNode;

template <typename T, typename Tag>
class ListHead {
public:
    ListHead(){}
    ListHead(const ListHead&) = delete;
    ListHead &operator=(const ListHead&) = delete;

    ~ListHead() {
        clear();
    }

    void clear() {
        while (first) {
            remove(first);
        }
    }

    void appendOrMoveToLast(T *e) {
        constexpr auto nodeOffset = ListTrait<Tag>::offset;
        auto& node = e->*nodeOffset;
        if (last == e) {
            return;
        }
        if (node.next) {
            //move
            remove(e);
        }
        if (last) {
            (last->*nodeOffset).next = e;
            node.prev = last;
            last = e;
        } else {
            first = e;
            last = e;
        }
    }

    void remove(T *e) {
        constexpr auto nodeOffset = ListTrait<Tag>::offset;
        auto &node = e->*nodeOffset;
        if (e == first) {
            first = node.next;
        }
        if (e == last) {
            last = node.prev;
        }
        if (node.prev) {
            (node.prev->*nodeOffset).next = node.next;
        }
        if (node.next) {
            (node.next->*nodeOffset).prev = node.prev;
        }
        node.prev = nullptr;
        node.next = nullptr;
    }

    T *first = nullptr;
    T *last = nullptr;
};


template <typename T>
class ListNode {
public:
    ListNode() = default;
    ListNode(const ListNode&) = delete;
    ListNode &operator=(const ListNode&) = delete;

    T *prev = nullptr;
    T *next = nullptr;
};

TUIWIDGETS_NS_END

#endif // TUIWIDGETS_LISTNODE_P_INCLUDED
