#include <algorithm>
#include <chrono>
#include <deque>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

const int BORDER_FOR_GREEDY = 2000;
const int MAX_CNT_CONSIDERING = 5;
const int MAX_REPEATS = 6;
const int MAX_TO_CONSIDER_DELETED = 10;
const int64_t INF = numeric_limits<int>::max();
const int RANDOM_SEED = 52;
const string BIGGEST_COST = "biggest_cost";
const string LESS_FREQUENT_LEFT_BUILT = "less_frequent_left_built";

const string HEURISTIC = LESS_FREQUENT_LEFT_BUILT;

mt19937 rnd(RANDOM_SEED);

double rev_x_squared(double x) {
    return 1.0 / (x * x * x);
}

vector<double> create_distr(const vector<double>& rel_cost, function<double(double)> f);

int select_ind_from_distr(const vector<double>& distr);

void update_by_choosing(int selected_ind, vector<set<int>>& set_by_element, vector<bool>& chosen, set<pair<double, int>>& rel_cost_set, vector<double>& rel_cost, vector<set<int>>& sets, const vector<int64_t>& cost);

pair<int64_t, vector<bool>> solve_greedy(int n, int m, const vector<int64_t>& cost, vector<set<int>>& sets, vector<set<int>>& set_by_element);

struct Case {
    int64_t current_cost;
    set<int> remaining;
    set<int> chosen;
    vector<int> uncovered_count;
    set<pair<double, int>> rel_cost_set;
    vector<double> rel_cost;
    int64_t best_cost = INF;
};

Case find_best_cost_greedy(Case current_case, const vector<set<int>>& set_by_element, const vector<int64_t>& cost, const vector<set<int>>& sets) {
    current_case.best_cost = current_case.current_cost;
    while (!current_case.remaining.empty() && current_case.chosen.size() != sets.size()) {
        if (current_case.rel_cost_set.empty()) {
            current_case.best_cost = INF;
            return current_case;
        }
        auto it = current_case.rel_cost_set.begin();
        int set_to_add = it->second;
        current_case.rel_cost_set.erase(it);
        current_case.best_cost += cost[set_to_add];
        current_case.chosen.insert(set_to_add);
        for (int element : sets[set_to_add]) { 
            if (current_case.remaining.erase(element) == 0) {
                continue;
            }
            for (int set_ind : set_by_element[element]) {
                if (current_case.chosen.count(set_ind) == 0) {
                    current_case.rel_cost_set.erase({current_case.rel_cost[set_ind], set_ind});
                    current_case.uncovered_count[set_ind] -= 1;
                    if (current_case.uncovered_count[set_ind] > 0) {
                        current_case.rel_cost[set_ind] = static_cast<double>(cost[set_ind]) / current_case.uncovered_count[set_ind];
                        current_case.rel_cost_set.insert({current_case.rel_cost[set_ind], set_ind});
                    }
                }
            }
        }
    }
    if (!current_case.remaining.empty()) {
        current_case.best_cost = INF;
    }
    return current_case;
}

Case add_set_to_chosen(const Case& current_case, int set_to_add, const vector<set<int>>& set_by_element, const vector<int64_t>& cost, const vector<set<int>>& sets) {
    Case new_case = current_case;
    new_case.current_cost += cost[set_to_add];
    new_case.chosen.insert(set_to_add);
    new_case.rel_cost_set.erase({new_case.rel_cost[set_to_add], set_to_add});
    for (int element : sets[set_to_add]) {
        if (new_case.remaining.erase(element) == 0) {
            continue;
        }
        for (int set_ind : set_by_element[element]) {
            if (new_case.chosen.count(set_ind) == 0) {
                new_case.rel_cost_set.erase({new_case.rel_cost[set_ind], set_ind});
                new_case.uncovered_count[set_ind] -= 1;
                if (new_case.uncovered_count[set_ind] > 0) {
                    new_case.rel_cost[set_ind] = static_cast<double>(cost[set_ind]) / new_case.uncovered_count[set_ind];
                    new_case.rel_cost_set.insert({new_case.rel_cost[set_ind], set_ind});
                }
            }
        }
    }
    return new_case;

}

