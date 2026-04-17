#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

using namespace std;

const int NUMBER_OF_TRIES = 15;
const int CONSIDERING_M = 10;
const double INF = numeric_limits<double>::infinity();

double dist2(pair<double, double> a, pair<double, double> b) {
    return (a.first - b.first) * (a.first - b.first) + (a.second - b.second) * (a.second - b.second);
}

struct Statements {
    int n;
    int m;
    vector<double> cost;
    vector<int> capacity;
    vector<pair<double, double>> shop;
    vector<int> demand;
    vector<pair<double, double>> customer;
    
    Statements(int n, int m, vector<double> cost, vector<int> capacity, vector<pair<double, double>> shop, vector<int> demand, vector<pair<double, double>> customer) : n(n), m(m), cost(cost), capacity(capacity), shop(shop), demand(demand), customer(customer) {}

    vector<double> GetShopRatings() const {
        vector<vector<int>> nearest_customers_by_shop(n);
        vector<vector<double>> dist_to_customers(n, vector<double>(m));
        vector<double> ratings(n);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                dist_to_customers[i][j] = sqrt(dist2(shop[i], customer[j]));
            }
            vector<pair<double, int>> curr_customers;
            for (int j = 0; j < m; ++j) {
                curr_customers.push_back({dist_to_customers[i][j], j});
            }
            sort(curr_customers.begin(), curr_customers.end());
            for (int j = 0; j < min(CONSIDERING_M, m); j++) {
                nearest_customers_by_shop[i].push_back(curr_customers[j].second);
            }
        }
        vector<double> best_distance(m, INF);
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                best_distance[i] = min(best_distance[i], dist_to_customers[j][i] + (double)cost[j] / min(CONSIDERING_M, m));
            }
        }
        for (int i = 0; i < n; ++i) {
            for (auto elem : nearest_customers_by_shop[i]) {
                double curr = dist_to_customers[i][elem] + (double)cost[i] / nearest_customers_by_shop[i].size();
                ratings[i] += curr / best_distance[elem];
            }
        }
        return ratings;
    }
};

struct SolutionCase{
    double answer;
    vector<int> opened;
    vector<int> chosen;
    vector<int> banned;
    Statements statements;
    bool valid;

    void Solve();

    SolutionCase(Statements statements, vector<int> banned = {}) : banned(banned), statements(statements) {
        if (this->banned.empty()) {
            this->banned.assign(statements.n, 0);
        }
        answer = 0;
        opened.resize(statements.n);
        chosen.resize(statements.m);
        valid = true;
        Solve();
    }

    vector<int> get_open_shops_by_rating() const {
        vector<double> ratings = statements.GetShopRatings();
        vector<pair<double, int>> ranked;
        for (int i = 0; i < statements.n; i++) {
            if (!opened[i]) {
                continue;
            }
            ranked.push_back({ratings[i], i});
        }
        sort(ranked.begin(), ranked.end(), [](const pair<double, int>& lhs, const pair<double, int>& rhs) {
            if (lhs.first != rhs.first) {
                return lhs.first > rhs.first;
            }
            return lhs.second < rhs.second;
        });
        vector<int> result;
        result.reserve(ranked.size());
        for (const auto& item : ranked) {
            result.push_back(item.second);
        }
        return result;
    }
};

void SolutionCase::Solve() {
    answer = 0;
    fill(opened.begin(), opened.end(), 0);
    fill(chosen.begin(), chosen.end(), 0);
    valid = true;

    vector<double> dist_to_closest_shop(statements.m);
    for (int i = 0; i < statements.m; i++) {
        double best_dist = 1e18;
        for (int j = 0; j < statements.n; j++) {
            if (banned[j]) {
                continue;
            }
            best_dist = min(best_dist, sqrt(dist2(statements.shop[j], statements.customer[i])) + statements.cost[j]);
        }
        dist_to_closest_shop[i] = best_dist;
    }

    int assigned = 0;
    while (assigned < statements.m) {
        int best_shop = -1;
        vector<int> best_customers;
        double best_cost = 1e18;
        double best_coef = 1e18;
        for (int i = 0; i < statements.n; i++) {
            if (opened[i] || banned[i]) {
                continue;
            }
            set<pair<double, int>> curr_customers;
            for (int j = 0; j < statements.m; j++) {
                if (chosen[j]) {
                    continue;
                }
                double curr_cost = dist2(statements.shop[i], statements.customer[j]);
                curr_customers.insert({curr_cost, j});
            }
            vector<int> curr_chosen;
            int curr_capacity = statements.capacity[i];
            double curr_cost = 0;
            for (auto it : curr_customers) {
                if (curr_capacity < statements.demand[it.second]) {
                    // continue;
                    continue;
                }
                if (sqrt(it.first) > dist_to_closest_shop[it.second]) {
                    continue;
                }
                curr_chosen.push_back(it.second);
                curr_capacity -= statements.demand[it.second];
                curr_cost += sqrt(it.first);
            }
            if (curr_chosen.empty()) {
                continue;
            }
            double curr_coef = (curr_cost + statements.cost[i]) / curr_chosen.size();
            if (curr_coef < best_coef) {
                best_coef = curr_coef;
                best_shop = i;
                best_customers = curr_chosen;
                best_cost = curr_cost + statements.cost[i];
            }
        }
        if (best_shop == -1 || best_customers.empty()) {
            valid = false;
            answer = INF;
            return;
        }
        assigned += best_customers.size();
        opened[best_shop] = 1;
        for (int j : best_customers) {
            chosen[j] = best_shop + 1;
        }
        answer += best_cost;
    }
}


int main() {
    int n;
    int m;
    cin >> n >> m;
    vector<double> cost(n);
    vector<int> capacity(n);
    vector<pair<double, double>> shop(n);
    for (int i = 0; i < n; i++) {
        cin >> cost[i] >> capacity[i] >> shop[i].first >> shop[i].second;
    }
    vector<int> demand(m);
    vector<pair<double, double>> customer(m);
    for (int i = 0; i < m; i++) {
        cin >> demand[i] >> customer[i].first >> customer[i].second;
    }
    Statements original_statements(n, m, cost, capacity, shop, demand, customer);
    SolutionCase best_case(original_statements);

    vector<int> banned(original_statements.n, 0);
    vector<int> order = best_case.get_open_shops_by_rating();
    int tries = min(NUMBER_OF_TRIES, (int)order.size());
    for (int i = 0; i < tries; ++i) {
        banned[order[i]] = 1;
        SolutionCase candidate_case(original_statements, banned);
        if (candidate_case.valid && candidate_case.answer < best_case.answer) {
            best_case = candidate_case;
        }
    }

    cout << fixed << setprecision(20) << best_case.answer << endl;
    for (int i = 0; i < original_statements.n; ++i) {
        if (best_case.opened[i]) {
            cout << i + 1 << " ";
        }
    }
    cout << endl;
    for (int i = 0; i < original_statements.m; ++i) {
        cout << best_case.chosen[i] << " ";
    }
    cout << endl;
    return 0;
}