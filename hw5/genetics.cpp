
#include <chrono>
#include <cmath>
#include <iomanip>  
#include <iostream>
#include <deque>
#include <queue>
#include <random>
#include <set>
#include <vector>

using namespace std;

double dist2(pair<double, double> a, pair<double, double> b) {
    return (a.first - b.first) * (a.first - b.first) + (a.second - b.second) * (a.second - b.second);
}

mt19937 rng(42);

struct Statements {
    int n;
    int m;
    vector<double> cost;
    vector<int> capacity;
    vector<pair<double, double>> shop;
    vector<int> demand;
    vector<pair<double, double>> customer;

    vector<vector<double>> customer_to_shop_dist;
    
    Statements(int n, int m, vector<double> cost, vector<int> capacity, vector<pair<double, double>> shop, vector<int> demand, vector<pair<double, double>> customer) : n(n), m(m), cost(cost), capacity(capacity), shop(shop), demand(demand), customer(customer), customer_to_shop_dist(n, vector<double>(m, 0)) {
        Preprocess();
    }

    void Preprocess();
};

const int NUMBER_OF_TRIES_FOR_INITIAL_SOLUTION = 1;
const int NUMBER_OF_TRIES = 5;
const int CONSIDERING_M = 10;
const double INF = 1e100;

vector<double> GetShopRatings(const Statements& statements) {
    int n = statements.n, m = statements.m;
    vector<vector<int>> nearest_customers_by_shop(n);
    vector<vector<double>> dist_to_customers(n, vector<double>(m));
    vector<double> ratings(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            dist_to_customers[i][j] = sqrt(dist2(statements.shop[i], statements.customer[j]));
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
            best_distance[i] = min(best_distance[i], dist_to_customers[j][i] + (double)statements.cost[j] / min(CONSIDERING_M, m));
        }
    }
    for (int i = 0; i < n; ++i) {
        for (auto elem : nearest_customers_by_shop[i]) {
            double curr = dist_to_customers[i][elem] + (double)statements.cost[i] / nearest_customers_by_shop[i].size();
            ratings[i] += curr / best_distance[elem];
        }
    }
    return ratings;
}

struct SolutionCase {
    double answer;
    vector<int> opened;
    vector<int> chosen;
    vector<int> banned;
    Statements statements;
    bool valid;

    void Solve();

    SolutionCase(const Statements& statements, vector<int> banned = {}) : banned(banned), statements(statements) {
        if (this->banned.empty()) {
            this->banned.assign(statements.n, 0);
        } else {
            vector<int> new_banned(statements.n, 0);
            for (auto shop : banned) {
                new_banned[shop] = 1;
            }
            swap(this->banned, new_banned);
        }
        answer = 0;
        opened.resize(statements.n);
        chosen.resize(statements.m);
        valid = true;
        Solve();
    }

    vector<int> get_open_shops_by_rating() const {
        vector<double> ratings = GetShopRatings(statements);
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
    fill(chosen.begin(), chosen.end(), -1);
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
                if (chosen[j] != -1) {
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
            chosen[j] = best_shop ;
        }
        answer += best_cost;
    }
}

SolutionCase solve_statements(const Statements& original_statements, bool initial_solution = false) {
    SolutionCase best_case(original_statements);
    vector<int> banned;
    vector<int> order = best_case.get_open_shops_by_rating();
    int tries = min(NUMBER_OF_TRIES, (int)order.size());
    if (initial_solution) {
        for (int i = 0; i < NUMBER_OF_TRIES_FOR_INITIAL_SOLUTION && i < tries; ++i) {
            int number_banned = uniform_int_distribution<int>(1, tries)(rng);
            banned.push_back(order[number_banned - 1]);
            SolutionCase candidate_case(original_statements, banned);
            if (candidate_case.valid && candidate_case.answer < best_case.answer) {
                best_case = candidate_case;
            }
        }
        return best_case;
    }

    for (int i = 0; i < tries; ++i) {
        banned.push_back(order[i]);
        SolutionCase candidate_case(original_statements, banned);
        if (candidate_case.valid && candidate_case.answer < best_case.answer) {
            best_case = candidate_case;
        }
    }
    return best_case;
}

