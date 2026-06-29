// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "diagram/layout/layered_layout.h"

#include "diagram/geometry/shapes.h"
#include "diagram/layout/work_graph.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <QSet>
#include <util/numeric.h>

namespace be::diagram {

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();

bool isVertical(Direction dir) {
    return dir == Direction::TB || dir == Direction::BT;
}

bool isDummy(const WorkNode& n) {
    return n.kind != WorkKind::Real;
}

// Resolve the top-level cluster ancestor of a cluster id (walking parentId).
QString topCluster(const DiagramModel& model, const QString& cid) {
    if (cid.isEmpty()) {
        return cid;
    }
    QString cur = cid;
    for (int guard = 0; guard < model.clusters.size() + 1; ++guard) {
        int idx = -1;
        for (int i = 0; i < model.clusters.size(); ++i) {
            if (model.clusters[i].id == cur) {
                idx = i;
                break;
            }
        }
        if (idx < 0 || model.clusters[idx].parentId.isEmpty()) {
            return cur;
        }
        cur = model.clusters[idx].parentId;
    }
    return cur;
}

// ---------------------------------------------------------------------------
// Build the work graph from the model: one Real node per DiagramNode, one edge
// per DiagramEdge (self-loops skipped). Cluster tagging uses the top ancestor so
// ordering keeps whole subgraphs contiguous.
// ---------------------------------------------------------------------------
WorkGraph buildWorkGraph(const DiagramModel& model) {
    WorkGraph g;
    g.nodes.reserve(model.nodes.size());
    for (int i = 0; i < model.nodes.size(); ++i) {
        const DiagramNode& dn = model.nodes[i];
        WorkNode wn;
        wn.id = dn.id;
        wn.kind = WorkKind::Real;
        wn.w = dn.width;
        wn.h = dn.height;
        wn.cluster = topCluster(model, dn.parentCluster);
        wn.srcNode = i;
        g.add(wn);
    }
    for (int i = 0; i < model.edges.size(); ++i) {
        const DiagramEdge& e = model.edges[i];
        const int u = g.index.value(e.fromId, -1);
        const int v = g.index.value(e.toId, -1);
        if (u < 0 || v < 0 || u == v) {
            continue;
        }
        g.addEdge(u, v, 1.0, 1, i);
    }
    return g;
}

// ---------------------------------------------------------------------------
// Acyclic: greedy DFS feedback-arc removal. Back edges (to a node on the current
// recursion stack) are reversed in place.
// ---------------------------------------------------------------------------
void acyclic(WorkGraph& g) {
    const int n = daemon_app::to_int(g.nodes.size());
    QVector<int> state(n, 0); // 0=unvisited, 1=on-stack, 2=done
    const QVector<QVector<int>> out = g.outEdges();

    std::function<void(int)> dfs = [&](int u) {
        state[u] = 1;
        for (int ei : out[u]) {
            WorkEdge& e = g.edges[ei];
            if (e.u != u) {
                continue; // already reversed away from u
            }
            const int v = e.v;
            if (state[v] == 1) {
                std::swap(e.u, e.v);
                e.reversed = !e.reversed;
            } else if (state[v] == 0) {
                dfs(v);
            }
        }
        state[u] = 2;
    };
    for (int i = 0; i < n; ++i) {
        if (state[i] == 0) {
            dfs(i);
        }
    }
}

// ---------------------------------------------------------------------------
// Rank: longest-path init then a light "tighten" that pulls slack sources down
// toward their earliest child. Ranks are then doubled so every edge spans an
// even number of half-ranks, leaving odd ranks free for edge-label dummies.
// ---------------------------------------------------------------------------
void rank(WorkGraph& g) {
    const int n = daemon_app::to_int(g.nodes.size());
    for (WorkNode& nd : g.nodes) {
        nd.rank = 0;
    }
    for (int pass = 0; pass <= n; ++pass) {
        bool changed = false;
        for (const WorkEdge& e : g.edges) {
            if (e.u < 0 || e.v < 0) {
                continue;
            }
            const int want = g.nodes[e.u].rank + e.minlen;
            if (g.nodes[e.v].rank < want) {
                g.nodes[e.v].rank = want;
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }

    // Tighten: a source with slack can slide forward to sit just above its
    // earliest successor, shortening over-long spans.
    const QVector<QVector<int>> out = g.outEdges();
    const QVector<QVector<int>> in = g.inEdges();
    for (int guard = 0; guard <= n; ++guard) {
        bool changed = false;
        for (int v = 0; v < n; ++v) {
            if (!in[v].isEmpty() || out[v].isEmpty()) {
                continue;
            }
            int m = std::numeric_limits<int>::max();
            for (int ei : out[v]) {
                m = std::min(m, g.nodes[g.edges[ei].v].rank - g.edges[ei].minlen);
            }
            if (m != std::numeric_limits<int>::max() && m > g.nodes[v].rank) {
                g.nodes[v].rank = m;
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }

    int minRank = std::numeric_limits<int>::max();
    for (const WorkNode& nd : g.nodes) {
        minRank = std::min(minRank, nd.rank);
    }
    if (minRank == std::numeric_limits<int>::max()) {
        minRank = 0;
    }
    for (WorkNode& nd : g.nodes) {
        nd.rank = (nd.rank - minRank) * 2;
    }
}

// ---------------------------------------------------------------------------
// Add left/right border walls spanning each top-level cluster's rank range, with
// nesting edges to keep each wall vertically aligned. The walls reserve a
// rectangular column so foreign nodes cannot intrude into the cluster region;
// they are layout-only and never written back to the model.
// ---------------------------------------------------------------------------
void addBorderSegments(WorkGraph& g, const DiagramModel& model, const LayoutOptions& opts) {
    const int realCount = daemon_app::to_int(g.nodes.size());
    for (const DiagramCluster& c : model.clusters) {
        if (!c.parentId.isEmpty()) {
            continue; // top-level clusters only; nested rely on bounds
        }
        int rmin = std::numeric_limits<int>::max();
        int rmax = std::numeric_limits<int>::min();
        for (int i = 0; i < realCount; ++i) {
            const WorkNode& wn = g.nodes[i];
            if (wn.kind == WorkKind::Real && wn.cluster == c.id) {
                rmin = std::min(rmin, wn.rank);
                rmax = std::max(rmax, wn.rank);
            }
        }
        if (rmin > rmax) {
            continue;
        }
        int prevL = -1;
        int prevR = -1;
        for (int r = rmin; r <= rmax; ++r) {
            WorkNode l;
            l.kind = WorkKind::BorderLeft;
            l.rank = r;
            l.cluster = c.id;
            l.w = 1.0;
            l.h = 1.0;
            WorkNode rr;
            rr.kind = WorkKind::BorderRight;
            rr.rank = r;
            rr.cluster = c.id;
            rr.w = 1.0;
            rr.h = 1.0;
            const int li = g.add(l);
            const int ri = g.add(rr);
            if (prevL >= 0) {
                g.addEdge(prevL, li, 8.0, 1, -1);
            }
            if (prevR >= 0) {
                g.addEdge(prevR, ri, 8.0, 1, -1);
            }
            prevL = li;
            prevR = ri;
        }
    }
}

// ---------------------------------------------------------------------------
// Normalize: replace every (now even-rank) edge with a chain of dummy nodes,
// one per intermediate rank. The mid rank carries an edge-label dummy sized to
// the measured label. chains[edgeIdx] keeps the ordered dummy indices (low rank
// to high rank) so undo() can collect bend points.
// ---------------------------------------------------------------------------
struct Chains {
    QHash<int, QVector<int>> dummies; // DiagramEdge index -> dummy node indices
    QHash<int, int> labelDummy;       // DiagramEdge index -> label dummy node index
};

Chains normalize(WorkGraph& g, const DiagramModel& model, const LayoutOptions& opts) {
    Chains chains;
    const int edgeCount = daemon_app::to_int(g.edges.size());
    for (int ei = 0; ei < edgeCount; ++ei) {
        WorkEdge edge = g.edges[ei];
        if (edge.u < 0 || edge.v < 0 || edge.edgeIdx < 0) {
            continue;
        }
        int r0 = g.nodes[edge.u].rank;
        int r1 = g.nodes[edge.v].rank;
        if (r1 <= r0) {
            continue;
        }
        const DiagramEdge& de = model.edges[edge.edgeIdx];
        const bool hasLabel = de.labelWidth > 0.0 && de.labelHeight > 0.0;
        const int midRank = r0 + ((r1 - r0) / 2);
        const QString cluster = topCluster(model, model.nodes[edge.v].parentCluster) ==
                                        topCluster(model, model.nodes[edge.u].parentCluster)
                                    ? g.nodes[edge.u].cluster
                                    : QString();

        if (r1 - r0 == 1) {
            continue; // already a unit segment, no intermediate rank
        }

        // Deactivate the long edge; we rebuild it as a dummy chain.
        g.edges[ei].u = -1;
        g.edges[ei].v = -1;

        QVector<int> chain;
        int prev = edge.u;
        for (int r = r0 + 1; r < r1; ++r) {
            WorkNode d;
            d.kind = WorkKind::EdgeDummy;
            d.rank = r;
            d.cluster = cluster;
            d.edgeIdx = edge.edgeIdx;
            d.w = 1.0;
            d.h = 1.0;
            if (hasLabel && r == midRank) {
                d.kind = WorkKind::EdgeLabelDummy;
                d.w = de.labelWidth;
                d.h = de.labelHeight;
                chains.labelDummy.insert(edge.edgeIdx, -1); // filled below with index
            }
            const int di = g.add(d);
            if (d.kind == WorkKind::EdgeLabelDummy) {
                chains.labelDummy[edge.edgeIdx] = di;
            }
            g.addEdge(prev, di, edge.weight, 1, edge.edgeIdx);
            chain.push_back(di);
            prev = di;
        }
        g.addEdge(prev, edge.v, edge.weight, 1, edge.edgeIdx);
        chains.dummies.insert(edge.edgeIdx, chain);
    }
    return chains;
}

// Build [rank][order] layering from current node.order values.
QVector<QVector<int>> buildLayering(const WorkGraph& g, int& numRanks) {
    numRanks = 0;
    for (const WorkNode& n : g.nodes) {
        numRanks = std::max(numRanks, n.rank + 1);
    }
    QVector<QVector<int>> layers(numRanks);
    for (int i = 0; i < g.nodes.size(); ++i) {
        layers[g.nodes[i].rank].push_back(i);
    }
    for (QVector<int>& layer : layers) {
        std::ranges::stable_sort(layer,
                                 [&](int a, int b) { return g.nodes[a].order < g.nodes[b].order; });
    }
    return layers;
}

// Adjacency between adjacent ranks (only active unit edges).
struct Adj {
    QVector<QVector<int>> preds; // by node -> predecessor node indices (rank-1)
    QVector<QVector<int>> succs; // by node -> successor node indices (rank+1)
};

Adj buildAdj(const WorkGraph& g) {
    Adj a;
    a.preds.resize(g.nodes.size());
    a.succs.resize(g.nodes.size());
    for (const WorkEdge& e : g.edges) {
        if (e.u < 0 || e.v < 0) {
            continue;
        }
        a.succs[e.u].push_back(e.v);
        a.preds[e.v].push_back(e.u);
    }
    return a;
}

// ---------------------------------------------------------------------------
// Ordering: DFS init then barycenter sweeps, keeping the layering with the
// fewest crossings. Cluster contiguity keeps subgraph members adjacent.
// ---------------------------------------------------------------------------
int crossCount(const WorkGraph& g, const QVector<QVector<int>>& layers) {
    QVector<int> pos(g.nodes.size(), 0);
    for (const QVector<int>& layer : layers) {
        for (int i = 0; i < layer.size(); ++i) {
            pos[layer[i]] = i;
        }
    }
    int total = 0;
    for (int r = 0; r + 1 < layers.size(); ++r) {
        QVector<QPair<int, int>> segs; // (pos in r, pos in r+1)
        for (int u : layers[r]) {
            for (const WorkEdge& e : g.edges) {
                if (e.u == u && e.v >= 0 && g.nodes[e.v].rank == r + 1) {
                    segs.push_back({pos[u], pos[e.v]});
                }
            }
        }
        std::ranges::sort(segs);
        for (int i = 0; i < segs.size(); ++i) {
            for (int j = i + 1; j < segs.size(); ++j) {
                if (segs[j].second < segs[i].second) {
                    ++total;
                }
            }
        }
    }
    return total;
}

void initOrder(WorkGraph& g, const Adj& adj, int numRanks) {
    QVector<int> counter(numRanks, 0);
    QVector<bool> visited(g.nodes.size(), false);
    std::function<void(int)> dfs = [&](int v) {
        if (visited[v]) {
            return;
        }
        visited[v] = true;
        g.nodes[v].order = counter[g.nodes[v].rank]++;
        for (int w : adj.succs[v]) {
            dfs(w);
        }
    };
    // Roots first (no predecessors), in node order, then any stragglers.
    for (int v = 0; v < g.nodes.size(); ++v) {
        if (adj.preds[v].isEmpty()) {
            dfs(v);
        }
    }
    for (int v = 0; v < g.nodes.size(); ++v) {
        dfs(v);
    }
}

void sortRank(WorkGraph& g, QVector<int>& layer, const Adj& adj, bool useSucc) {
    const int m = daemon_app::to_int(layer.size());
    QHash<int, double> bc;
    for (int i = 0; i < m; ++i) {
        const int v = layer[i];
        const QVector<int>& nbrs = useSucc ? adj.succs[v] : adj.preds[v];
        double sum = 0.0;
        int cnt = 0;
        for (int nb : nbrs) {
            sum += g.nodes[nb].order;
            ++cnt;
        }
        bc.insert(v, cnt == 0 ? double(i) : sum / cnt);
    }
    // Per-cluster column proxy: the mean within-rank order of a cluster's real
    // members across the whole graph. Defined even on border-only ranks, where a
    // per-rank average would be meaningless.
    QHash<QString, double> colSum;
    QHash<QString, int> colCnt;
    for (const WorkNode& n : g.nodes) {
        if (n.kind == WorkKind::Real && !n.cluster.isEmpty()) {
            colSum[n.cluster] += n.order;
            colCnt[n.cluster] += 1;
        }
    }
    const auto groupKey = [&](int v) -> double {
        const QString& c = g.nodes[v].cluster;
        if (c.isEmpty() || !colCnt.contains(c)) {
            return bc[v];
        }
        return colSum[c] / colCnt[c];
    };
    // Within a cluster: left wall, then members (by barycenter), then right wall.
    const auto withinTier = [&](int v) -> int {
        if (g.nodes[v].kind == WorkKind::BorderLeft) {
            return -1;
        }
        if (g.nodes[v].kind == WorkKind::BorderRight) {
            return 1;
        }
        return 0;
    };
    std::ranges::stable_sort(layer, [&](int a, int b) {
        if (g.nodes[a].cluster == g.nodes[b].cluster && !g.nodes[a].cluster.isEmpty()) {
            const int ta = withinTier(a);
            const int tb = withinTier(b);
            if (ta != tb) {
                return ta < tb;
            }
            return bc[a] < bc[b];
        }
        return groupKey(a) < groupKey(b);
    });
}

void order(WorkGraph& g, const Adj& adj, int numRanks) {
    initOrder(g, adj, numRanks);
    QVector<QVector<int>> layers = buildLayering(g, numRanks);

    const auto applyOrders = [&](const QVector<QVector<int>>& ls) {
        for (const QVector<int>& layer : ls) {
            for (int i = 0; i < layer.size(); ++i) {
                g.nodes[layer[i]].order = i;
            }
        }
    };

    QVector<QVector<int>> best = layers;
    int bestCross = crossCount(g, layers);

    for (int iter = 0; iter < 8 && bestCross > 0; ++iter) {
        const bool down = (iter % 2) == 0;
        if (down) {
            for (int r = 1; r < layers.size(); ++r) {
                sortRank(g, layers[r], adj, /*useSucc=*/false);
                applyOrders(layers);
            }
        } else {
            for (int r = daemon_app::to_int(layers.size() - 2); r >= 0; --r) {
                sortRank(g, layers[r], adj, /*useSucc=*/true);
                applyOrders(layers);
            }
        }
        const int cc = crossCount(g, layers);
        if (cc < bestCross) {
            bestCross = cc;
            best = layers;
        }
    }
    applyOrders(best);
}

// ---------------------------------------------------------------------------
// Brandes-Koepf X assignment. Operates in the canonical frame (ranks stacked,
// cross axis is the value computed here). Half-size along the cross axis is the
// node width for vertical layouts and the node height for horizontal layouts.
// ---------------------------------------------------------------------------
struct BK {
    const WorkGraph& g;
    const Adj& adj;
    const LayoutOptions& opts;
    bool vertical;
    QVector<QVector<int>> layers;
    QVector<int> pos; // order within rank (canonical)

    [[nodiscard]] double halfCross(int v) const {
        return 0.5 * (vertical ? g.nodes[v].w : g.nodes[v].h);
    }

    static bool isBorder(const WorkNode& n) {
        return n.kind == WorkKind::BorderLeft || n.kind == WorkKind::BorderRight;
    }

    // Minimum center-to-center spacing between two nodes adjacent in a rank.
    [[nodiscard]] double sep(int a, int b) const {
        double gap = opts.edgeSep;
        if (isBorder(g.nodes[a]) || isBorder(g.nodes[b])) {
            gap = opts.clusterPad; // a wall hugs its own members
        } else if (!isDummy(g.nodes[a]) && !isDummy(g.nodes[b])) {
            gap = opts.nodeSep;
        }
        if (g.nodes[a].cluster != g.nodes[b].cluster) {
            gap += 2.0 * opts.clusterPad; // keep cluster boxes off their neighbors
        }
        return halfCross(a) + halfCross(b) + gap;
    }

    [[nodiscard]] bool conflict(const QSet<qint64>& cf, int a, int b) const {
        const qint64 lo = std::min(a, b);
        const qint64 hi = std::max(a, b);
        return cf.contains((lo * qint64(g.nodes.size())) + hi);
    }

    [[nodiscard]] QSet<qint64> type1Conflicts() const {
        QSet<qint64> cf;
        const auto add = [&](int a, int b) {
            const qint64 lo = std::min(a, b);
            const qint64 hi = std::max(a, b);
            cf.insert((lo * qint64(g.nodes.size())) + hi);
        };
        for (int r = 1; r < layers.size(); ++r) {
            const QVector<int>& layer = layers[r];
            int k0 = 0;
            int scanPos = 0;
            const int last = daemon_app::to_int(layer.size() - 1);
            for (int l1 = 0; l1 < layer.size(); ++l1) {
                const int v = layer[l1];
                int w = -1; // inner-segment predecessor (dummy->dummy)
                if (isDummy(g.nodes[v])) {
                    for (int u : adj.preds[v]) {
                        if (isDummy(g.nodes[u])) {
                            w = u;
                            break;
                        }
                    }
                }
                if (w != -1 || l1 == last) {
                    const int k1 =
                        (w != -1) ? pos[w] : daemon_app::to_int(layers[r - 1].size() - 1);
                    for (int l = scanPos; l <= l1; ++l) {
                        const int u2 = layer[l];
                        for (int uu : adj.preds[u2]) {
                            const int p = pos[uu];
                            if ((p < k0 || p > k1) &&
                                !(isDummy(g.nodes[uu]) && isDummy(g.nodes[u2]))) {
                                add(uu, u2);
                            }
                        }
                    }
                    scanPos = l1 + 1;
                    k0 = k1;
                }
            }
        }
        return cf;
    }

    // One alignment pass. `vertReverse` walks ranks bottom-up using successors;
    // `horizReverse` reverses each layer so compaction packs from the right.
    void run(const QSet<qint64>& cf, bool vertReverse, bool horizReverse,
             QHash<int, double>& outX) {
        QVector<QVector<int>> ls = layers;
        if (vertReverse) {
            std::ranges::reverse(ls);
        }
        if (horizReverse) {
            for (QVector<int>& l : ls) {
                std::ranges::reverse(l);
            }
        }
        QVector<int> lpos(g.nodes.size(), 0);
        QVector<int> leftOf(g.nodes.size(), -1);
        for (const QVector<int>& l : ls) {
            for (int i = 0; i < l.size(); ++i) {
                lpos[l[i]] = i;
                if (i > 0) {
                    leftOf[l[i]] = l[i - 1];
                }
            }
        }

        // Vertical alignment.
        QVector<int> root(g.nodes.size());
        QVector<int> align(g.nodes.size());
        for (int i = 0; i < g.nodes.size(); ++i) {
            root[i] = i;
            align[i] = i;
        }
        for (const QVector<int>& layer : ls) {
            int prevIdx = -1;
            for (int v : layer) {
                QVector<int> ws = vertReverse ? adj.succs[v] : adj.preds[v];
                if (ws.isEmpty()) {
                    continue;
                }
                std::ranges::sort(ws, [&](int a, int b) { return lpos[a] < lpos[b]; });
                const double mp = static_cast<double>(ws.size() - 1) / 2.0;
                const int from = int(std::floor(mp));
                const int to = int(std::ceil(mp));
                for (int m = from; m <= to; ++m) {
                    if (align[v] != v) {
                        break;
                    }
                    const int w = ws[m];
                    if (!conflict(cf, w, v) && prevIdx < lpos[w]) {
                        align[w] = v;
                        root[v] = root[w];
                        align[v] = root[w];
                        prevIdx = lpos[w];
                    }
                }
            }
        }

        // Horizontal compaction (classic BK with sink/shift).
        QVector<int> sink(g.nodes.size());
        QVector<double> shift(g.nodes.size(), kInf);
        QVector<double> x(g.nodes.size(), kInf);
        for (int i = 0; i < g.nodes.size(); ++i) {
            sink[i] = i;
        }

        std::function<void(int)> placeBlock = [&](int v) {
            if (x[v] != kInf) {
                return;
            }
            x[v] = 0.0;
            int w = v;
            do {
                const int u = leftOf[w];
                if (u >= 0) {
                    const int ur = root[u];
                    placeBlock(ur);
                    if (sink[v] == v) {
                        sink[v] = sink[ur];
                    }
                    const double s = sep(u, w);
                    if (sink[v] != sink[ur]) {
                        shift[sink[ur]] = std::min(shift[sink[ur]], x[v] - x[ur] - s);
                    } else {
                        x[v] = std::max(x[v], x[ur] + s);
                    }
                }
                w = align[w];
            } while (w != v);
        };

        for (int v = 0; v < g.nodes.size(); ++v) {
            if (root[v] == v) {
                placeBlock(v);
            }
        }
        // Absolute coordinates.
        for (int v = 0; v < g.nodes.size(); ++v) {
            x[v] = x[root[v]];
        }
        for (int v = 0; v < g.nodes.size(); ++v) {
            const double sh = shift[sink[root[v]]];
            if (sh != kInf) {
                x[v] += sh;
            }
        }
        outX.clear();
        for (int v = 0; v < g.nodes.size(); ++v) {
            outX.insert(v, horizReverse ? -x[v] : x[v]);
        }
    }

    QHash<int, double> assign() {
        const QSet<qint64> cf = type1Conflicts();
        QVector<QHash<int, double>> xss(4);
        int k = 0;
        for (int vr = 0; vr < 2; ++vr) {
            for (int hr = 0; hr < 2; ++hr) {
                run(cf, vr == 1, hr == 1, xss[k++]);
            }
        }

        // Pick the alignment of smallest width.
        int smallest = 0;
        double bestWidth = kInf;
        for (int i = 0; i < 4; ++i) {
            double lo = kInf;
            double hi = -kInf;
            for (auto it = xss[i].constBegin(); it != xss[i].constEnd(); ++it) {
                lo = std::min(lo, it.value());
                hi = std::max(hi, it.value());
            }
            if (hi - lo < bestWidth) {
                bestWidth = hi - lo;
                smallest = i;
            }
        }
        double alignLo = kInf;
        double alignHi = -kInf;
        for (auto it = xss[smallest].constBegin(); it != xss[smallest].constEnd(); ++it) {
            alignLo = std::min(alignLo, it.value());
            alignHi = std::max(alignHi, it.value());
        }
        for (int i = 0; i < 4; ++i) {
            if (i == smallest) {
                continue;
            }
            double lo = kInf;
            double hi = -kInf;
            for (auto it = xss[i].constBegin(); it != xss[i].constEnd(); ++it) {
                lo = std::min(lo, it.value());
                hi = std::max(hi, it.value());
            }
            // Even indices are left alignments, odd are right.
            const double delta = (i % 2 == 0) ? (alignLo - lo) : (alignHi - hi);
            for (auto it = xss[i].begin(); it != xss[i].end(); ++it) {
                it.value() += delta;
            }
        }

        QHash<int, double> result;
        for (int v = 0; v < g.nodes.size(); ++v) {
            QVector<double> vals{xss[0][v], xss[1][v], xss[2][v], xss[3][v]};
            std::ranges::sort(vals);
            result.insert(v, (vals[1] + vals[2]) / 2.0);
        }
        return result;
    }
};

} // namespace

QRectF layoutFlowchart(DiagramModel& model, const LayoutOptions& opts) {
    if (model.nodes.isEmpty()) {
        return {};
    }

    const bool vertical = isVertical(model.dir);

    WorkGraph g = buildWorkGraph(model);
    acyclic(g);
    rank(g);
    addBorderSegments(g, model, opts);
    const Chains chains = normalize(g, model, opts);

    int numRanks = 0;
    buildLayering(g, numRanks);
    Adj adj = buildAdj(g);
    order(g, adj, numRanks);

    // Y (rank axis) centers: stack per-rank max thickness with half rank-sep.
    QVector<QVector<int>> layers = buildLayering(g, numRanks);
    QVector<double> rankCenter(numRanks, 0.0);
    const double halfRankSep = opts.rankSep / 2.0;
    double cursor = opts.margin;
    for (int r = 0; r < numRanks; ++r) {
        double thick = 0.0;
        for (int v : layers[r]) {
            thick = std::max(thick, vertical ? g.nodes[v].h : g.nodes[v].w);
        }
        rankCenter[r] = cursor + (thick / 2.0);
        cursor += thick + halfRankSep;
    }
    const double rankTotal = cursor - halfRankSep + opts.margin;

    // X (cross axis) via Brandes-Koepf.
    BK bk{g, adj, opts, vertical, layers, {}};
    bk.pos.resize(g.nodes.size());
    for (const QVector<int>& layer : layers) {
        for (int i = 0; i < layer.size(); ++i) {
            bk.pos[layer[i]] = i;
        }
    }
    QHash<int, double> cross = bk.assign();

    double crossMin = kInf;
    for (auto it = cross.constBegin(); it != cross.constEnd(); ++it) {
        crossMin = std::min(crossMin, it.value() - bk.halfCross(it.key()));
    }
    if (crossMin == kInf) {
        crossMin = 0.0;
    }

    // Map canonical (cross, rank) -> page (x, y) by direction; store on work nodes.
    for (int v = 0; v < g.nodes.size(); ++v) {
        const double c = cross[v] - crossMin + opts.margin;
        const double q = rankCenter[g.nodes[v].rank];
        double px = 0.0;
        double py = 0.0;
        switch (model.dir) {
        case Direction::TB:
            px = c;
            py = q;
            break;
        case Direction::BT:
            px = c;
            py = rankTotal - q;
            break;
        case Direction::LR:
            px = q;
            py = c;
            break;
        case Direction::RL:
            px = rankTotal - q;
            py = c;
            break;
        }
        g.nodes[v].x = px;
        g.nodes[v].y = py;
    }

    // Write real node centers back to the model.
    for (const WorkNode& wn : g.nodes) {
        if (wn.kind == WorkKind::Real && wn.srcNode >= 0) {
            model.nodes[wn.srcNode].x = wn.x;
            model.nodes[wn.srcNode].y = wn.y;
            model.nodes[wn.srcNode].rank = wn.rank / 2;
            model.nodes[wn.srcNode].order = wn.order;
        }
    }

    // Edge routing: collect dummy bend points, clip endpoints to node shapes.
    for (int ei = 0; ei < model.edges.size(); ++ei) {
        DiagramEdge& edge = model.edges[ei];
        const int a = model.indexOf(edge.fromId);
        const int b = model.indexOf(edge.toId);
        if (a < 0 || b < 0) {
            continue;
        }
        const DiagramNode& na = model.nodes[a];
        const DiagramNode& nb = model.nodes[b];

        QVector<QPointF> mids;
        const auto it = chains.dummies.constFind(ei);
        if (it != chains.dummies.constEnd()) {
            for (int di : it.value()) {
                mids.push_back(QPointF(g.nodes[di].x, g.nodes[di].y));
            }
        }
        // Dummy chain runs low-rank -> high-rank. If the original edge was
        // reversed for acyclicity, that is target -> source, so flip it.
        bool reversed = false;
        for (const WorkEdge& we : g.edges) {
            if (we.edgeIdx == ei && we.reversed) {
                reversed = true;
                break;
            }
        }
        if (reversed) {
            std::ranges::reverse(mids);
        }

        const QRectF ra(na.x - (na.width / 2.0), na.y - (na.height / 2.0), na.width, na.height);
        const QRectF rb(nb.x - (nb.width / 2.0), nb.y - (nb.height / 2.0), nb.width, nb.height);
        const QPointF towardA = mids.isEmpty() ? QPointF(nb.x, nb.y) : mids.first();
        const QPointF towardB = mids.isEmpty() ? QPointF(na.x, na.y) : mids.last();
        const QPointF start = shapeBoundaryPoint(na.shape, ra, towardA);
        const QPointF end = shapeBoundaryPoint(nb.shape, rb, towardB);

        QVector<QPointF> pts;
        pts.push_back(start);
        pts += mids;
        pts.push_back(end);
        edge.points = pts;

        const auto lit = chains.labelDummy.constFind(ei);
        if (lit != chains.labelDummy.constEnd() && lit.value() >= 0) {
            edge.labelPos = QPointF(g.nodes[lit.value()].x, g.nodes[lit.value()].y);
        } else {
            edge.labelPos = (start + end) / 2.0;
        }
    }

    // Cluster bounds from member rectangles (recursive over nested members).
    for (DiagramCluster& cluster : model.clusters) {
        QRectF box;
        bool first = true;
        for (const DiagramNode& node : model.nodes) {
            // Membership: node's cluster chain includes this cluster.
            QString cid = node.parentCluster;
            bool inside = false;
            for (int guard = 0; guard < model.clusters.size() + 1 && !cid.isEmpty(); ++guard) {
                if (cid == cluster.id) {
                    inside = true;
                    break;
                }
                int idx = -1;
                for (int i = 0; i < model.clusters.size(); ++i) {
                    if (model.clusters[i].id == cid) {
                        idx = i;
                        break;
                    }
                }
                cid = (idx >= 0) ? model.clusters[idx].parentId : QString();
            }
            if (!inside) {
                continue;
            }
            const QRectF r(node.x - (node.width / 2.0), node.y - (node.height / 2.0), node.width,
                           node.height);
            box = first ? r : box.united(r);
            first = false;
        }
        if (!first) {
            box.adjust(-opts.clusterPad, -opts.clusterPad - opts.clusterTitle, opts.clusterPad,
                       opts.clusterPad);
            cluster.bounds = box;
        }
    }

    // Overall bounds.
    QRectF bounds;
    bool first = true;
    for (const DiagramNode& node : model.nodes) {
        const QRectF r(node.x - (node.width / 2.0), node.y - (node.height / 2.0), node.width,
                       node.height);
        bounds = first ? r : bounds.united(r);
        first = false;
    }
    for (const DiagramEdge& edge : model.edges) {
        for (const QPointF& p : edge.points) {
            bounds = bounds.united(QRectF(p, QSizeF(0.1, 0.1)));
        }
    }
    for (const DiagramCluster& cluster : model.clusters) {
        if (cluster.bounds.isValid()) {
            bounds = bounds.united(cluster.bounds);
        }
    }
    bounds.adjust(-opts.margin, -opts.margin, opts.margin, opts.margin);
    return bounds;
}

} // namespace be::diagram
