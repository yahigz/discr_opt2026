#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <vector>

using namespace std;

namespace TSPSolver {
    struct Point {
	long double x;
	long double y;
	int id;
};

struct Individual {
	vector<int> order;
	long double cost = numeric_limits<long double>::infinity();
};

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

int chooseNearestNext(const vector<Point>& points, const vector<char>& used, int current) {
	int n = (int)points.size();
	int best = -1;
	long double bestDist2 = numeric_limits<long double>::infinity();

	for (int candidate = 0; candidate < n; ++candidate) {
		if (used[candidate]) {
			continue;
		}

		long double dx = points[current].x - points[candidate].x;
		long double dy = points[current].y - points[candidate].y;
		long double d2 = dx * dx + dy * dy;
		if (
			best == -1 || d2 < bestDist2 ||
			(d2 == bestDist2 && points[candidate].id < points[best].id)
		) {
			best = candidate;
			bestDist2 = d2;
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
		int next = chooseNearestNext(points, used, current);
		used[next] = true;
		order.push_back(next);
		current = next;
	}

	return order;
}

inline long double twoOptDelta(const vector<int>& order, const vector<Point>& points, int i, int j) {
	int n = (int)order.size();
	int a = order[i];
	int b = order[(i + 1) % n];
	int c = order[j];
	int d = order[(j + 1) % n];
	long double oldCost = edgeLen(points, a, b) + edgeLen(points, c, d);
	long double newCost = edgeLen(points, a, c) + edgeLen(points, b, d);
	return newCost - oldCost;
}

inline void applyTwoOptMove(vector<int>& order, int i, int j) {
	reverse(order.begin() + i + 1, order.begin() + j + 1);
}

void improveTwoOptRandom(vector<int>& order, const vector<Point>& points, mt19937_64& rng, int attempts) {
	int n = (int)order.size();
	if (n < 4 || attempts <= 0) {
		return;
	}

	uniform_int_distribution<int> distI(0, n - 2);
	for (int it = 0; it < attempts; ++it) {
		int i = distI(rng);
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
		long double delta = twoOptDelta(order, points, i, j);
		if (delta < -1e-12L) {
			applyTwoOptMove(order, i, j);
		}
	}
}

void applyNonSequentialFourChange(vector<int>& order, mt19937_64& rng) {
	int n = (int)order.size();
	if (n < 8) {
		return;
	}

	uniform_int_distribution<int> cut1(1, n - 7);
	int p1 = cut1(rng);
	uniform_int_distribution<int> cut2(p1 + 2, n - 5);
	int p2 = cut2(rng);
	uniform_int_distribution<int> cut3(p2 + 2, n - 3);
	int p3 = cut3(rng);
	uniform_int_distribution<int> cut4(p3 + 2, n - 1);
	int p4 = cut4(rng);

	vector<int> mutated;
	mutated.reserve(n);

	mutated.insert(mutated.end(), order.begin(), order.begin() + p1);      // A
	mutated.insert(mutated.end(), order.begin() + p3, order.begin() + p4); // D
	mutated.insert(mutated.end(), order.begin() + p2, order.begin() + p3); // C
	mutated.insert(mutated.end(), order.begin() + p1, order.begin() + p2); // B
	mutated.insert(mutated.end(), order.begin() + p4, order.end());         // E

	order.swap(mutated);
}

array<int, 2> parentNeighbors(const vector<int>& parent, const vector<int>& pos, int v) {
	int n = (int)parent.size();
	int p = pos[v];
	return {parent[(p - 1 + n) % n], parent[(p + 1) % n]};
}

bool isAdjacentInParent(const array<int, 2>& nb, int x) {
	return nb[0] == x || nb[1] == x;
}

vector<vector<int>> buildCommonFragments(
	const vector<int>& pa,
	const vector<int>& pb,
	const vector<Point>& points
) {
	int n = (int)pa.size();
	vector<int> posA(n), posB(n);
	for (int i = 0; i < n; ++i) {
		posA[pa[i]] = i;
		posB[pb[i]] = i;
	}

	vector<vector<int>> common(n);
	for (int v = 0; v < n; ++v) {
		auto nA = parentNeighbors(pa, posA, v);
		auto nB = parentNeighbors(pb, posB, v);
		for (int cand : nA) {
			if (isAdjacentInParent(nB, cand)) {
				common[v].push_back(cand);
			}
		}
	}

	vector<char> visited(n, false);
	vector<vector<int>> fragments;
	fragments.reserve(n);

	for (int start = 0; start < n; ++start) {
		if (visited[start]) {
			continue;
		}

		if (common[start].empty()) {
			visited[start] = true;
			fragments.push_back({start});
			continue;
		}

		int degreeOneStart = -1;
		vector<int> componentNodes;
		vector<int> stack = {start};
		visited[start] = true;

		while (!stack.empty()) {
			int v = stack.back();
			stack.pop_back();
			componentNodes.push_back(v);
			if ((int)common[v].size() == 1) {
				degreeOneStart = v;
			}
			for (int to : common[v]) {
				if (!visited[to]) {
					visited[to] = true;
					stack.push_back(to);
				}
			}
		}

		int first = degreeOneStart != -1 ? degreeOneStart : componentNodes[0];
		vector<int> seq;
		seq.reserve(componentNodes.size());
		int prev = -1;
		int cur = first;
		while (true) {
			seq.push_back(cur);
			int next = -1;
			for (int to : common[cur]) {
				if (to != prev) {
					next = to;
					break;
				}
			}
			if (next == -1) {
				break;
			}
			prev = cur;
			cur = next;
			if (cur == first) {
				break;
			}
		}

		bool isCycle = ((int)common[first].size() == 2) && ((int)seq.size() == (int)componentNodes.size());
		if (isCycle && seq.size() >= 2) {
			int m = (int)seq.size();
			int cutPos = 0;
			long double worst = -1.0L;
			for (int i = 0; i < m; ++i) {
				int a = seq[i];
				int b = seq[(i + 1) % m];
				long double w = edgeLen(points, a, b);
				if (w > worst) {
					worst = w;
					cutPos = i;
				}
			}
			rotate(seq.begin(), seq.begin() + ((cutPos + 1) % m), seq.end());
		}

		fragments.push_back(std::move(seq));
	}

	return fragments;
}

vector<int> dpxCrossover(
	const vector<int>& parentA,
	const vector<int>& parentB,
	const vector<Point>& points,
	mt19937_64& rng
) {
	vector<vector<int>> fragments = buildCommonFragments(parentA, parentB, points);
	if (fragments.empty()) {
		return parentA;
	}

	uniform_int_distribution<int> coin(0, 1);
	while (fragments.size() > 1) {
		int bestI = -1;
		int bestJ = -1;
		bool revI = false;
		bool revJ = false;
		long double bestCost = numeric_limits<long double>::infinity();

		for (int i = 0; i < (int)fragments.size(); ++i) {
			int iL = fragments[i].front();
			int iR = fragments[i].back();
			for (int j = i + 1; j < (int)fragments.size(); ++j) {
				int jL = fragments[j].front();
				int jR = fragments[j].back();

				struct Variant {
					long double cost;
					bool ri;
					bool rj;
				} variants[4] = {
					{edgeLen(points, iR, jL), false, false},
					{edgeLen(points, iR, jR), false, true},
					{edgeLen(points, iL, jL), true, false},
					{edgeLen(points, iL, jR), true, true},
				};

				for (const auto& v : variants) {
					if (v.cost < bestCost || (v.cost == bestCost && coin(rng) == 1)) {
						bestCost = v.cost;
						bestI = i;
						bestJ = j;
						revI = v.ri;
						revJ = v.rj;
					}
				}
			}
		}

		if (bestI == -1 || bestJ == -1) {
			break;
		}

		if (revI) {
			reverse(fragments[bestI].begin(), fragments[bestI].end());
		}
		if (revJ) {
			reverse(fragments[bestJ].begin(), fragments[bestJ].end());
		}

		fragments[bestI].insert(
			fragments[bestI].end(),
			fragments[bestJ].begin(),
			fragments[bestJ].end()
		);
		fragments.erase(fragments.begin() + bestJ);
	}

	return fragments.front();
}

Individual makeIndividual(vector<int> order, const vector<Point>& points) {
	return {std::move(order), 0.0L};
}

void evaluateIndividual(Individual& ind, const vector<Point>& points) {
	ind.cost = tourLengthIdx(ind.order, points);
}

vector<int> buildRandomPermutation(int n, mt19937_64& rng) {
	vector<int> order(n);
	iota(order.begin(), order.end(), 0);
	shuffle(order.begin(), order.end(), rng);
	return order;
}

vector<int> solveWithGenetic(const vector<Point>& points) {
	int n = (int)points.size();
	uint64_t seed = 42;
	mt19937_64 rng(seed);

	int populationSize = 36;
    if (n > 300) {
        populationSize = 12;
    }
    if (n > 10000) {
        populationSize = 12;
    }

	int localTwoOptIters = max(4000, 70 * n);
    if (n > 300) {
        localTwoOptIters = max(2000, 40 * n);
    }
    if (n > 10000) {
        localTwoOptIters = max(1000, 15 * n);
    }
	int localTwoOptItersChild = max(6000, 70 * n);
    if (n > 300) {
        localTwoOptItersChild = max(4000, 50 * n);
    }
    if (n > 10000) {
        localTwoOptItersChild = max(2000, 15 * n);
    }
	long double mutationProb = 0.19L;
	long double stagnationLimit = (n <= 1000 ? 1400 : 700);
	auto timeStart = chrono::steady_clock::now();
	int timeLimitMs = 120000;

	vector<Individual> pop;
	pop.reserve(populationSize);

	int nnStarts = min(populationSize / 2, max(3, min(n, 12)));
	for (int s = 0; s < nnStarts; ++s) {
		int start = (s * n) / nnStarts;
		vector<int> order = buildNearestNeighborTour(points, start);
		improveTwoOptRandom(order, points, rng, localTwoOptIters);
		Individual ind = makeIndividual(std::move(order), points);
		evaluateIndividual(ind, points);
		pop.push_back(std::move(ind));
	}

	while ((int)pop.size() < populationSize) {
		vector<int> order = buildRandomPermutation(n, rng);
		improveTwoOptRandom(order, points, rng, max(1200, 20 * n));
		Individual ind = makeIndividual(std::move(order), points);
		evaluateIndividual(ind, points);
		pop.push_back(std::move(ind));
	}

	int bestIdx = 0;
	for (int i = 1; i < populationSize; ++i) {
		if (pop[i].cost < pop[bestIdx].cost) {
			bestIdx = i;
		}
	}

	long double bestCost = pop[bestIdx].cost;
	vector<int> bestOrder = pop[bestIdx].order;

	uniform_real_distribution<long double> real01(0.0L, 1.0L);
	uniform_int_distribution<int> parentDist(0, populationSize - 1);
	int noImprove = 0;

	while (true) {
		auto now = chrono::steady_clock::now();
		int elapsedMs = (int)chrono::duration_cast<chrono::milliseconds>(now - timeStart).count();
		if (elapsedMs >= timeLimitMs || noImprove > stagnationLimit) {
			break;
		}

		int ia = parentDist(rng);
		int ib = parentDist(rng);
		if (ia == ib) {
			ib = (ib + 1) % populationSize;
		}

		vector<int> childOrder = dpxCrossover(pop[ia].order, pop[ib].order, points, rng);
		improveTwoOptRandom(childOrder, points, rng, localTwoOptItersChild);

		if (real01(rng) < mutationProb) {
			applyNonSequentialFourChange(childOrder, rng);
			improveTwoOptRandom(childOrder, points, rng, localTwoOptItersChild / 2);
		}

		Individual child = makeIndividual(std::move(childOrder), points);
		evaluateIndividual(child, points);

		int worstIdx = 0;
		for (int i = 1; i < populationSize; ++i) {
			if (pop[i].cost > pop[worstIdx].cost) {
				worstIdx = i;
			}
		}

		if (child.cost < pop[worstIdx].cost) {
			pop[worstIdx] = std::move(child);
		}

		bestIdx = 0;
		for (int i = 1; i < populationSize; ++i) {
			if (pop[i].cost < pop[bestIdx].cost) {
				bestIdx = i;
			}
		}

		if (pop[bestIdx].cost + 1e-12L < bestCost) {
			bestCost = pop[bestIdx].cost;
			bestOrder = pop[bestIdx].order;
			noImprove = 0;
		} else {
			++noImprove;
		}
	}

	return bestOrder;
}
};

struct Statements {
    int n;
    int v;
    int c;
    vector<int> d;
    vector<pair<double, double>> points;