struct GAParams {
    double mutation_rate = 0.23;
    int generations = 500;
    int population_size = 24;
    int tournament_size = 5;
    int tabu_tenure = 7;

    int max_ban = 3;
    int max_open = 3;

    int two_opt_iterations = 1000;
    double random_two_opt_ratio = 0.2;
};

enum MutationType {
    OPEN_SHOP,
    CLOSE_SHOP,
    REASSIGN_CUSTOMER
};

struct TabuCase {
    int shop;
    int customer;
    MutationType mutation_type;

    TabuCase(int shop, int customer, MutationType mutation_type) : shop(shop), customer(customer), mutation_type(mutation_type) {}

    bool operator<(const TabuCase& other) const {
        if (shop != other.shop) return shop < other.shop;
        if (customer != other.customer) return customer < other.customer;
        return mutation_type < other.mutation_type;
    }
};

struct Individual {
    vector<bool> opened;
    vector<int> chosen;
    deque<pair<TabuCase, int>> tabu_list;
    set<TabuCase> tabu_set;
    vector<int> used_capacity;

    const Statements& statements;

    vector<bool> strictly_opened;
    vector<bool> strictly_banned;
    
    double total_cost;
    bool valid;

    vector<int> unassigned_customers;

    Individual(const Statements& statements) : statements(statements), opened(statements.n), chosen(statements.m, -1), strictly_opened(statements.n), strictly_banned(statements.n), used_capacity(statements.n, 0), total_cost(0), valid(false) {}

    Individual(const Individual& other) : statements(other.statements), opened(other.opened), chosen(other.chosen), tabu_list(other.tabu_list), tabu_set(other.tabu_set), used_capacity(other.used_capacity), strictly_opened(other.strictly_opened), strictly_banned(other.strictly_banned), total_cost(other.total_cost), valid(other.valid), unassigned_customers(other.unassigned_customers) {}

    Individual& operator=(const Individual& other) {
        if (this != &other) {
            opened = other.opened;
            chosen = other.chosen;
            tabu_list = other.tabu_list;
            tabu_set = other.tabu_set;
            used_capacity = other.used_capacity;
            strictly_opened = other.strictly_opened;
            strictly_banned = other.strictly_banned;
            total_cost = other.total_cost;
            valid = other.valid;
            unassigned_customers = other.unassigned_customers;
        }
        return *this;
    }

    void UpToDate(int timer) {
        while (!tabu_list.empty() && tabu_list.front().second <= timer) {
            tabu_set.erase(tabu_list.front().first);
            tabu_list.pop_front();
        }
    }

    bool TwoOpt(int shop_i, int shop_j, int timer, int tabu_tenure);

    void CalculateTotalCost() {
        total_cost = 0;
        vector<bool> actually_opened(statements.n, false);
        vector<long long> current_caps(statements.n, 0);

        for (int j = 0; j < statements.m; ++j) {
            if (chosen[j] == -1) { valid = false; total_cost = INF; return; }
            int s = chosen[j];
            total_cost += statements.customer_to_shop_dist[s][j];
            current_caps[s] += statements.demand[j];
            actually_opened[s] = true;
        }

        for (int i = 0; i < statements.n; ++i) {
            if (current_caps[i] > statements.capacity[i]) { valid = false; total_cost = INF; return; }
            if (actually_opened[i]) total_cost += statements.cost[i];
            opened[i] = actually_opened[i];
            used_capacity[i] = (int)current_caps[i];
        }
        valid = true;
    }
};

bool operator<(const Individual& a, const Individual& b) {
    if (a.valid != b.valid) {
        return a.valid > b.valid;
    }
    return a.total_cost < b.total_cost;
}

struct GeneticsTask {
    int timer = 0;
    Statements statements;
    GAParams ga_params;
    vector<Individual> population;
    Individual best_individual;

