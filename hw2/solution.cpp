#include <algorithm>
#include <deque>
#include <iostream>
#include <numeric>
#include <queue>
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


struct BranchAndBoundItem {
    int index;
    int64_t cost;
    int64_t weight;
};

struct BranchAndBoundCase {
    int level;
    int parent;
    int64_t current_weight;
    int64_t current_cost;
    double best_cost;
    bool taken;
};

pair<int64_t, vector<int>> perform_branch_and_bound(int n, int64_t max_weight, const vector<int>& cost, const vector<int64_t>& weight) {
    vector<BranchAndBoundItem> items(n);
    for (int i = 0; i < n; ++i) {
        items[i] = {i, cost[i], weight[i]};
    }
    sort(items.begin(), items.end(), [](const BranchAndBoundItem& a, const BranchAndBoundItem& b) {
        return a.cost * b.weight > b.cost * a.weight;
    });

    vector<int64_t> prefix_weight(n + 1, 0);
    vector<int64_t> prefix_cost(n + 1, 0);
    for (int i = 0; i < n; ++i) {
        prefix_weight[i + 1] = prefix_weight[i] + items[i].weight;
        prefix_cost[i + 1] = prefix_cost[i] + items[i].cost;
    }

    auto calculate_bound = [&](int level, int64_t current_weight, int64_t current_cost) -> double {
        if (current_weight > max_weight) {
            return -1.0;
        }
        if (level >= n) {
            return current_cost;
        }
        int64_t target_weight = prefix_weight[level] + (max_weight - current_weight);
        int full_prefix = int(upper_bound(prefix_weight.begin() + level, prefix_weight.end(), target_weight) - prefix_weight.begin()) - 1;
        double bound = current_cost + double(prefix_cost[full_prefix] - prefix_cost[level]);
        if (full_prefix < n) {
            int64_t remaining_weight = target_weight - prefix_weight[full_prefix];
            bound += double(items[full_prefix].cost) * remaining_weight / items[full_prefix].weight;
        }
        return bound;
    };

    int64_t best_cost = 0;
    vector<int> best_answer;
    vector<int> greedy_answer;
    int64_t greedy_weight = 0;
    for (int i = 0; i < n; ++i) {
        if (greedy_weight + items[i].weight <= max_weight) {
            greedy_weight += items[i].weight;
            best_cost += items[i].cost;
            greedy_answer.push_back(items[i].index);
        }
    }
    best_answer = greedy_answer;
    for (int i = 0; i < n; ++i) {
        if (items[i].weight <= max_weight && items[i].cost > best_cost) {
            best_cost = items[i].cost;
            best_answer = {items[i].index};
        }
    }

    vector<BranchAndBoundCase> cases;
    cases.push_back({0, -1, 0, 0, calculate_bound(0, 0, 0), false});

    priority_queue<pair<double, int>> queue;
    queue.push({cases[0].best_cost, 0});
    int best_case_index = -1;

    auto restore_answer = [&](int case_index) {
        vector<int> answer;
        while (case_index != -1) {
            const auto& current_case = cases[case_index];
            if (current_case.taken) {
                answer.push_back(items[current_case.level - 1].index);
            }
            case_index = current_case.parent;
        }
        return answer;
    };

    while (!queue.empty()) {
        auto [bound, case_index] = queue.top();
        queue.pop();
        const auto current_case = cases[case_index];
        if (bound <= best_cost || current_case.level >= n) {
            continue;
        }

        const auto& current_item = items[current_case.level];

        if (current_case.current_weight + current_item.weight <= max_weight) {
            BranchAndBoundCase take_case{
                current_case.level + 1,
                case_index,
                current_case.current_weight + current_item.weight,
                current_case.current_cost + current_item.cost,
                0.0,
                true,
            };
            take_case.best_cost = calculate_bound(take_case.level, take_case.current_weight, take_case.current_cost);
            cases.push_back(take_case);
            int take_index = (int) cases.size() - 1;

            if (take_case.current_cost > best_cost) {
                best_cost = take_case.current_cost;
                best_case_index = take_index;
            }
            if (take_case.best_cost > best_cost) {
                queue.push({take_case.best_cost, take_index});
            }
        }

        BranchAndBoundCase skip_case{
            current_case.level + 1,
            case_index,
            current_case.current_weight,
            current_case.current_cost,
            0.0,
            false,
        };
        skip_case.best_cost = calculate_bound(skip_case.level, skip_case.current_weight, skip_case.current_cost);
        cases.push_back(skip_case);
        int skip_index = (int) cases.size() - 1;
        if (skip_case.best_cost > best_cost) {
            queue.push({skip_case.best_cost, skip_index});
        }
    }

    if (best_case_index != -1) {
        best_answer = restore_answer(best_case_index);
    }
    return {best_cost, best_answer};
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
        auto result = perform_branch_and_bound(n, max_weight, cost, weight);
        cout << result.first << endl;
        for (int i = (int) result.second.size() - 1; i >= 0; --i) {
            cout << result.second[i] + 1 << " ";
        }
    }
}