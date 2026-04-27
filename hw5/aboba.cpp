#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <random>

using namespace std;

const int NUMBER_OF_TRIES = 5;
const int CONSIDERING_M = 10;
const double INF = 1e18;
const double RANDOM_RATIO = 0.2; 
const int TOTAL_TWO_OPT_ITERS = 3000; 
const int MAX_PASSES = 2;

double dist2(pair<double, double> a, pair<double, double> b) {
    return (a.first - b.first) * (a.first - b.first) + (a.second - b.second) * (a.second - b.second);
}

struct Statements {
    int n, m;
    vector<double> cost;
    vector<int> capacity;
    vector<pair<double, double>> shop;
    vector<int> demand;
    vector<pair<double, double>> customer;
    vector<vector<double>> customer_to_shop_dist;
    
    Statements(int n, int m, vector<double> cost, vector<int> capacity, vector<pair<double, double>> shop, vector<int> demand, vector<pair<double, double>> customer) 
        : n(n), m(m), cost(cost), capacity(capacity), shop(shop), demand(demand), customer(customer) {
        customer_to_shop_dist.assign(n, vector<double>(m));
        for(int i = 0; i < n; ++i) {
            for(int j = 0; j < m; ++j) {
                customer_to_shop_dist[i][j] = sqrt(dist2(shop[i], customer[j]));
            }
        }
    }

    vector<double> GetShopRatings() const {
        vector<double> ratings(n, 0);
        vector<double> best_distance(m, INF);
        for (int j = 0; j < m; j++) {
            for (int i = 0; i < n; i++) {
                best_distance[j] = min(best_distance[j], customer_to_shop_dist[i][j] + cost[i] / (double)CONSIDERING_M);
            }
        }
        for (int i = 0; i < n; i++) {
            vector<pair<double, int>> curr;
            for (int j = 0; j < m; j++) curr.push_back({customer_to_shop_dist[i][j], j});
            sort(curr.begin(), curr.end());
            for (int j = 0; j < min(CONSIDERING_M, m); j++) {
                int c_idx = curr[j].second;
                ratings[i] += (customer_to_shop_dist[i][c_idx] + cost[i] / (double)CONSIDERING_M) / (best_distance[c_idx] + 1e-9);
            }
        }
        return ratings;
    }
};

struct SolutionCase {
    double answer;
    vector<int> opened;
    vector<int> chosen; 
    vector<int> used_capacity;
    vector<int> banned;
    Statements statements;
    bool valid;

    SolutionCase(Statements statements, vector<int> banned = {}) : banned(banned), statements(statements) {
        if (this->banned.empty()) this->banned.assign(statements.n, 0);
        opened.assign(statements.n, 0);
        chosen.assign(statements.m, 0);
        used_capacity.assign(statements.n, 0);
        valid = true;
        Solve();
    }

    void Solve();
    bool TwoOpt(int shop_i, int shop_j); 
    void FinalSync();
};

void SolutionCase::FinalSync() {
    answer = 0;
    fill(used_capacity.begin(), used_capacity.end(), 0);
    vector<int> active(statements.n, 0);
    for (int j = 0; j < statements.m; j++) {
        if (chosen[j] <= 0) continue;
        int s_idx = chosen[j] - 1;
        used_capacity[s_idx] += statements.demand[j];
        answer += statements.customer_to_shop_dist[s_idx][j];
        active[s_idx] = 1;
    }
    for (int i = 0; i < statements.n; i++) {
        opened[i] = active[i];
        if (opened[i]) answer += statements.cost[i];
    }
}

