#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

struct Statements {
    int n;
    int m;
    vector<int64_t> cost;
    vector<set<int>> sets;
    vector<set<int>> set_by_element;

    Statements(int n, int m, vector<int64_t> cost, vector<set<int>> sets, vector<set<int>> set_by_element)
        : n(n), m(m), cost(std::move(cost)), sets(std::move(sets)), set_by_element(std::move(set_by_element)) {}


    vector<double> GetSetRatings() const {
        vector<double> elem_ratings(n, numeric_limits<double>::infinity());
        for (int e = 0; e < n; ++e) {
            if (set_by_element[e].empty()) continue;
            int64_t sum = 0;
            for (int set_ind : set_by_element[e]) sum += cost[set_ind];
            elem_ratings[e] = static_cast<double>(sum) / static_cast<double>(set_by_element[e].size());
        }

        vector<double> set_ratings(m, numeric_limits<double>::infinity());
        for (int i = 0; i < m; ++i) {
            if (sets[i].empty()) continue;
            double sum = 0.0;
            int cnt = 0;
            for (int e : sets[i]) {
                if (e >= 0 && e < n && isfinite(elem_ratings[e])) {
                    sum += elem_ratings[e];
                    ++cnt;
                }
            }
            if (cnt > 0) set_ratings[i] = sum / static_cast<double>(cnt);
        }
        return set_ratings;
    }
};

struct Individual {
    vector<uint8_t> chosen;
    int64_t cost = numeric_limits<int64_t>::max();
    vector<int> tabu_until;
};

enum class FillHeuristic {
    CostPerNewElement = 0,
    CostMinusAltElementAvg = 1,
};

static constexpr FillHeuristic kFillHeuristic = FillHeuristic::CostPerNewElement;

struct GAParams {
    int pop_size = 36;
    int generations = 500;
    int tournament_k = 5;
    int tabu_tenure = 6;
    int max_banned_mutation = 4;
    uint32_t rng_seed = 42;
};

static int64_t EvalCost(const Statements& st, const vector<uint8_t>& chosen) {
    int64_t total = 0;
    for (int i = 0; i < st.m; ++i) if (chosen[i]) total += st.cost[i];
    return total;
}

static bool IsFeasible(const Statements& st, const vector<uint8_t>& chosen) {
    vector<uint8_t> covered(st.n, 0);
    int covered_cnt = 0;
    for (int i = 0; i < st.m; ++i) {
        if (!chosen[i]) continue;
        for (int e : st.sets[i]) {
            if (e >= 0 && e < st.n && !covered[e]) {
                covered[e] = 1;
                ++covered_cnt;
            }
        }
    }
    return covered_cnt == st.n;
}

static void PruneRedundant(const Statements& st, vector<uint8_t>& chosen) {
    vector<int> cover_cnt(st.n, 0);
    for (int s = 0; s < st.m; ++s) {
        if (!chosen[s]) continue;
        for (int e : st.sets[s]) {
            if (e >= 0 && e < st.n) ++cover_cnt[e];
        }
    }

    vector<int> selected;
    selected.reserve(st.m);
    for (int s = 0; s < st.m; ++s) {
        if (chosen[s]) selected.push_back(s);
    }

    sort(selected.begin(), selected.end(), [&](int a, int b) {
        if (st.cost[a] != st.cost[b]) return st.cost[a] > st.cost[b];
        return st.sets[a].size() < st.sets[b].size();
    });

    for (int s : selected) {
        bool can_remove = true;
        for (int e : st.sets[s]) {
            if (e >= 0 && e < st.n && cover_cnt[e] <= 1) {
                can_remove = false;
                break;
            }
        }
        if (!can_remove) continue;

        chosen[s] = 0;
        for (int e : st.sets[s]) {
            if (e >= 0 && e < st.n) --cover_cnt[e];
        }
    }
}

static vector<double> BuildElemCostSums(const Statements& st) {
    vector<double> elem_cost_sum(st.n, 0.0);
    for (int e = 0; e < st.n; ++e) {
        double sum = 0.0;
        for (int s : st.set_by_element[e]) {
            sum += static_cast<double>(st.cost[s]);
        }
        elem_cost_sum[e] = sum;
    }
    return elem_cost_sum;
}