    GeneticsTask(Statements statements, GAParams ga_params) : statements(statements), ga_params(ga_params), best_individual(statements) {}

    void Mutate(Individual& individual);

    Individual Crossover(const Individual& parent1, const Individual& parent2);

    void Resolve(Individual& individual);

    Individual TournamentSelection() {
        uniform_int_distribution<int> dist(0, ga_params.population_size - 1);
        Individual best = population[dist(rng)];
        for (int i = 1; i < ga_params.tournament_size; i++) {
            Individual curr = population[dist(rng)];
            if (curr.total_cost < best.total_cost) {
                best = curr;
            }
        }
        return best;
    }

    void Evolve();

    void PerformTwoOpt(Individual& individual);

    Individual CreateRandomInit();

    void InitPopulation() {
    population.clear();
    population.reserve(ga_params.population_size);

    for (int i = 0; i < ga_params.population_size; ++i) {
        Individual ind = CreateRandomInit();
        
        if (ind.valid) {
            PerformTwoOpt(ind);
        }

        population.push_back(ind);

        if (ind < best_individual) {
            best_individual = ind;
        }
    }
}
};

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
    GAParams ga_params;
    GeneticsTask genetics_task(original_statements, ga_params);

    genetics_task.InitPopulation();
    cerr << "Population initialized" << endl;
    for (int k = 0; k < ga_params.generations; ++k) {
        cerr << "Generation " << k + 1 << "/" << ga_params.generations << ", best cost: " << genetics_task.best_individual.total_cost << endl;
        genetics_task.Evolve();
        cerr << "Generation " << k + 1 << " completed, best cost: " << genetics_task.best_individual.total_cost << endl;
    }
    cout << fixed << setprecision(20) << genetics_task.best_individual.total_cost << endl;
    for (int i = 0; i < original_statements.n; ++i) {
        if (genetics_task.best_individual.opened[i]) {
            cout << i + 1 << " ";
        }
    }
    for (int i = 0; i < original_statements.m; ++i) {
        if (genetics_task.best_individual.chosen[i] != -1) {
            cout << genetics_task.best_individual.chosen[i] + 1 << endl;
        }
    }
}

void Statements::Preprocess() {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            customer_to_shop_dist[i][j] = sqrt(dist2(shop[i], customer[j]));
        }
    }
}