bool SolutionCase::TwoOpt(int shop_i, int shop_j) {
    vector<pair<double, int>> to_j, to_i;
    for (int i = 0; i < (int)chosen.size(); i++) {
        if (chosen[i] == shop_i + 1) {
            if (statements.customer_to_shop_dist[shop_j][i] < statements.customer_to_shop_dist[shop_i][i])
                to_j.push_back({statements.customer_to_shop_dist[shop_j][i] - statements.customer_to_shop_dist[shop_i][i], i});
        }
        if (chosen[i] == shop_j + 1) {
            if (statements.customer_to_shop_dist[shop_i][i] < statements.customer_to_shop_dist[shop_j][i])
                to_i.push_back({statements.customer_to_shop_dist[shop_i][i] - statements.customer_to_shop_dist[shop_j][i], i});
        }
    }
    sort(to_j.begin(), to_j.end());
    sort(to_i.begin(), to_i.end());
    
    int capacity_i = statements.capacity[shop_i] - used_capacity[shop_i];
    int capacity_j = statements.capacity[shop_j] - used_capacity[shop_j];
    
    vector<int> all_i, all_j;
    for (int i = 0; i < (int)chosen.size(); ++i) {
        if (chosen[i] == shop_i + 1) all_i.push_back(i);
        if (chosen[i] == shop_j + 1) all_j.push_back(i);
    }

    double best_diff = 0;
    vector<int> best_to_j, best_to_i;

    vector<int> curr_to_i; double curr_diff = 0; int curr_dem = 0;
    for (int i = 0; i < (int)to_i.size(); ++i) {
        curr_to_i.push_back(to_i[i].second);
        curr_diff += to_i[i].first;
        curr_dem += statements.demand[to_i[i].second];
        vector<vector<double>> knapsack(all_i.size() + 1, vector<double>(capacity_j + curr_dem + 1, INF));
        knapsack[0][0] = 0;
        for (int j = 0; j < (int)all_i.size(); ++j) {
            for (int w = 0; w <= curr_dem + capacity_j; ++w) {
                knapsack[j + 1][w] = min(knapsack[j + 1][w], knapsack[j][w]);
                if (w + statements.demand[all_i[j]] <= curr_dem + capacity_j)
                    knapsack[j + 1][w + statements.demand[all_i[j]]] = min(knapsack[j + 1][w + statements.demand[all_i[j]]], knapsack[j][w] + statements.customer_to_shop_dist[shop_j][all_i[j]] - statements.customer_to_shop_dist[shop_i][all_i[j]]);
            }
        }
        double best_local_diff = INF; int best_w = -1;
        for (int w = max(0, curr_dem - capacity_i); w <= curr_dem + capacity_j; ++w)
            if (knapsack[all_i.size()][w] < best_local_diff) { best_local_diff = knapsack[all_i.size()][w]; best_w = w; }

        if (curr_diff + best_local_diff < best_diff) {
            best_diff = curr_diff + best_local_diff; best_to_i = curr_to_i; best_to_j.clear();
            int w = best_w;
            for (int j = (int)all_i.size() - 1; j >= 0; --j)
                if (abs(knapsack[j + 1][w] - knapsack[j][w]) > 1e-9) { best_to_j.push_back(all_i[j]); w -= statements.demand[all_i[j]]; }
        } else { curr_diff -= to_i[i].first; curr_dem -= statements.demand[to_i[i].second]; curr_to_i.pop_back(); }
    }

    vector<int> curr_to_j; curr_diff = 0; curr_dem = 0;
    for (int i = 0; i < (int)to_j.size(); ++i) {
        curr_to_j.push_back(to_j[i].second);
        curr_diff += to_j[i].first;
        curr_dem += statements.demand[to_j[i].second];
        vector<vector<double>> knapsack(all_j.size() + 1, vector<double>(capacity_i + curr_dem + 1, INF));
        knapsack[0][0] = 0;
        for (int j = 0; j < (int)all_j.size(); ++j) {
            for (int w = 0; w <= curr_dem + capacity_i; ++w) {
                knapsack[j + 1][w] = min(knapsack[j + 1][w], knapsack[j][w]);
                if (w + statements.demand[all_j[j]] <= curr_dem + capacity_i)
                    knapsack[j + 1][w + statements.demand[all_j[j]]] = min(knapsack[j + 1][w + statements.demand[all_j[j]]], knapsack[j][w] + statements.customer_to_shop_dist[shop_i][all_j[j]] - statements.customer_to_shop_dist[shop_j][all_j[j]]);
            }
        }
        double best_local_diff = INF; int best_w = -1;
        for (int w = max(0, curr_dem - capacity_j); w <= curr_dem + capacity_i; ++w)
            if (knapsack[all_j.size()][w] < best_local_diff) { best_local_diff = knapsack[all_j.size()][w]; best_w = w; }
        if (curr_diff + best_local_diff < best_diff) {
            best_diff = curr_diff + best_local_diff; best_to_j = curr_to_j; best_to_i.clear();
            int w = best_w;
            for (int j = (int)all_j.size() - 1; j >= 0; --j)
                if (abs(knapsack[j + 1][w] - knapsack[j][w]) > 1e-9) { best_to_i.push_back(all_j[j]); w -= statements.demand[all_j[j]]; }
        } else { curr_diff -= to_j[i].first; curr_dem -= statements.demand[to_j[i].second]; curr_to_j.pop_back(); }
    }

    if (best_diff < -1e-8 && (!best_to_i.empty() || !best_to_j.empty())) {
        for (int c : best_to_j) { chosen[c] = shop_j + 1; used_capacity[shop_i] -= statements.demand[c]; used_capacity[shop_j] += statements.demand[c]; }
        for (int c : best_to_i) { chosen[c] = shop_i + 1; used_capacity[shop_j] -= statements.demand[c]; used_capacity[shop_i] += statements.demand[c]; }
        return true;
    }
    return false;
}