static inline double AltElemAvgExcludingCurrent(
    const Statements& st,
    const vector<double>& elem_cost_sum,
    int set_idx,
    int elem
) {
    int deg = static_cast<int>(st.set_by_element[elem].size());
    if (deg <= 1) {
        return static_cast<double>(st.cost[set_idx]);
    }
    return (elem_cost_sum[elem] - static_cast<double>(st.cost[set_idx])) / static_cast<double>(deg - 1);
}

static vector<uint8_t> GreedyFill(
    const Statements& st,
    const vector<uint8_t>& base_chosen,
    const vector<uint8_t>& banned_now,
    const vector<double>& elem_cost_sum
) {
    vector<uint8_t> chosen = base_chosen;
    vector<uint8_t> covered(st.n, 0);
    int covered_cnt = 0;
    vector<int> gain(st.m, 0);
    vector<double> alt_sum(st.m, 0.0);

    for (int s = 0; s < st.m; ++s) {
        gain[s] = static_cast<int>(st.sets[s].size());
        if (kFillHeuristic == FillHeuristic::CostMinusAltElementAvg) {
            double sum = 0.0;
            for (int e : st.sets[s]) {
                sum += AltElemAvgExcludingCurrent(st, elem_cost_sum, s, e);
            }
            alt_sum[s] = sum;
        }
    }

    auto apply_set = [&](int s) {
        for (int e : st.sets[s]) {
            if (e >= 0 && e < st.n && !covered[e]) {
                covered[e] = 1;
                ++covered_cnt;

                for (int t : st.set_by_element[e]) {
                    if (gain[t] > 0) --gain[t];
                    if (kFillHeuristic == FillHeuristic::CostMinusAltElementAvg) {
                        alt_sum[t] -= AltElemAvgExcludingCurrent(st, elem_cost_sum, t, e);
                    }
                }
            }
        }
    };

    for (int i = 0; i < st.m; ++i) if (chosen[i]) apply_set(i);

    auto pick_best = [&](bool allow_tabu) -> int {
        int best = -1;
        double best_score = numeric_limits<double>::infinity();
        int best_gain = -1;

        for (int s = 0; s < st.m; ++s) {
            if (chosen[s]) continue;
            if (!allow_tabu && banned_now[s]) continue;

            int cur_gain = gain[s];
            if (cur_gain == 0) continue;

            double score;
            if (kFillHeuristic == FillHeuristic::CostPerNewElement) {
                score = static_cast<double>(st.cost[s]) / static_cast<double>(cur_gain);
            } else {
                double mean_alt = alt_sum[s] / static_cast<double>(cur_gain);
                score = static_cast<double>(st.cost[s]) - mean_alt;
            }

            if (score < best_score - 1e-12 ||
                (abs(score - best_score) <= 1e-12 &&
                 (cur_gain > best_gain ||
                  (cur_gain == best_gain && st.cost[s] < st.cost[best])))) {
                best = s;
                best_score = score;
                best_gain = cur_gain;
            }
        }
        return best;
    };

    while (covered_cnt < st.n) {
        int nxt = pick_best(false);
        if (nxt == -1) return {};
        chosen[nxt] = 1;
        apply_set(nxt);
    }

    return chosen;
}

static Individual MakeIndividual(const Statements& st, const vector<uint8_t>& bits) {
    Individual ind;
    ind.chosen = bits;
    ind.cost = bits.empty() ? numeric_limits<int64_t>::max() : EvalCost(st, bits);
    ind.tabu_until.assign(st.m, -1);
    return ind;
}

static int TournamentPick(const vector<Individual>& pop, mt19937& rng, int tournament_k) {
    uniform_int_distribution<int> uid(0, static_cast<int>(pop.size()) - 1);
    int best = uid(rng);
    tournament_k = max(1, min(tournament_k, static_cast<int>(pop.size())));
    for (int i = 1; i < tournament_k; ++i) {
        int c = uid(rng);
        if (pop[c].cost < pop[best].cost) best = c;
    }
    return best;
}

