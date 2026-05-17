#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <tuple>
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

long double twoOptDelta(const vector<int>& order, const vector<Point>& points, int i, int j) {
	int n = (int)order.size();
	int a = order[i];
	int b = order[(i + 1) % n];
	int c = order[j];
	int d = order[(j + 1) % n];
	long double oldCost = edgeLen(points, a, b) + edgeLen(points, c, d);
	long double newCost = edgeLen(points, a, c) + edgeLen(points, b, d);
	return newCost - oldCost;
}

void applyTwoOptMove(vector<int>& order, int i, int j) {
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
	mutated.insert(mutated.end(), order.begin() + p4, order.end());        // E

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

namespace GRASP {
	using Edge = pair<int, int>;

	double dist(const pair<double, double>& a, const pair<double, double>& b) {
		double dx = a.first - b.first;
		double dy = a.second - b.second;
		return hypot(dx, dy);
	}

	vector<vector<double>> buildWeights(const vector<pair<double, double>>& pts) {
		int n = (int)pts.size();
		vector<vector<double>> w(n, vector<double>(n, 0.0));
		for (int i = 0; i < n; ++i) {
			for (int j = i + 1; j < n; ++j) {
				double d = dist(pts[i], pts[j]);
				double weight = 1.0 / (1.0 + d);
				w[i][j] = weight;
				w[j][i] = weight;
			}
		}
		return w;
	}

	vector<vector<int>> initializeHWE(const Statements& stmt, const vector<vector<double>>& wgt) {
		int n = stmt.n;
		int p = stmt.v;

		vector<char> assigned(n, false);
		vector<vector<int>> clusters(p);
		vector<int> nodes(n);
		iota(nodes.begin(), nodes.end(), 0);
		sort(nodes.begin(), nodes.end(), [&](int a, int b) {
			if (stmt.d[a] != stmt.d[b]) {
				return stmt.d[a] > stmt.d[b];
			}
			return a < b;
		});

		int k = 0;
		while (k < p) {
			int seed = -1;
			for (int i = 0; i < n; ++i) {
				if (!assigned[nodes[i]]) {
					seed = nodes[i];
					break;
				}
			}
			if (seed == -1) {
				break;
			}

			for (int v : nodes) {
				if (!assigned[v]) {
					clusters[k].push_back(v);
					assigned[v] = true;
					++k;
					break;
				}
			}
		}

		vector<tuple<double, int, int>> edges;
		for (int i = 0; i < n; ++i) {
			for (int j = i + 1; j < n; ++j) {
				edges.emplace_back(wgt[i][j], i, j);
			}
		}
		sort(edges.begin(), edges.end(), [](const auto& a, const auto& b) {
			return get<0>(a) > get<0>(b);
		});

		for (int i = 0; i < p; ++i) {
			if (clusters[i].empty()) {
				bool placed = false;
				for (const auto& e : edges) {
					int a = get<1>(e);
					int b = get<2>(e);
					if (assigned[a] || assigned[b]) {
						continue;
					}
					if (stmt.d[a] + stmt.d[b] <= stmt.c) {
						clusters[i].push_back(a);
						clusters[i].push_back(b);
						assigned[a] = true;
						assigned[b] = true;
						placed = true;
						break;
					}
				}
				if (!placed) {
					break;
				}
			} else {
				int cur = clusters[i].front();
				int best = -1;
				double bestW = -1.0;
				for (int v = 0; v < n; ++v) {
					if (!assigned[v] && stmt.d[cur] + stmt.d[v] <= stmt.c && wgt[cur][v] > bestW) {
						bestW = wgt[cur][v];
						best = v;
					}
				}
				if (best != -1) {
					clusters[i].push_back(best);
					assigned[best] = true;
				}
			}
		}

		return clusters;
	}

	struct Solution {
		vector<int> assign;
		double value;
	};

	struct GraspResult {
		vector<vector<int>> clusters;
		bool timedOut = false;
	};

	bool timeExceeded(const chrono::steady_clock::time_point& deadline) {
		return chrono::steady_clock::now() >= deadline;
	}

	double evaluateSolution(const Solution& sol, const vector<vector<double>>& wgt) {
		int n = (int)sol.assign.size();
		double val = 0.0;
		for (int i = 0; i < n; ++i) {
			for (int j = i + 1; j < n; ++j) {
				if (sol.assign[i] == sol.assign[j]) {
					val += wgt[i][j];
				}
			}
		}
		return val;
	}

	vector<vector<int>> buildClustersFromAssign(const vector<int>& assign, int p) {
		vector<vector<int>> cls(p);
		for (int i = 0; i < (int)assign.size(); ++i) {
			if (assign[i] >= 0) {
				cls[assign[i]].push_back(i);
			}
		}
		return cls;
	}

	struct Candidate {
		int node;
		int cluster;
		double gain;
	};

	vector<Candidate> buildCL(const Solution& sol, const Statements& stmt, const vector<vector<double>>& wgt) {
		int n = stmt.n;
		int p = stmt.v;
		vector<int> load(p, 0);
		for (int i = 0; i < n; ++i) {
			if (sol.assign[i] >= 0) {
				load[sol.assign[i]] += stmt.d[i];
			}
		}

		vector<vector<int>> cls = buildClustersFromAssign(sol.assign, p);
		vector<Candidate> CL;
		CL.reserve(n * p);

		for (int node = 0; node < n; ++node) {
			if (sol.assign[node] != -1) {
				continue;
			}
			for (int k = 0; k < p; ++k) {
				if (load[k] + stmt.d[node] > stmt.c) {
					continue;
				}
				double gain = 0.0;
				for (int v : cls[k]) {
					gain += wgt[node][v];
				}
				CL.push_back({node, k, gain});
			}
		}

		sort(CL.begin(), CL.end(), [](const Candidate& a, const Candidate& b) {
			return a.gain > b.gain;
		});
		return CL;
	}

	void localSearchRVND(
		Solution& sol,
		const Statements& stmt,
		const vector<vector<double>>& wgt,
		const chrono::steady_clock::time_point& deadline,
		int maxNoImprove = 100,
		bool* timedOut = nullptr
	) {
		int n = stmt.n;
		int p = stmt.v;
		int noImp = 0;
		double bestVal = evaluateSolution(sol, wgt);

		while (noImp < maxNoImprove) {
			if (timeExceeded(deadline)) {
				if (timedOut != nullptr) {
					*timedOut = true;
				}
				return;
			}

			bool improved = false;
			for (int i = 0; i < n && !improved; ++i) {
				if (timeExceeded(deadline)) {
					if (timedOut != nullptr) {
						*timedOut = true;
					}
					return;
				}

				int cur = sol.assign[i];
				for (int k = 0; k < p && !improved; ++k) {
					if (k == cur) {
						continue;
					}
					if (timeExceeded(deadline)) {
						if (timedOut != nullptr) {
							*timedOut = true;
						}
						return;
					}

					vector<int> load(p, 0);
					for (int t = 0; t < n; ++t) {
						if (sol.assign[t] >= 0) {
							load[sol.assign[t]] += stmt.d[t];
						}
					}
					if (load[k] + stmt.d[i] > stmt.c) {
						continue;
					}

					int old = sol.assign[i];
					sol.assign[i] = k;
					double val = evaluateSolution(sol, wgt);
					if (val > bestVal + 1e-15) {
						bestVal = val;
						improved = true;
						break;
					}
					sol.assign[i] = old;
				}
			}

			if (improved) {
				noImp = 0;
				continue;
			}

			for (int i = 0; i < n && !improved; ++i) {
				for (int j = i + 1; j < n && !improved; ++j) {
					if (timeExceeded(deadline)) {
						if (timedOut != nullptr) {
							*timedOut = true;
						}
						return;
					}

					int a = sol.assign[i];
					int b = sol.assign[j];
					if (a == b) {
						continue;
					}

					vector<int> load(p, 0);
					for (int t = 0; t < n; ++t) {
						if (sol.assign[t] >= 0) {
							load[sol.assign[t]] += stmt.d[t];
						}
					}
					if (load[a] - stmt.d[i] + stmt.d[j] > stmt.c) {
						continue;
					}
					if (load[b] - stmt.d[j] + stmt.d[i] > stmt.c) {
						continue;
					}

					swap(sol.assign[i], sol.assign[j]);
					double val = evaluateSolution(sol, wgt);
					if (val > bestVal + 1e-15) {
						bestVal = val;
						improved = true;
						break;
					}
					swap(sol.assign[i], sol.assign[j]);
				}
			}

			if (improved) {
				noImp = 0;
				continue;
			}
			++noImp;
		}

		sol.value = bestVal;
	}

	Solution pathRelink(
		const Solution& s,
		const Solution& t,
		const Statements& stmt,
		const vector<vector<double>>& wgt,
		const chrono::steady_clock::time_point& deadline,
		bool* timedOut = nullptr
	) {
		Solution cur = s;
		int n = stmt.n;
		double bestVal = cur.value;
		Solution best = cur;
		vector<int> diff;

		for (int i = 0; i < n; ++i) {
			if (cur.assign[i] != t.assign[i]) {
				diff.push_back(i);
			}
		}

		for (int node : diff) {
			if (timeExceeded(deadline)) {
				if (timedOut != nullptr) {
					*timedOut = true;
				}
				return best;
			}

			vector<int> load(stmt.v, 0);
			for (int i = 0; i < n; ++i) {
				if (cur.assign[i] >= 0) {
					load[cur.assign[i]] += stmt.d[i];
				}
			}

			int target = t.assign[node];
			if (target < 0 || load[target] + stmt.d[node] > stmt.c) {
				continue;
			}

			cur.assign[node] = target;
			localSearchRVND(cur, stmt, wgt, deadline, 50, timedOut);
			if (timedOut != nullptr && *timedOut) {
				return best;
			}

			if (cur.value > bestVal + 1e-15) {
				bestVal = cur.value;
				best = cur;
			}
		}

		return best;
	}

	GraspResult solveGRASP(const Statements& stmt, int timeLimitSeconds = 60) {
		int n = stmt.n;
		int p = stmt.v;
		auto wgt = buildWeights(stmt.points);

		int maxIter = 50;
		int eliteSize = max(3, min(10, p));
		vector<double> A = {0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5};
		int m = (int)A.size();
		vector<double> probs(m, 1.0 / m), Av(m, 0.0);
		vector<Solution> elite;
		mt19937_64 rng(1234567);

		auto deadline = chrono::steady_clock::now() + chrono::seconds(timeLimitSeconds);
		bool timedOut = false;

		for (int iter = 0; iter < maxIter; ++iter) {
			if (timeExceeded(deadline)) {
				timedOut = true;
				break;
			}

			discrete_distribution<int> d(probs.begin(), probs.end());
			int ai = d(rng);
			double alpha = A[ai];

			auto clusters = initializeHWE(stmt, wgt);
			Solution sol;
			sol.assign.assign(n, -1);
			for (int k = 0; k < p; ++k) {
				for (int v : clusters[k]) {
					sol.assign[v] = k;
				}
			}

			while (true) {
				if (timeExceeded(deadline)) {
					timedOut = true;
					break;
				}

				auto CL = buildCL(sol, stmt, wgt);
				if (CL.empty()) {
					break;
				}

				int lCL = (int)CL.size();
				int lRCL = max(1, min((int)floor(alpha * lCL), lCL));
				uniform_int_distribution<int> pick(0, lRCL - 1);
				int sel = pick(rng);
				auto c = CL[sel];
				sol.assign[c.node] = c.cluster;
			}

			bool incomplete = false;
			for (int i = 0; i < n; ++i) {
				if (sol.assign[i] == -1) {
					incomplete = true;
					break;
				}
			}
			if (incomplete) {
				if (timedOut) {
					break;
				}
				continue;
			}

			sol.value = evaluateSolution(sol, wgt);
			localSearchRVND(sol, stmt, wgt, deadline, 150, &timedOut);
			if (timedOut) {
				break;
			}

			if ((int)elite.size() < eliteSize) {
				elite.push_back(sol);
			} else {
				int worst = 0;
				for (int i = 1; i < (int)elite.size(); ++i) {
					if (elite[i].value < elite[worst].value) {
						worst = i;
					}
				}
				if (sol.value > elite[worst].value) {
					elite[worst] = sol;
				}
			}

			Av[ai] += sol.value;
			if (iter > 0 && iter % 40 == 0) {
				double bestVal = 0.0;
				for (auto& e : elite) {
					bestVal = max(bestVal, e.value);
				}
				vector<double> q(m, 0.0);
				double delta = 10.0;
				for (int i = 0; i < m; ++i) {
					double Ai = (Av[i] > 0 ? Av[i] / 20.0 : 1e-9);
					q[i] = pow(Ai / max(1e-9, bestVal), delta);
				}
				double sumq = 0.0;
				for (double x : q) {
					sumq += x;
				}
				if (sumq > 0) {
					for (int i = 0; i < m; ++i) {
						probs[i] = q[i] / sumq;
					}
				}
			}
		}

		if (!timedOut && elite.size() >= 2) {
			for (size_t i = 0; i < elite.size(); ++i) {
				for (size_t j = i + 1; j < elite.size(); ++j) {
					Solution pr = pathRelink(elite[i], elite[j], stmt, wgt, deadline, &timedOut);
					int worst = 0;
					for (int t = 1; t < (int)elite.size(); ++t) {
						if (elite[t].value < elite[worst].value) {
							worst = t;
						}
					}
					if (pr.value > elite[worst].value) {
						elite[worst] = pr;
					}
					if (timedOut) {
						break;
					}
				}
				if (timedOut) {
					break;
				}
			}
		}

		GraspResult result;
		result.timedOut = timedOut;
		if (!elite.empty()) {
			int best = 0;
			for (int i = 1; i < (int)elite.size(); ++i) {
				if (elite[i].value > elite[best].value) {
					best = i;
				}
			}
			result.clusters = buildClustersFromAssign(elite[best].assign, p);
		}
		return result;
	}
}
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
		auto graspResult = GRASP::solveGRASP(statements, 60);
		vector<vector<int>> clusters = std::move(graspResult.clusters);
		if (graspResult.timedOut) {
			cerr << "GRASP clustering timed out; falling back to DSU clustering\n";
			clusters = DSU::algoKruskal(points, statements);
		}
    double ans = 0;
    vector<vector<int>> tours;
    for (const auto& cluster : clusters) {
        if (cluster.empty()) {
            continue;
        }
        vector<TSPSolver::Point> clusterPoints;\
		clusterPoints.push_back({points[0].first, points[0].second, 0});
		for (int idx : cluster) {
			if (idx == 0) continue;
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