void SolutionCase::Solve() {
    fill(opened.begin(), opened.end(), 0); fill(chosen.begin(), chosen.end(), 0); fill(used_capacity.begin(), used_capacity.end(), 0); valid = true;
    vector<double> dist_to_closest_shop(statements.m);
    for (int i = 0; i < statements.m; i++) {
        double best_dist = 1e18;
        for (int j = 0; j < statements.n; j++) if (!banned[j]) best_dist = min(best_dist, statements.customer_to_shop_dist[j][i] + statements.cost[j]);
        dist_to_closest_shop[i] = best_dist;
    }
    int assigned = 0;
    while (assigned < statements.m) {
        int best_shop = -1; vector<int> best_customers; double best_coef = 1e18;
        for (int i = 0; i < statements.n; i++) {
            if (opened[i] || banned[i]) continue;
            vector<pair<double, int>> cand;
            for (int j = 0; j < statements.m; j++) if (chosen[j] == 0) cand.push_back({statements.customer_to_shop_dist[i][j], j});
            sort(cand.begin(), cand.end());
            vector<int> curr_chosen; long long curr_cap = statements.capacity[i]; double curr_dist_sum = 0;
            for (auto& it : cand) {
                if (curr_cap < (long long)statements.demand[it.second]) continue;
                if (it.first > dist_to_closest_shop[it.second] + 1e-7) continue;
                curr_chosen.push_back(it.second); curr_cap -= statements.demand[it.second]; curr_dist_sum += it.first;
            }
            if (curr_chosen.empty()) continue;
            double curr_coef = (curr_dist_sum + statements.cost[i]) / curr_chosen.size();
            if (curr_coef < best_coef) { best_coef = curr_coef; best_shop = i; best_customers = curr_chosen; }
        }
        if (best_shop == -1) break;
        assigned += (int)best_customers.size(); opened[best_shop] = 1;
        for (int j : best_customers) { chosen[j] = best_shop + 1; used_capacity[best_shop] += statements.demand[j]; }
    }
    for (int j = 0; j < statements.m; j++) {
        if (chosen[j] == 0) {
            int best_s = -1; double min_d = 1e18;
            for (int i = 0; i < statements.n; i++) {
                if (banned[i]) continue;
                if (used_capacity[i] + (long long)statements.demand[j] <= (long long)statements.capacity[i]) {
                    double d = statements.customer_to_shop_dist[i][j]; double total = opened[i] ? d : d + statements.cost[i];
                    if (total < min_d) { min_d = total; best_s = i; }
                }
            }
            if (best_s == -1) { valid = false; return; }
            opened[best_s] = 1; chosen[j] = best_s + 1; used_capacity[best_s] += statements.demand[j];
        }
    }
    FinalSync();
}