void GeneticsTask::Mutate(Individual& individual) {
    individual.UpToDate(timer);

    uniform_int_distribution<int> mutation_ban_cnt(1, 3);
    uniform_int_distribution<int> mutation_open_cnt(1, 3);

    int ban_cnt = mutation_ban_cnt(rng);
    int open_cnt = mutation_open_cnt(rng);

    vector<int> open_shops;
    for (int i = 0; i < statements.n; ++i) {
        if (individual.opened[i] && !individual.strictly_banned[i]) open_shops.push_back(i);
    }
    for (int i = 0; i < ban_cnt && !open_shops.empty(); i++) {
        bool success = false;
        while (!success && !open_shops.empty()) {
            uniform_int_distribution<int> shop_dist(0, (int)open_shops.size() - 1);
            int idx = shop_dist(rng);
            int shop = open_shops[idx];
            if (individual.strictly_banned[shop]) {
                open_shops.erase(open_shops.begin() + idx);
                continue;
            }
            if (individual.strictly_opened[shop]) {
                if (individual.tabu_set.count(TabuCase(shop, -1, CLOSE_SHOP))) {
                    open_shops.erase(open_shops.begin() + idx);
                    continue;
                }
            }
            individual.used_capacity[shop] = statements.capacity[shop];
            individual.tabu_list.push_back({TabuCase(shop, -1, CLOSE_SHOP), timer + ga_params.tabu_tenure});
            individual.tabu_set.insert(TabuCase(shop, -1, CLOSE_SHOP));
            individual.strictly_banned[shop] = true;
            individual.strictly_opened[shop] = false;
            individual.opened[shop] = false;
            open_shops.erase(open_shops.begin() + idx);
            success = true;
        }
    }

    vector<int> closed_shops;
    for (int i = 0; i < statements.n; ++i) {
        if (!individual.opened[i] && !individual.strictly_opened[i]) closed_shops.push_back(i);
    }
    for (int i = 0; i < open_cnt && !closed_shops.empty(); i++) {
        bool success = false;
        while (!success && !closed_shops.empty()) {
            uniform_int_distribution<int> shop_dist(0, (int)closed_shops.size() - 1);
            int idx = shop_dist(rng);
            int shop = closed_shops[idx];
            if (individual.strictly_opened[shop]) {
                closed_shops.erase(closed_shops.begin() + idx);
                continue;
            }
            if (individual.strictly_banned[shop]) {
                if (individual.tabu_set.count(TabuCase(shop, -1, OPEN_SHOP))) {
                    closed_shops.erase(closed_shops.begin() + idx);
                    continue;
                }
            }
            individual.used_capacity[shop] = 0;
            individual.tabu_list.push_back({TabuCase(shop, -1, OPEN_SHOP), timer + ga_params.tabu_tenure});
            individual.tabu_set.insert(TabuCase(shop, -1, OPEN_SHOP));
            individual.strictly_opened[shop] = true;
            individual.strictly_banned[shop] = false;
            individual.opened[shop] = true;
            closed_shops.erase(closed_shops.begin() + idx);
            success = true;
        }
    }

    set<int> banned_shops;
    for (int i = 0; i < statements.n; i++) {
        if (!individual.opened[i]) {
            banned_shops.insert(i);
        }
    }
    for (int i = 0; i < statements.m; i++) {
        if (individual.chosen[i] == -1) {
            individual.unassigned_customers.push_back(i);
        }
        if (individual.chosen[i] != -1 && banned_shops.count(individual.chosen[i])) {
            individual.chosen[i] = -1;
            individual.unassigned_customers.push_back(i);
        }
    }
}

Individual GeneticsTask::Crossover(const Individual& parent1, const Individual& parent2) {
    return parent1;
}

