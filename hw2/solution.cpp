#include <algorithm>
#include <deque>
#include <iostream>
#include <numeric>
#include <set>
#include <vector>

using namespace std;

const int MAX_CNT_CONSIDERING = 10;
const int BORDER_FOR_DP = 1000'000'000;

struct CustomCompare {
    bool operator()(const int& lhs, const int& rhs) const {
        return lhs > rhs;
    }
};

struct Case {
    int64_t best_cost;
    int64_t current_cost;
    int64_t current_weight;
    vector<int> items;
};

void complete_greedy(Case& c, const vector<int>& cost, const vector<int64_t>& weight, int64_t max_weight, int i, vector<int>& order) {
    if (i < 0 || i >= (int) order.size()) {
        return;
    }
    int64_t current_best_cost = c.current_cost;
    int64_t current_weight = c.current_weight;
    for (int j = i; j < (int) order.size(); ++j) {
        if (current_weight + weight[order[j]] <= max_weight) {
            current_best_cost += cost[order[j]];
            current_weight += weight[order[j]];
        }
    }
    c.best_cost = current_best_cost;
}

vector<Case> generate_new_cases(const Case& c, const vector<int>& cost, const vector<int64_t>& weight, int64_t max_weight, int i, vector<int>& order) {
    if (i < 0 || i >= (int) order.size()) {
        return {};
    }
    vector<Case> new_cases;
    Case complete_case = c;
    bool can_add = complete_case.current_weight + weight[order[i]] <= max_weight;
    if (complete_case.current_weight + weight[order[i]] <= max_weight) {
        complete_case.current_cost += cost[order[i]];
        complete_case.current_weight += weight[order[i]];
        complete_case.items.push_back(order[i]);
    }
    complete_greedy(complete_case, cost, weight, max_weight, i + 1, order);
    new_cases.push_back(complete_case);
    if (can_add) {
        Case not_complete_case = c;
        complete_greedy(not_complete_case, cost, weight, max_weight, i + 1, order);
        new_cases.push_back(not_complete_case);
    }
    return new_cases;
}

int main() {
    int n;
    int64_t max_weight;
    cin >> n >> max_weight;
    vector<int> cost(n);
    vector<int64_t> weight(n);
    for (int i = 0; i < n; ++i) {
        cin >> cost[i] >> weight[i];
    }
    if (n * max_weight < BORDER_FOR_DP) {
        vector<int64_t> dp(max_weight + 1, 0);
        vector<vector<int>> path(n + 1, vector<int>(max_weight + 1, 0));
        for (int i = 0; i < n; ++i) {
            for (int64_t w = max_weight; w >= weight[i]; --w) {
                if (dp[w] < dp[w - weight[i]] + cost[i]) {
                    path[i + 1][w] = 1;
                }
                dp[w] = max(dp[w], dp[w - weight[i]] + cost[i]);
            }
        }
        cout << dp[max_weight] << endl;
        vector<int> answer;
        int64_t w = max_weight;
        for (int i = n; i > 0; --i) {
            if (path[i][w]) {
                answer.push_back(i);
                w -= weight[i - 1];
            }
        }
        for (int i = (int) answer.size() - 1; i >= 0; --i) {
            cout << answer[i] << " ";
        }
    } else {
        vector<int> order(n);
        iota(order.begin(), order.end(), 0);
        sort(order.begin(), order.end(), [&](int i, int j) {
            return (int64_t) cost[i] * weight[j] > (int64_t) cost[j] * weight[i];
        });
        deque<Case> cases;
        Case initial_case{0, 0, 0, {}};
        complete_greedy(initial_case, cost, weight, max_weight, 0, order);
        cases.push_back(initial_case);
        Case answer_case = initial_case;
        int64_t answer_cost = 0;
        int answer_ind = 0;
        int i = 0;
        while (!cases.empty()) {
            deque<Case> new_cases;
            for (auto& c : cases) {
                vector<Case> generated_cases = generate_new_cases(c, cost, weight, max_weight, i, order);
                for (auto& new_case : generated_cases) {
                    new_cases.push_back(new_case);
                }
            }
            sort(new_cases.begin(), new_cases.end(), [](const Case& lhs, const Case& rhs) {
                return lhs.best_cost > rhs.best_cost;
            });
            new_cases.resize(min((int) new_cases.size(), MAX_CNT_CONSIDERING));
            cases = std::move(new_cases);
            if (cases.empty()) {
                break;
            }
            if (cases[0].best_cost > answer_cost) {
                answer_cost = cases[0].best_cost;
                answer_case = cases[0];
                answer_ind = i + 1;
            }
            ++i;
        }
        if (answer_ind < (int) order.size()) {
            complete_greedy(answer_case, cost, weight, max_weight, answer_ind, order);
        }
        cout << answer_case.current_cost << endl;
        for (int i = (int) answer_case.items.size() - 1; i >= 0; --i) {
            cout << answer_case.items[i] + 1 << " ";
        }
        cout << endl;
    }
}