vector<Case> generate_cases(const Case& current_case, const vector<set<int>>& set_by_element, const vector<int64_t>& cost, const vector<int>& order, int index, const vector<set<int>>& sets) {
    if (index >= order.size()) {
        return {};
    }
    Case new_current_case = current_case;
    new_current_case.rel_cost_set.erase({new_current_case.rel_cost[order[index]], order[index]});
    new_current_case.best_cost = find_best_cost_greedy(new_current_case, set_by_element, cost, sets).best_cost;
    vector<Case> new_cases;
    if (new_current_case.best_cost != INF) {
        new_cases.emplace_back(std::move(new_current_case));
    }
    Case case_with_add = add_set_to_chosen(current_case, order[index], set_by_element, cost, sets);
    case_with_add.best_cost = find_best_cost_greedy(case_with_add, set_by_element, cost, sets).best_cost;
    if (case_with_add.best_cost != INF) {
        new_cases.emplace_back(std::move(case_with_add));
    }
    return new_cases;
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

    if (m > BORDER_FOR_GREEDY) {
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

    set<pair<double, int>> rel_cost_set;
    vector<double> rel_cost(m);
    vector<int> uncovered_count(m, 0);
    for (int i = 0; i < m; i++) {
        if (sets[i].empty()) {
            rel_cost[i] = numeric_limits<double>::infinity();
        } else {
            uncovered_count[i] = static_cast<int>(sets[i].size());
            rel_cost[i] = static_cast<double>(cost[i]) / uncovered_count[i];
            rel_cost_set.insert({rel_cost[i], i});
        }
    }

    vector<int> order(m);
    for (int i = 0; i < m; i++) {
        order[i] = i;
    }
    set<int> remaining;
    for (int i = 0; i < n; i++) {
        remaining.insert(i);
    }
    if (HEURISTIC == BIGGEST_COST) {
        sort(order.begin(), order.end(), [&](int a, int b) {
            return cost[a] > cost[b];
        });
    } else if (HEURISTIC == LESS_FREQUENT_LEFT_BUILT) {
        vector<int> elem_mapping(n);
        vector<int> elem_order(n);
        for (int i = 0; i < n; ++i) {
            elem_order[i] = i;
        }
        sort(elem_order.begin(), elem_order.end(), [&](int a, int b) {
            return set_by_element[a].size() < set_by_element[b].size();
        });
        for (int i = 0; i < n; ++i) {
            elem_mapping[elem_order[i]] = i;
        }
        vector<set<int>> new_set_by_element(n);
        for (int i = 0; i < m; ++i) {
            set<int> new_set;
            for (int element : sets[i]) {
                new_set.insert(elem_mapping[element]);
                new_set_by_element[elem_mapping[element]].insert(i);
            }
            sets[i] = std::move(new_set);
        }
        set_by_element = std::move(new_set_by_element);
        sort(order.begin(), order.end(), [&](int a, int b) {
            return *(sets[a].begin()) < *(sets[b].begin());
        });
    }
    deque<Case> cases;
    cases.push_back({0, remaining, {}, uncovered_count, rel_cost_set, rel_cost});
    Case ans_case = cases.front();
    int64_t ans_cost = INF;
    vector<int> ans_order = order;
    for (int i = 0; i < m && !cases.empty(); ++i) {
        deque<Case> new_cases;
        while (!cases.empty()) {
            Case current_case = cases.front();
            cases.pop_front();
            vector<Case> generated_cases = generate_cases(current_case, set_by_element, cost, order, i, sets);
            new_cases.insert(new_cases.end(), generated_cases.begin(), generated_cases.end());
        }
        sort(new_cases.begin(), new_cases.end(), [](const Case& a, const Case& b) {
            return a.best_cost < b.best_cost;
        });
        new_cases.resize(min(static_cast<size_t>(MAX_CNT_CONSIDERING), new_cases.size()));
        cases = new_cases;
        if (!cases.empty() && cases.front().best_cost < ans_cost) {
            ans_cost = cases.front().best_cost;
            ans_case = cases.front();
        }
    }
    ans_case = find_best_cost_greedy(ans_case, set_by_element, cost, sets);
    Case curr_ans_case = ans_case;
    int64_t curr_ans_cost = INF;
    for (int k = 0; k < MAX_REPEATS; ++k) {
        vector<bool> chosen(m, false);
        for (int set_ind : curr_ans_case.chosen) {
            chosen[set_ind] = true;
        }
        cases.push_back({0, remaining, {}, uncovered_count, rel_cost_set, rel_cost});
        curr_ans_case = cases.front();
        curr_ans_cost = INF;
        sort(order.begin(), order.end(), [&](int a, int b) {
            if (chosen[a] == chosen[b]) {
                return cost[a] > cost[b];
            }
            return chosen[a] < chosen[b];
        });
        for (int i = 0; i < m && !cases.empty(); ++i) {
            deque<Case> new_cases;
            while (!cases.empty()) {
                Case current_case = cases.front();
                cases.pop_front();
                vector<Case> generated_cases = generate_cases(current_case, set_by_element, cost, order, i, sets);
                new_cases.insert(new_cases.end(), generated_cases.begin(), generated_cases.end());
            }
            sort(new_cases.begin(), new_cases.end(), [](const Case& a, const Case& b) {
                return a.best_cost < b.best_cost;
            });
            new_cases.resize(min(static_cast<size_t>(MAX_CNT_CONSIDERING), new_cases.size()));
            cases = new_cases;
            if (!cases.empty() && cases.front().best_cost < curr_ans_cost) {
                curr_ans_cost = cases.front().best_cost;
                curr_ans_case = cases.front();
            }
        }
        curr_ans_case = find_best_cost_greedy(curr_ans_case, set_by_element, cost, sets);
        curr_ans_cost = curr_ans_case.best_cost;
        if (ans_cost > curr_ans_cost) {
            ans_cost = curr_ans_cost;
            ans_case = curr_ans_case;
            ans_order = order;
        }
    }
    vector<int> tmp_order = order;
    vector<int> place_in_order(m);
    for (int i = 0; i < m; ++i) {
        place_in_order[tmp_order[i]] = i;
    }
    sort(tmp_order.begin(), tmp_order.end(), [&](int a, int b) {
        if (ans_case.chosen.count(a) > 0 && ans_case.chosen.count(b) > 0) {
            return place_in_order[a] < place_in_order[b];
        }
        if (ans_case.chosen.count(a) > 0) {
            return false;
        }
        if (ans_case.chosen.count(b) > 0) {
            return true;
        }
        return cost[a] > cost[b];
    });
    for (int i = 0; i < m && i < MAX_TO_CONSIDER_DELETED; ++i) {
        int64_t sv_cost = cost[tmp_order[i]];
        cost[tmp_order[i]] = INF;
        sort(order.begin(), order.end(), [&](int a, int b) {
            if (a == tmp_order[i]) {
                return false;
            }
            if (b == tmp_order[i]) {
                return true;
            }
            return cost[a] > cost[b];
        });
        cases.push_back({0, remaining, {}, uncovered_count, rel_cost_set, rel_cost});
        curr_ans_case = cases.front();
        curr_ans_cost = INF;
        
        for (int i = 0; i < m && !cases.empty(); ++i) {
            deque<Case> new_cases;
            while (!cases.empty()) {
                Case current_case = cases.front();
                cases.pop_front();
                vector<Case> generated_cases = generate_cases(current_case, set_by_element, cost, order, i, sets);
                new_cases.insert(new_cases.end(), generated_cases.begin(), generated_cases.end());
            }
            sort(new_cases.begin(), new_cases.end(), [](const Case& a, const Case& b) {
                return a.best_cost < b.best_cost;
            });
            new_cases.resize(min(static_cast<size_t>(MAX_CNT_CONSIDERING), new_cases.size()));
            cases = new_cases;
            if (!cases.empty() && cases.front().best_cost < curr_ans_cost) {
                curr_ans_cost = cases.front().best_cost;
                curr_ans_case = cases.front();
            }
        }
        curr_ans_case = find_best_cost_greedy(curr_ans_case, set_by_element, cost, sets);
        curr_ans_cost = curr_ans_case.best_cost;
        if (ans_cost > curr_ans_cost) {
            ans_cost = curr_ans_cost;
            ans_case = curr_ans_case;
        }
        cost[tmp_order[i]] = sv_cost;
    }
    cout << ans_case.best_cost << endl;
    for (int set_ind : ans_case.chosen) {
        cout << set_ind + 1 << " ";
    }
    cout << endl;
    return 0;
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