void GeneticsTask::Resolve(Individual& individual) {
    if (!individual.unassigned_customers.empty()) {
        vector<int> still_unassigned = individual.unassigned_customers;
        while (!still_unassigned.empty()) {
            vector<pair<double, int>> rating;
            for (auto elem : still_unassigned) {
                double best_distance = 1e18;
                for (int j = 0; j < statements.n; j++) {
                    if (!individual.opened[j]) continue;
                    best_distance = min(best_distance, statements.customer_to_shop_dist[j][elem]);
                }
                rating.push_back({best_distance, elem});
            }
            sort(rating.begin(), rating.end());
            vector<int> next_unassigned;
            for (auto elem : rating) {
                int customer = elem.second;
                int best_shop = -1;
                double best_distance = 1e18;
                for (int j = 0; j < statements.n; j++) {
                    if (!individual.opened[j]) continue;
                    if (statements.customer_to_shop_dist[j][customer] < best_distance && statements.capacity[j] - individual.used_capacity[j] >= statements.demand[customer]) {
                        best_distance = statements.customer_to_shop_dist[j][customer];
                        best_shop = j;
                    }
                }
                if (best_shop != -1) {
                    individual.chosen[customer] = best_shop;
                    individual.used_capacity[best_shop] += statements.demand[customer];
                } else {
                    next_unassigned.push_back(customer);
                }
            }
            still_unassigned = next_unassigned;
            if (still_unassigned.empty()) break;
            vector<int> shop_map, shop_revmap(statements.n, -1);
            for (int i = 0; i < statements.n; ++i) {
                if (!individual.strictly_banned[i] && !individual.opened[i]) {
                    shop_revmap[i] = shop_map.size();
                    shop_map.push_back(i);
                }
            }
            int n2 = shop_map.size();
            int m2 = still_unassigned.size();
            if (n2 > 0 && m2 > 0) {
                vector<double> cost2(n2);
                vector<int> capacity2(n2);
                vector<pair<double, double>> shop2(n2);
                for (int i = 0; i < n2; ++i) {
                    int orig = shop_map[i];
                    cost2[i] = statements.cost[orig];
                    capacity2[i] = statements.capacity[orig];
                    shop2[i] = statements.shop[orig];
                }
                vector<int> demand2(m2);
                vector<pair<double, double>> customer2(m2);
                for (int i = 0; i < m2; ++i) {
                    demand2[i] = statements.demand[still_unassigned[i]];
                    customer2[i] = statements.customer[still_unassigned[i]];
                }
                Statements reduced(n2, m2, cost2, capacity2, shop2, demand2, customer2);
                SolutionCase sol = solve_statements(reduced);
                for (int i = 0; i < n2; ++i) {
                    if (sol.opened[i]) {
                        int orig = shop_map[i];
                        individual.opened[orig] = true;
                    }
                }
                for (int i = 0; i < m2; ++i) {
                    if (sol.chosen[i] != -1) {
                        int orig_shop = shop_map[sol.chosen[i]];
                        int orig_cust = still_unassigned[i];
                        individual.chosen[orig_cust] = orig_shop;
                        individual.used_capacity[orig_shop] += statements.demand[orig_cust];
                    }
                }
                vector<int> really_unassigned;
                for (int i = 0; i < m2; ++i) {
                    if (sol.chosen[i] == -1) really_unassigned.push_back(still_unassigned[i]);
                }
                still_unassigned = really_unassigned;
            }
            if (!still_unassigned.empty()) {
                deque<pair<TabuCase, int>> temp_deque;
                bool ban_removed = false;
                while (!individual.tabu_list.empty()) {
                    auto last = individual.tabu_list.front();
                    individual.tabu_list.pop_front();
                    if (!ban_removed && last.first.mutation_type == CLOSE_SHOP) {
                        individual.strictly_banned[last.first.shop] = false;
                        ban_removed = true;
                    } else {
                        temp_deque.push_back(last);
                    }
                }
                for (auto it = temp_deque.rbegin(); it != temp_deque.rend(); ++it) {
                    individual.tabu_list.push_front(*it);
                }
                if (!ban_removed) {
                    break;
                }
            }
        }
        individual.unassigned_customers = still_unassigned;
    }
}

void GeneticsTask::Evolve() {
    vector<Individual> next_gen;
    next_gen.reserve(ga_params.population_size);
    next_gen.push_back(best_individual);

    while(next_gen.size() < ga_params.population_size) {
        Individual ind = TournamentSelection();
        if (uniform_real_distribution<double>(0, 1)(rng) < ga_params.mutation_rate) {
            Mutate(ind);
            Resolve(ind);
        }
        PerformTwoOpt(ind);
        next_gen.push_back(ind);
        if (ind < best_individual) best_individual = ind;
    }
    swap(population, next_gen);
    timer++;
}

void GeneticsTask::PerformTwoOpt(Individual& individual) {
    for (int i = 0; i < ga_params.two_opt_iterations * ga_params.random_two_opt_ratio; ++i) {
        vector<int> open_shops;
        for (int j = 0; j < statements.n; ++j) {
            if (individual.opened[j]) {
                open_shops.push_back(j);
            }
        }
        if (open_shops.size() < 2) {
            break;
        }
        uniform_int_distribution<int> shop_dist(0, (int)open_shops.size() - 1);
        int idx_i = shop_dist(rng);
        int idx_j = shop_dist(rng);
        while (idx_j == idx_i) {
            idx_j = shop_dist(rng);
        }
        int shop_i = open_shops[idx_i];
        int shop_j = open_shops[idx_j];
        if (individual.TwoOpt(shop_i, shop_j, timer, ga_params.tabu_tenure)) {
        }
    }
    vector<pair<double, pair<int, int>>> pairs;
    for (int i = 0; i < statements.n; ++i) {
        if (!individual.opened[i]) continue;
        for (int j = i + 1; j < statements.n; ++j) {
            if (!individual.opened[j]) continue;
            double d = sqrt(dist2(statements.shop[i], statements.shop[j]));
            pairs.push_back({d, {i, j}});
        }
    }
    sort(pairs.begin(), pairs.end());
    int limit = min((int)pairs.size(), ga_params.two_opt_iterations - (int)(ga_params.two_opt_iterations * ga_params.random_two_opt_ratio));
    for (int k = 0; k < limit; ++k) {
        int i = pairs[k].second.first;
        int j = pairs[k].second.second;
        individual.TwoOpt(i, j, timer, ga_params.tabu_tenure);
    }
}

