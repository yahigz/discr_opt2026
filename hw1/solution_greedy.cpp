#include <algorithm>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

const int64_t INF = numeric_limits<int>::max();
const int MAX_CNT_CONSIDERING = 1;
const int RANDOM_SEED = 52;

mt19937 rnd(RANDOM_SEED);

double rev_x_squared(double x) {
    return 1.0 / (x * x * x);
}

vector<double> create_distr(const vector<double>& rel_cost, function<double(double)> f) {
    vector<double> distr(rel_cost.size());
    double sum = 0.0;
    for (size_t i = 0; i < rel_cost.size(); i++) {
        distr[i] = f(rel_cost[i]);
        sum += distr[i];
    }
    for (size_t i = 0; i < distr.size(); i++) {
        distr[i] /= sum;
    }
    return distr;
}

int select_ind_from_distr(const vector<double>& distr) {
    double r = rnd() / static_cast<double>(rnd.max());
    double cumulative = 0.0;
    for (size_t i = 0; i < distr.size(); i++) {
        cumulative += distr[i];
        if (r < cumulative) {
            return i;
        }
    }
    return distr.size() - 1;
}

void update_by_choosing(int selected_ind, vector<set<int>>& set_by_element, vector<bool>& chosen, set<pair<double, int>>& rel_cost_set, vector<double>& rel_cost, vector<set<int>>& sets, const vector<int64_t>& cost) {
    for (int element : sets[selected_ind]) {
        for (int set_ind : set_by_element[element]) {
            if (!chosen[set_ind]) {
                rel_cost_set.erase({rel_cost[set_ind], set_ind});
                sets[set_ind].erase(element);
                if (!sets[set_ind].empty()) {
                    rel_cost[set_ind] = static_cast<double>(cost[set_ind]) / sets[set_ind].size();
                    rel_cost_set.insert({rel_cost[set_ind], set_ind});
                }
            }
        }
    }
}

pair<int64_t, vector<bool>> solve_greedy(int n, int m, const vector<int64_t>& cost, vector<set<int>>& sets, vector<set<int>>& set_by_element) {
    vector<bool> chosen(m, false);
    int64_t total_cost = 0;
    set<pair<double, int>> rel_cost_set;
    vector<double> rel_cost(m);
    for (int i = 0; i < m; i++) {
        rel_cost[i] = static_cast<double>(cost[i]) / sets[i].size();
        rel_cost_set.insert({rel_cost[i], i});
    }

    int done = 0;
    while (done < n) {
        vector<double> tmp_rel_cost;
        vector<double> tmp_ind;
        auto it = rel_cost_set.begin();
        for (int i = 0; i < MAX_CNT_CONSIDERING && it != rel_cost_set.end(); i++, it++) {
            tmp_rel_cost.push_back(it->first);
            tmp_ind.push_back(it->second);
        }
        vector<double> distr = create_distr(tmp_rel_cost, rev_x_squared);
        int selected_ind = tmp_ind[select_ind_from_distr(distr)];
        chosen[selected_ind] = true;
        total_cost += cost[selected_ind];
        done += sets[selected_ind].size();
        rel_cost_set.erase({rel_cost[selected_ind], selected_ind});
        update_by_choosing(selected_ind, set_by_element, chosen, rel_cost_set, rel_cost, sets, cost);
    }
    return {total_cost, chosen};

}

int main() {
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
            sets[i].insert(element);
            set_by_element[element].insert(i);
        }
    }

    vector<bool> best_chosen;
    int64_t best_cost = INF;

    {
        auto tmp = solve_greedy(n, m, cost, sets, set_by_element);
        best_cost = tmp.first;
        best_chosen = tmp.second;
    }
    
    cout << best_cost << '\n';
    for (int i = 0; i < m; i++) {
        if (best_chosen[i]) {
            cout << i + 1 << ' ';
        }
    }
    cout << '\n';
    return 0;
}