void apply_2opt_with_warmup(SolutionCase& sol, const Statements& st) {
    if (!sol.valid) return;
    vector<int> open_shops;
    for (int i = 0; i < st.n; ++i) if (sol.opened[i]) open_shops.push_back(i);
    if (open_shops.size() < 2) return;

    vector<pair<double, pair<int, int>>> pairs;
    for (int i = 0; i < (int)open_shops.size(); ++i) {
        for (int j = i + 1; j < (int)open_shops.size(); ++j) {
            pairs.push_back({dist2(st.shop[open_shops[i]], st.shop[open_shops[j]]), {open_shops[i], open_shops[j]}});
        }
    }

    for (int p = 0; p < MAX_PASSES; ++p) {
        bool improved = false;
        mt19937 rng(42 + p);
        int random_iters = (int)(TOTAL_TWO_OPT_ITERS * RANDOM_RATIO);
        uniform_int_distribution<int> dist(0, (int)open_shops.size() - 1);
        for (int i = 0; i < random_iters; ++i) {
            int idx_i = dist(rng), idx_j = dist(rng);
            while (idx_i == idx_j) idx_j = dist(rng);
            if (sol.TwoOpt(open_shops[idx_i], open_shops[idx_j])) improved = true;
        }
        sort(pairs.begin(), pairs.end());
        int limit = min((int)pairs.size(), TOTAL_TWO_OPT_ITERS - random_iters);
        for (int k = 0; k < limit; ++k) {
            if (sol.TwoOpt(pairs[k].second.first, pairs[k].second.second)) improved = true;
        }
        
        if (!improved) break;
    }
    sol.FinalSync();
}

int main() {
    ios::sync_with_stdio(false); cin.tie(0);
    int n, m; if (!(cin >> n >> m)) return 0;
    vector<double> cost(n); vector<int> capacity(n); vector<pair<double, double>> shop(n);
    for (int i = 0; i < n; i++) cin >> cost[i] >> capacity[i] >> shop[i].first >> shop[i].second;
    vector<int> demand(m); vector<pair<double, double>> customer(m);
    for (int i = 0; i < m; i++) cin >> demand[i] >> customer[i].first >> customer[i].second;
    
    Statements st(n, m, cost, capacity, shop, demand, customer);
    SolutionCase best_case(st);
    apply_2opt_with_warmup(best_case, st);

    vector<double> ratings = st.GetShopRatings();
    vector<pair<double, int>> ranked_open;
    for (int i = 0; i < n; i++) if (best_case.opened[i]) ranked_open.push_back({ratings[i], i});
    sort(ranked_open.rbegin(), ranked_open.rend());

    for (int i = 0; i < min(NUMBER_OF_TRIES, (int)ranked_open.size()); ++i) {
        vector<int> banned(n, 0); banned[ranked_open[i].second] = 1;
        SolutionCase candidate(st, banned);
        if (candidate.valid) {
            apply_2opt_with_warmup(candidate, st);
            if (candidate.answer < best_case.answer) best_case = candidate;
        }
    }

    cout << fixed << setprecision(20) << best_case.answer << endl;
    bool first = true;
    for (int i = 0; i < n; ++i) if (best_case.opened[i]) { if (!first) cout << " "; cout << i + 1; first = false; }
    cout << endl;
    for (int i = 0; i < m; ++i) cout << best_case.chosen[i] << (i == m - 1 ? "" : " ");
    cout << endl;
    return 0;
}