    Statements(int n, int v, int c, vector<int> d, vector<pair<double, double>> points)
        : n(n), v(v), c(c), d(d), points(points) {}
};

namespace DSU {
    constexpr int kClusterDfsTimeLimitMs = 20000;
    constexpr int kClusterDfsMaxBranchingPerLevel = 5;
	constexpr double kRelevanceDistMultiplier = 0.3;
	constexpr int kTriesWithRelevance = 600;

    struct Edge {
        int a;
        int b;
        double dist;

		Edge(int a, int b, double dist) : a(a), b(b), dist(dist) {}
		Edge() : a(0), b(0), dist(0.0) {}

		int relevance = 0;
    };

    int find_set(vector<int>& parent, int v) {
        if (parent[v] == v) {
            return v;
        }
        return parent[v] = find_set(parent, parent[v]);
    }

    void union_sets(vector<int>& parent, vector<int>& rank, vector<int>& capacity, int a, int b) {
        a = find_set(parent, a);
        b = find_set(parent, b);
        if (a != b) {
            if (rank[a] < rank[b]) {
                swap(a, b);
            }
            parent[b] = a;
            capacity[a] += capacity[b];
            if (rank[a] == rank[b]) {
                ++rank[a];
            }
        }
    }

    bool check_can_unite(vector<int>& parent, vector<int>& capacity, int a, int b, int c) {
        a = find_set(parent, a);
        b = find_set(parent, b);
        if (a == b) {
            return false;
        }
        return capacity[a] + capacity[b] <= c;
    }