int main() {
    GAParams params;

    int n, m;
    cin >> n >> m;
    cin.ignore();

    vector<int64_t> cost(m);
    vector<set<int>> sets(m);

    vector<set<int>> set_by_element(n);

    for (int i = 0; i < m; i++) {
        string line;
        getline(cin, line);
        stringstream ss(line);

        ss >> cost[i];
        int element;
        while (ss >> element) {
            if (element >= 0 && element < n) {
                sets[i].insert(element);
                set_by_element[element].insert(i);
            }
        }
    }

    Statements st(n, m, cost, sets, set_by_element);

    for (int e = 0; e < n; ++e) {
        if (st.set_by_element[e].empty()) {
            cout << -1 << '\n' << '\n';
            return 0;
        }
    }

    mt19937 rng(params.rng_seed);
    vector<double> elem_cost_sum = BuildElemCostSums(st);

    vector<Individual> population;
    population.reserve(params.pop_size);
    uniform_real_distribution<double> p01(0.0, 1.0);

    for (int i = 0; i < params.pop_size; ++i) {
        vector<uint8_t> empty(st.m, 0);
        vector<uint8_t> random_ban(st.m, 0);
        for (int s = 0; s < st.m; ++s) {
            if (p01(rng) < 0.15) random_ban[s] = 1;
        }

        auto bits = GreedyFill(st, empty, random_ban, elem_cost_sum);
        if (bits.empty()) {
            vector<uint8_t> no_ban(st.m, 0);
            bits = GreedyFill(st, empty, no_ban, elem_cost_sum);
        }
        if (!bits.empty()) {
            PruneRedundant(st, bits);
            population.push_back(MakeIndividual(st, bits));
        }
    }

    if (population.empty()) {
        cout << -1 << '\n' << '\n';
        return 0;
    }

    auto cmp = [](const Individual& a, const Individual& b) { return a.cost < b.cost; };
    sort(population.begin(), population.end(), cmp);
    if (static_cast<int>(population.size()) > params.pop_size) population.resize(params.pop_size);
    Individual best = population.front();

    for (int gen = 0; gen < params.generations; ++gen) {

        vector<Individual> offspring;
        offspring.reserve(params.pop_size);

        for (int k = 0; k < params.pop_size; ++k) {
            int i1 = TournamentPick(population, rng, params.tournament_k);
            int i2 = TournamentPick(population, rng, params.tournament_k);
            const auto& p1 = population[i1];
            const auto& p2 = population[i2];

            vector<int> child_tabu = p1.tabu_until;
            for (int s = 0; s < m; ++s) {
                child_tabu[s] = max(child_tabu[s], p2.tabu_until[s]);
            }

            vector<uint8_t> child_bits(st.m, 0);
            for (int s = 0; s < st.m; ++s) {
                if (p1.chosen[s] && p2.chosen[s]) child_bits[s] = 1;
            }

            vector<uint8_t> banned_now(m, 0);
            for (int s = 0; s < m; ++s) {
                if (child_tabu[s] > gen) banned_now[s] = 1;
            }

            vector<int> chosen_ids;
            for (int s = 0; s < st.m; ++s) {
                if (child_bits[s]) chosen_ids.push_back(s);
            }

            vector<uint8_t> active_ban = banned_now;
            int mutation_upper = min(params.max_banned_mutation, static_cast<int>(chosen_ids.size()));
            if (!chosen_ids.empty() && mutation_upper > 0) {
                shuffle(chosen_ids.begin(), chosen_ids.end(), rng);
                uniform_int_distribution<int> cnt_dist(
                    1,
                    mutation_upper
                );
                int ban_cnt = cnt_dist(rng);

                for (int j = 0; j < ban_cnt; ++j) {
                    int s = chosen_ids[j];
                    child_bits[s] = 0;
                    active_ban[s] = 1;
                    child_tabu[s] = max(child_tabu[s], gen + params.tabu_tenure);
                }
            }

            child_bits = GreedyFill(st, child_bits, active_ban, elem_cost_sum);
            if (child_bits.empty()) {
                vector<uint8_t> no_ban(st.m, 0);
                child_bits = GreedyFill(st, child_bits, no_ban, elem_cost_sum);
                if (child_bits.empty()) continue;
            }

            PruneRedundant(st, child_bits);
            Individual child = MakeIndividual(st, child_bits);
            child.tabu_until = child_tabu;
            offspring.push_back(std::move(child));
        }

        vector<Individual> pool = population;
        pool.insert(pool.end(), offspring.begin(), offspring.end());
        sort(pool.begin(), pool.end(), cmp);
        if (static_cast<int>(pool.size()) > params.pop_size) pool.resize(params.pop_size);
        population.swap(pool);

        if (!population.empty() && population.front().cost < best.cost) {
            best = population.front();
        }
    }

    if (!IsFeasible(st, best.chosen)) {
        cout << -1 << '\n' << '\n';
        return 0;
    }

    cout << best.cost << '\n';
    for (int i = 0; i < m; ++i) {
        if (best.chosen[i]) cout << (i + 1) << ' ';
    }
    cout << '\n';
    return 0;
}