bool Individual::TwoOpt(int shop_i, int shop_j, int timer, int tabu_tenure) {
    vector<pair<double, int>> to_j;
    vector<pair<double, int>> to_i;
    for (int i = 0; i < chosen.size(); i++) {
        if (chosen[i] == shop_i) {
            if (statements.customer_to_shop_dist[shop_j][i] < statements.customer_to_shop_dist[shop_i][i] && tabu_set.count(TabuCase(shop_i, i, REASSIGN_CUSTOMER)) == 0) {
                to_j.push_back({statements.customer_to_shop_dist[shop_j][i] - statements.customer_to_shop_dist[shop_i][i], i});
            }
        }
        if (chosen[i] == shop_j) {
            if (statements.customer_to_shop_dist[shop_i][i] < statements.customer_to_shop_dist[shop_j][i] && tabu_set.count(TabuCase(shop_j, i, REASSIGN_CUSTOMER)) == 0) {
                to_i.push_back({statements.customer_to_shop_dist[shop_i][i] - statements.customer_to_shop_dist[shop_j][i], i});
            }
        }
    }
    sort(to_j.begin(), to_j.end());
    sort(to_i.begin(), to_i.end());
    
    int capacity_i = statements.capacity[shop_i] - used_capacity[shop_i];
    int capacity_j = statements.capacity[shop_j] - used_capacity[shop_j];
    
    vector<int> all_i;
    vector<int> all_j;

    for (int i = 0; i < chosen.size(); ++i) {
        if (chosen[i] == shop_i) {
            all_i.push_back(i);
        }
        if (chosen[i] == shop_j) {
            all_j.push_back(i);
        }
    }

    double best_diff = 0;
    vector<int> best_to_j;
    vector<int> best_to_i;

    vector<int> curr_to_i;
    double curr_diff = 0;
    int curr_dem = 0;
    for (int i = 0; i < to_i.size(); ++i) {
        curr_to_i.push_back(to_i[i].second);
        curr_diff += to_i[i].first;
        curr_dem += statements.demand[to_i[i].second];

        vector<vector<double>> knapsack(all_i.size() + 1, vector<double>(capacity_j + curr_dem + 1, INF));
        knapsack[0][0] = 0;
        for (int j = 0; j < all_i.size(); ++j) {
            for (int w = 0; w <= curr_dem + capacity_j; ++w) {
                knapsack[j + 1][w] = min(knapsack[j + 1][w], knapsack[j][w]);
                if (w + statements.demand[all_i[j]] <= curr_dem + capacity_j) {
                    knapsack[j + 1][w + statements.demand[all_i[j]]] = min(knapsack[j + 1][w + statements.demand[all_i[j]]], knapsack[j][w] + statements.customer_to_shop_dist[shop_j][all_i[j]] - statements.customer_to_shop_dist[shop_i][all_i[j]]);
                }
            }
        }
        
        double best_local_diff = INF;
        int best_w = -1;
        for (int w = curr_dem - capacity_i; w <= curr_dem + capacity_j; ++w) {
            if (knapsack[all_i.size()][w] < best_local_diff) {
                best_local_diff = knapsack[all_i.size()][w];
                best_w = w;
            }
        }

        if (curr_diff + best_local_diff < best_diff) {
            best_diff = curr_diff + best_local_diff;
            best_to_i = curr_to_i;
            best_to_j.clear();
            int w = best_w;
            for (int j = (int)all_i.size() - 1; j >= 0; --j) {
                if (knapsack[j + 1][w] != knapsack[j][w]) {
                    best_to_j.push_back(all_i[j]);
                    w -= statements.demand[all_i[j]];
                }
            }
        } else {
            curr_diff -= to_i[i].first;
            curr_dem -= statements.demand[to_i[i].second];
            curr_to_i.pop_back();
        }
    }

    vector<int> curr_to_j;
    curr_diff = 0;
    curr_dem = 0;
    for (int i = 0; i < to_j.size(); ++i) {
        curr_to_j.push_back(to_j[i].second);
        curr_diff += to_j[i].first;
        curr_dem += statements.demand[to_j[i].second];

        vector<vector<double>> knapsack(all_j.size() + 1, vector<double>(capacity_i + curr_dem + 1, INF));
        knapsack[0][0] = 0;
        for (int j = 0; j < all_j.size(); ++j) {
            for (int w = 0; w <= curr_dem + capacity_i; ++w) {
                knapsack[j + 1][w] = min(knapsack[j + 1][w], knapsack[j][w]);
                if (w + statements.demand[all_j[j]] <= curr_dem + capacity_i) {
                    knapsack[j + 1][w + statements.demand[all_j[j]]] = min(knapsack[j + 1][w + statements.demand[all_j[j]]], knapsack[j][w] + statements.customer_to_shop_dist[shop_i][all_j[j]] - statements.customer_to_shop_dist[shop_j][all_j[j]]);
                }
            }
        }

        double best_local_diff = INF;
        int best_w = -1;
        for (int w = curr_dem - capacity_j; w <= curr_dem + capacity_i; ++w) {
            if (knapsack[all_j.size()][w] < best_local_diff) {
                best_local_diff = knapsack[all_j.size()][w];
                best_w = w;
            }
        }
        if (curr_diff + best_local_diff < best_diff) {
            best_diff = curr_diff + best_local_diff;
            best_to_j = curr_to_j;
            best_to_i.clear();
            int w = best_w;
            for (int j = all_j.size() - 1; j >= 0; --j) {
                if (knapsack[j + 1][w] != knapsack[j][w]) {
                    best_to_i.push_back(all_j[j]);
                    w -= statements.demand[all_j[j]];
                }
            }
        } else {
            curr_diff -= to_j[i].first;
            curr_dem -= statements.demand[to_j[i].second];
            curr_to_j.pop_back();
        }
    }

    
    if (best_diff < -1e-8 && (!best_to_i.empty() || !best_to_j.empty())) {
        for (int c : best_to_j) {
            chosen[c] = shop_j;
            used_capacity[shop_i] -= statements.demand[c];
            used_capacity[shop_j] += statements.demand[c];
            tabu_list.push_back({TabuCase(shop_i, c, REASSIGN_CUSTOMER), timer + tabu_tenure});
        }
        for (int c : best_to_i) {
            chosen[c] = shop_i;
            used_capacity[shop_j] -= statements.demand[c];
            used_capacity[shop_i] += statements.demand[c];
            tabu_list.push_back({TabuCase(shop_j, c, REASSIGN_CUSTOMER), timer + tabu_tenure});
        }
        return true;
    }
    return false;
}

Individual GeneticsTask::CreateRandomInit() {
    SolutionCase greedy = solve_statements(statements, true);
    Individual ind(statements);
    ind.valid = greedy.valid;
    ind.total_cost = greedy.answer;
    for (int i = 0; i < statements.n; ++i) {
        ind.opened[i] = greedy.opened[i];
    }
    for (int i = 0; i < statements.m; ++i) {
        ind.chosen[i] = greedy.chosen[i];
        if (greedy.chosen[i] != -1) {
            ind.used_capacity[greedy.chosen[i]] += statements.demand[i];
        }
    }
    return ind;
}