    bool tryBuildWithDfs(
        const vector<Edge>& edges,
        int edgeIndex,
        int targetSets,
        int currentSets,
        int capacityLimit,
        vector<int>& parent,
        vector<int>& rank,
        vector<int>& capacity,
        chrono::steady_clock::time_point deadline
    ) {
        if (currentSets <= targetSets) {
            return true;
        }
        if (edgeIndex >= (int)edges.size()) {
            return false;
        }
        if (chrono::steady_clock::now() >= deadline) {
            return false;
        }

        int triedEdges = 0;
        for (int i = edgeIndex; i < (int)edges.size(); ++i) {
            int a = find_set(parent, edges[i].a);
            int b = find_set(parent, edges[i].b);
            if (a == b) {
                continue;
            }
            if (capacity[a] + capacity[b] > capacityLimit) {
                continue;
            }
            if (triedEdges >= kClusterDfsMaxBranchingPerLevel) {
                break;
            }
            ++triedEdges;

            vector<int> parentNext = parent;
            vector<int> rankNext = rank;
            vector<int> capacityNext = capacity;
            union_sets(parentNext, rankNext, capacityNext, a, b);

            if (tryBuildWithDfs(
                    edges,
                    i + 1,
                    targetSets,
                    currentSets - 1,
                    capacityLimit,
                    parentNext,
                    rankNext,
                    capacityNext,
                    deadline
                )) {
                parent = std::move(parentNext);
                rank = std::move(rankNext);
                capacity = std::move(capacityNext);
                return true;
            }
        }

        return false;
    }

