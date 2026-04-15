#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

using namespace std;

constexpr long double kAnnealInitialTempFloor = 1.0L;
constexpr long double kAnnealMinTemp = 1e-6L;
constexpr long double kAnnealCoolingAlpha = 0.9995L;
constexpr int kAnnealMinIterations = 200000;
constexpr int kAnnealMaxIterations = 3000000;
constexpr int kAnnealIterationsPerVertex = 1200;
constexpr int kAnnealMinTempPhaseIterations = 20000;

struct Point {
    long double x;
    long double y;
    int id;
};

long double cross(const Point& a, const Point& b, const Point& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

long double dist2(const Point& a, const Point& b) {
    long double dx = a.x - b.x;
    long double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

int chooseNearestNext(const vector<Point>& points, const vector<char>& used, int current) {
    int n = (int)points.size();
    int best = -1;
    long double bestDist = numeric_limits<long double>::infinity();

    for (int candidate = 0; candidate < n; ++candidate) {
        if (used[candidate]) {
            continue;
        }

        long double d2 = dist2(points[current], points[candidate]);
        if (
            best == -1 || d2 < bestDist ||
            (d2 == bestDist && points[candidate].id < points[best].id)
        ) {
            bestDist = d2;
            best = candidate;
        }
    }

    return best;
}

vector<int> buildNearestNeighborTour(const vector<Point>& points, int start) {
    int n = (int)points.size();
    vector<int> order;
    order.reserve(n);
    vector<char> used(n, false);

    int current = start;
    order.push_back(current);
    used[current] = true;

    for (int step = 1; step < n; ++step) {
        int best = chooseNearestNext(points, used, current);
        current = best;
        used[current] = true;
        order.push_back(current);
    }

    return order;
}

vector<int> buildConvexHullIdx(const vector<Point>& points) {
    int n = (int)points.size();
    vector<int> idx(n);
    for (int i = 0; i < n; ++i) {
        idx[i] = i;
    }

    sort(idx.begin(), idx.end(), [&](int i, int j) {
        if (points[i].x != points[j].x) return points[i].x < points[j].x;
        if (points[i].y != points[j].y) return points[i].y < points[j].y;
        return points[i].id < points[j].id;
    });

    if (n <= 1) {
        return idx;
    }

    vector<int> lower;
    for (int id : idx) {
        while (lower.size() >= 2 && cross(points[lower[lower.size() - 2]], points[lower.back()], points[id]) <= 0) {
            lower.pop_back();
        }
        lower.push_back(id);
    }

    vector<int> upper;
    for (int i = n - 1; i >= 0; --i) {
        int id = idx[i];
        while (upper.size() >= 2 && cross(points[upper[upper.size() - 2]], points[upper.back()], points[id]) <= 0) {
            upper.pop_back();
        }
        upper.push_back(id);
    }

    lower.pop_back();
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());
    return lower;
}

vector<int> buildPolarCentroidTour(const vector<Point>& points) {
    int n = (int)points.size();
    vector<int> order(n);
    for (int i = 0; i < n; ++i) {
        order[i] = i;
    }

    long double sx = 0.0L;
    long double sy = 0.0L;
    for (const auto& p : points) {
        sx += p.x;
        sy += p.y;
    }
    long double cx = sx / n;
    long double cy = sy / n;

    sort(order.begin(), order.end(), [&](int i, int j) {
        long double aix = points[i].x - cx;
        long double aiy = points[i].y - cy;
        long double ajx = points[j].x - cx;
        long double ajy = points[j].y - cy;

        bool upI = (aiy > 0) || (aiy == 0 && aix >= 0);
        bool upJ = (ajy > 0) || (ajy == 0 && ajx >= 0);
        if (upI != upJ) {
            return upI > upJ;
        }

        long double cr = aix * ajy - aiy * ajx;
        if (cr != 0) {
            return cr > 0;
        }

        long double di = aix * aix + aiy * aiy;
        long double dj = ajx * ajx + ajy * ajy;
        if (di != dj) {
            return di < dj;
        }

        return points[i].id < points[j].id;
    });

    vector<int> hull = buildConvexHullIdx(points);
    vector<char> isHull(n, false);
    for (int id : hull) {
        isHull[id] = true;
    }

    int startPos = 0;
    for (int i = 0; i < n; ++i) {
        if (isHull[order[i]]) {
            startPos = i;
            break;
        }
    }

    rotate(order.begin(), order.begin() + startPos, order.end());
    return order;
}

vector<int> buildRandomTour(const vector<Point>& points, int start) {
    int n = (int)points.size();
    vector<int> order;
    order.reserve(n);
    order.push_back(start);

    for (int i = 0; i < n; ++i) {
        if (i != start) {
            order.push_back(i);
        }
    }

    return order;
}

long double edgeLen(const vector<Point>& points, int a, int b) {
    long double dx = points[a].x - points[b].x;
    long double dy = points[a].y - points[b].y;
    return hypotl(dx, dy);
}

long double tourLengthIdx(const vector<int>& order, const vector<Point>& points) {
    int n = (int)order.size();
    if (n <= 1) {
        return 0.0L;
    }

    long double total = 0.0L;
    for (int i = 0; i < n; ++i) {
        int a = order[i];
        int b = order[(i + 1) % n];
        total += edgeLen(points, a, b);
    }
    return total;
}

// Standard 2-opt delta for replacing (a-b, c-d) with (a-c, b-d).
long double twoOptDelta(const vector<int>& order, const vector<Point>& points, int i, int j) {
    int a = order[i];
    int b = order[(i + 1) % (int)order.size()];
    int c = order[j];
    int d = order[(j + 1) % (int)order.size()];

    long double oldCost = edgeLen(points, a, b) + edgeLen(points, c, d);
    long double newCost = edgeLen(points, a, c) + edgeLen(points, b, d);
    return newCost - oldCost;
}

void applyTwoOptMove(vector<int>& order, int i, int j) {
    reverse(order.begin() + i + 1, order.begin() + j + 1);
}

vector<int> improveAnnealingTwoOpt(const vector<int>& initialOrder, const vector<Point>& points) {
    int n = (int)initialOrder.size();
    if (n < 4) {
        return initialOrder;
    }

    vector<int> current = initialOrder;
    vector<int> best = current;

    long double currentCost = tourLengthIdx(current, points);
    long double bestCost = currentCost;

    uint64_t seed = (uint64_t)chrono::steady_clock::now().time_since_epoch().count();
    mt19937_64 rng(seed);
    uniform_real_distribution<long double> prob(0.0L, 1.0L);

    long double t0 = max(kAnnealInitialTempFloor, currentCost / n);
    long double tMin = kAnnealMinTemp;
    long double alpha = kAnnealCoolingAlpha;
    long double temperature = t0;

    int maxIters = min(
        kAnnealMaxIterations,
        max(kAnnealMinIterations, kAnnealIterationsPerVertex * n)
    );
    uniform_int_distribution<int> distI(0, n - 2);
    int minTempIters = 0;

    for (int iter = 0; iter < maxIters || minTempIters < kAnnealMinTempPhaseIterations; ++iter) {
        int i = distI(rng);

        // j must satisfy i+1 < j and not select closing adjacent edge when i==0.
        int left = i + 2;
        int right = n - 1;
        if (i == 0) {
            right = n - 2;
        }
        if (left > right) {
            continue;
        }

        uniform_int_distribution<int> distJ(left, right);
        int j = distJ(rng);

        long double delta = twoOptDelta(current, points, i, j);
        bool accept = false;
        if (delta < 0.0L) {
            accept = true;
        } else {
            long double p = expl(-delta / max(temperature, 1e-12L));
            accept = prob(rng) < p;
        }

        if (accept) {
            applyTwoOptMove(current, i, j);
            currentCost += delta;

            if (currentCost < bestCost) {
                bestCost = currentCost;
                best = current;
            }
        }

        temperature *= alpha;
        if (temperature < tMin) {
            temperature = tMin;
        }

        if (temperature <= tMin) {
            ++minTempIters;
        }
    }

    return best;
}

void improveTwoOpt(vector<int>& order, const vector<Point>& points, int maxPasses = 10000) {
    int n = (int)order.size();
    if (n < 4) {
        return;
    }

    const long double eps = 1e-12L;

    for (int pass = 0; pass < maxPasses; ++pass) {
        bool improved = false;

        for (int i = 0; i < n; ++i) {
            int a = order[i];
            int b = order[(i + 1) % n];

            for (int j = i + 2; j < n; ++j) {
                if (i == 0 && j == n - 1) {
                    continue;
                }

                int c = order[j];
                int d = order[(j + 1) % n];

                long double oldCost = edgeLen(points, a, b) + edgeLen(points, c, d);
                long double newCost = edgeLen(points, a, c) + edgeLen(points, b, d);

                if (newCost + eps < oldCost) {
                    reverse(order.begin() + i + 1, order.begin() + j + 1);
                    improved = true;
                    break;
                }
            }

            if (improved) {
                break;
            }
        }

        if (!improved) {
            break;
        }
    }
}

void applyThreeOptCase(vector<int>& order, int i, int j, int k, int caseId) {
    if (caseId == 1) {
        reverse(order.begin() + i + 1, order.begin() + j + 1);
        return;
    }
    if (caseId == 2) {
        reverse(order.begin() + j + 1, order.begin() + k + 1);
        return;
    }
    if (caseId == 3) {
        reverse(order.begin() + i + 1, order.begin() + j + 1);
        reverse(order.begin() + j + 1, order.begin() + k + 1);
        return;
    }
    if (caseId == 4) {
        rotate(order.begin() + i + 1, order.begin() + j + 1, order.begin() + k + 1);
        return;
    }
    if (caseId == 5) {
        reverse(order.begin() + j + 1, order.begin() + k + 1);
        rotate(order.begin() + i + 1, order.begin() + j + 1, order.begin() + k + 1);
        return;
    }
    if (caseId == 6) {
        reverse(order.begin() + i + 1, order.begin() + j + 1);
        rotate(order.begin() + i + 1, order.begin() + j + 1, order.begin() + k + 1);
        return;
    }
    if (caseId == 7) {
        reverse(order.begin() + i + 1, order.begin() + k + 1);
        return;
    }
}

void improveThreeOpt(vector<int>& order, const vector<Point>& points, int maxPasses = 10000) {
    int n = (int)order.size();
    if (n < 6) {
        improveTwoOpt(order, points);
        return;
    }

    const long double eps = 1e-12L;

    for (int pass = 0; pass < maxPasses; ++pass) {
        bool improved = false;

        for (int i = 0; i <= n - 4; ++i) {
            int a = order[i];
            int b = order[i + 1];

            for (int j = i + 1; j <= n - 3; ++j) {
                int c = order[j];
                int d = order[j + 1];

                for (int k = j + 1; k <= n - 2; ++k) {
                    int e = order[k];
                    int f = order[k + 1];

                    long double d0 = edgeLen(points, a, b) + edgeLen(points, c, d) + edgeLen(points, e, f);
                    long double bestCost = d0;
                    int bestCase = 0;

                    long double d1 = edgeLen(points, a, c) + edgeLen(points, b, d) + edgeLen(points, e, f);
                    if (d1 + eps < bestCost) {
                        bestCost = d1;
                        bestCase = 1;
                    }

                    long double d2 = edgeLen(points, a, b) + edgeLen(points, c, e) + edgeLen(points, d, f);
                    if (d2 + eps < bestCost) {
                        bestCost = d2;
                        bestCase = 2;
                    }

                    long double d3 = edgeLen(points, a, c) + edgeLen(points, b, e) + edgeLen(points, d, f);
                    if (d3 + eps < bestCost) {
                        bestCost = d3;
                        bestCase = 3;
                    }

                    long double d4 = edgeLen(points, a, d) + edgeLen(points, e, b) + edgeLen(points, c, f);
                    if (d4 + eps < bestCost) {
                        bestCost = d4;
                        bestCase = 4;
                    }

                    long double d5 = edgeLen(points, a, e) + edgeLen(points, d, b) + edgeLen(points, c, f);
                    if (d5 + eps < bestCost) {
                        bestCost = d5;
                        bestCase = 5;
                    }

                    long double d6 = edgeLen(points, a, d) + edgeLen(points, e, c) + edgeLen(points, b, f);
                    if (d6 + eps < bestCost) {
                        bestCost = d6;
                        bestCase = 6;
                    }

                    long double d7 = edgeLen(points, a, e) + edgeLen(points, d, c) + edgeLen(points, b, f);
                    if (d7 + eps < bestCost) {
                        bestCost = d7;
                        bestCase = 7;
                    }

                    if (bestCase != 0) {
                        applyThreeOptCase(order, i, j, k, bestCase);
                        improved = true;
                        break;
                    }
                }

                if (improved) {
                    break;
                }
            }

            if (improved) {
                break;
            }
        }

        if (!improved) {
            break;
        }
    }
}

vector<Point> toPointOrder(const vector<int>& orderIdx, const vector<Point>& points) {
    vector<Point> order;
    order.reserve(orderIdx.size());
    for (int idx : orderIdx) {
        order.push_back(points[idx]);
    }
    return order;
}

long double tourLength(const vector<Point>& order) {
    if (order.size() <= 1) {
        return 0.0L;
    }

    long double length = 0.0L;
    for (size_t i = 0; i + 1 < order.size(); ++i) {
        long double dx = order[i + 1].x - order[i].x;
        long double dy = order[i + 1].y - order[i].y;
        length += hypotl(dx, dy);
    }

    long double dx = order.front().x - order.back().x;
    long double dy = order.front().y - order.back().y;
    length += hypotl(dx, dy);
    return length;
}

int main() {
    int n;
    cin >> n;
    vector<Point> points(n);
    for (int i = 0; i < n; ++i) {
        cin >> points[i].x >> points[i].y;
        points[i].id = i + 1;
    }

    vector<int> orderIdx;
    if (n < 5000) {
        orderIdx = buildPolarCentroidTour(points);
        orderIdx = improveAnnealingTwoOpt(orderIdx, points);
    } else {
        orderIdx = buildNearestNeighborTour(points, 0);
    }

    vector<Point> order = toPointOrder(orderIdx, points);

    cout << fixed << setprecision(10) << (double)tourLength(order) << '\n';
    for (int i = 0; i < n; ++i) {
        if (i) cout << ' ';
        cout << order[i].id;
    }
    cout << '\n';
    return 0;
}