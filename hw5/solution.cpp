#include <iomanip>
#include <iostream>
#include <cmath>
#include <cstdio>
#include <set>
#include <vector>

using namespace std;

double dist2(pair<double, double> a, pair<double, double> b) {
    return (a.first - b.first) * (a.first - b.first) + (a.second - b.second) * (a.second - b.second);
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
    vector<double> dist_to_closest_shop(m);
    for (int i = 0; i < m; i++) {
        double best_dist = 1e18;
        for (int j = 0; j < n; j++) {
            best_dist = min(best_dist, sqrt(dist2(shop[j], customer[i])) + cost[j]);
        }
        dist_to_closest_shop[i] = best_dist;
    }
    double answer = 0;
    vector<int> opened(n);
    vector<int> chosen(m);
    int assigned = 0;
    while (assigned < m) {
        int best_shop = -1;
        vector<int> best_customers;
        double best_cost = 1e18;
        double best_coef = 1e18;
        for (int i = 0; i < n; i++) {
            if (opened[i]) {
                continue;
            }
            set<pair<double, int>> curr_customers;
            for (int j = 0; j < m; j++) {
                if (chosen[j]) {
                    continue;
                }
                double curr_cost = dist2(shop[i], customer[j]);
                curr_customers.insert({curr_cost, j});
            }
            vector<int> curr_chosen;
            int curr_capacity = capacity[i];
            double curr_cost = 0;
            for (auto it : curr_customers) {
                if (curr_capacity < demand[it.second] || sqrt(it.first) > dist_to_closest_shop[it.second]) {
                    // continue;
                    break;
                }
                curr_chosen.push_back(it.second);
                curr_capacity -= demand[it.second];
                curr_cost += sqrt(it.first);
            }
            if (curr_chosen.empty()) {
                continue;
            }
            double curr_coef = (curr_cost + cost[i]) / curr_chosen.size();
            if (curr_coef < best_coef) {
                best_coef = curr_coef;
                best_shop = i;
                best_customers = curr_chosen;
                best_cost = curr_cost + cost[i];
            }
        }
        assigned += best_customers.size();
        opened[best_shop] = 1;
        for (int j : best_customers) {
            chosen[j] = best_shop + 1;
        }
        answer += best_cost;
    }
    cout << fixed << setprecision(20) << answer << endl;
    for (int i = 0; i < n; ++i) {
        if (opened[i]) {
            cout << i + 1 << " ";
        }
    }
    cout << endl;
    for (int i = 0; i < m; ++i) {
        cout << chosen[i] << " ";
    }
    cout << endl;
    return 0;
}