    vector<vector<int>> algoKruskal(const vector<pair<double, double>>& points, const Statements& stmt) {
        vector<int> parent(points.size());
        for (int i = 0; i < (int)points.size(); ++i) {
            parent[i] = i;
        }
        vector<int> rank(points.size(), 0);
        vector<int> capacity = stmt.d;
        vector<Edge> edges;
        for (int i = 1; i < stmt.n; ++i) {
            for (int j = i + 1; j < stmt.n; ++j) {
                double dx = points[i].first - points[j].first;
                double dy = points[i].second - points[j].second;
                edges.push_back({i, j, hypot(dx, dy)});
            }
        }
        sort(edges.begin(), edges.end(), [](const Edge& lhs, const Edge& rhs) {
			return lhs.dist < rhs.dist;
        });

        int total_sets = max(0, stmt.n - 1);
        auto deadline = chrono::steady_clock::now() + chrono::milliseconds(kClusterDfsTimeLimitMs);
        bool built = tryBuildWithDfs(
            edges,
            0,
            stmt.v,
            total_sets,
            stmt.c,
            parent,
            rank,
            capacity,
            deadline
        );

        if (!built) {
			bool valid = true;
            while (total_sets > stmt.v) {
                int bestA = -1;
                int bestB = -1;
                double bestDist = numeric_limits<double>::infinity();
                for (const auto& edge : edges) {
                    if (!check_can_unite(parent, capacity, edge.a, edge.b, stmt.c)) {
                        continue;
                    }
                    if (edge.dist < bestDist) {
                        bestDist = edge.dist;
                        bestA = edge.a;
                        bestB = edge.b;
                    }
                }
                if (bestA == -1 || bestB == -1) {
					valid = false;
                    break;
                }
                union_sets(parent, rank, capacity, bestA, bestB);
                --total_sets;
            }

			if (!valid) {
				for (int cnt = 0; cnt < kTriesWithRelevance; ++cnt) {
					sort(edges.begin(), edges.end(), [&](const Edge& lhs, const Edge& rhs) {
						if (lhs.relevance != rhs.relevance) {
							return lhs.relevance > rhs.relevance;
						}
						double lhsDist = lhs.dist;
						double rhsDist = rhs.dist;
						if (lhs.relevance > rhs.relevance) {
							lhsDist *= kRelevanceDistMultiplier;
						} else {
							if (rhs.relevance > lhs.relevance) {
								rhsDist *= kRelevanceDistMultiplier;
							}
						}
						return lhsDist < rhsDist;
					});
					total_sets = max(0, stmt.n - 1);
					for (int i = 0; i < (int)points.size(); ++i) {
         			   parent[i] = i;
        			}
					fill(rank.begin(), rank.end(), 0);
					capacity = stmt.d;
					while (total_sets > stmt.v) {
						int bestA = -1;
						int bestB = -1;
						double bestDist = numeric_limits<double>::infinity();
						for (const auto& edge : edges) {
							if (!check_can_unite(parent, capacity, edge.a, edge.b, stmt.c)) {
								continue;
							}
							if (edge.dist < bestDist) {
								bestDist = edge.dist;
								bestA = edge.a;
								bestB = edge.b;
							}
						}
						if (bestA == -1 || bestB == -1) {
							valid = false;
							break;
						}
						union_sets(parent, rank, capacity, bestA, bestB);
						--total_sets;
					}
					if (total_sets <= stmt.v) {
						valid = true;
						break;
					} else {
						int tmp = 0;;
						for (const auto& edge : edges) {
							if (find_set(parent, edge.a) == find_set(parent, edge.b)) {
								++tmp;
							}
						}
						for (auto& edge : edges) {
							if (find_set(parent, edge.a) == find_set(parent, edge.b)) {
								edge.relevance -= tmp;
								--tmp;
							}
						}
					}
				}
			}
		}

        vector<vector<int>> buckets(stmt.n);
        for (int i = 1; i < stmt.n; ++i) {
            int p = find_set(parent, i);
            buckets[p].push_back(i);
        }

        vector<vector<int>> clusters;
        clusters.reserve(stmt.v);
        for (auto& bucket : buckets) {
            if (bucket.empty()) {
                continue;
            }
            vector<int> component;
            component.reserve(bucket.size() + 1);
            component.push_back(0);
            component.insert(component.end(), bucket.begin(), bucket.end());
            clusters.push_back(std::move(component));
        }
        return clusters;
    }
};

int main() {
    int n;
    int v;
    int c;
    cin >> n >> v >> c;
    vector<int> d(n);
    vector<pair<double, double>> points(n);
    for (int i = 0; i < n; ++i) {
        cin >> d[i] >> points[i].first >> points[i].second;
    }
    Statements statements(n, v, c, d, points);
    vector<vector<int>> clusters = DSU::algoKruskal(points, statements);
    double ans = 0;
    vector<vector<int>> tours;
    for (const auto& cluster : clusters) {
        if (cluster.empty()) {
            continue;
        }
        vector<TSPSolver::Point> clusterPoints;
        for (int idx : cluster) {
            clusterPoints.push_back({points[idx].first, points[idx].second, idx});
        }
        vector<int> tour = TSPSolver::solveWithGenetic(clusterPoints);
        ans += TSPSolver::tourLengthIdx(tour, clusterPoints);
        vector<int> mappedTour;
        for (int idx : tour) {
            mappedTour.push_back(clusterPoints[idx].id);
        }
        tours.push_back(mappedTour);
    }
    cout << fixed << setprecision(20) << ans << endl;
    for (const auto& tour : tours) {
        cout << tour.size() << " ";
        for (int idx : tour) {
            cout << idx + 1 << " ";
        }
        cout << endl;
    }
}
