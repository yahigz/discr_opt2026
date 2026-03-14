#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

void perform_coloring(int v, int& cnt_colors, vector<int>& coloring, const vector<vector<int>>& gr, const vector<int>& heur) {
    if (cnt_colors == 0) {
        coloring[v] = cnt_colors++;
    } else {
        vector<bool> used(cnt_colors, false);
        for (auto j : gr[v]) {
            if (coloring[j] != -1) {
                used[coloring[j]] = true;
            }
        }
        int lst = 0;
        while (lst < cnt_colors && used[lst]) {
            ++lst;
        }
        if (lst == cnt_colors) {
            ++cnt_colors;
        }
        coloring[v] = lst;
    }
    vector<int> lst;
    for (auto j : gr[v]) {
        if (coloring[j] == -1) {
            lst.push_back(j);
        }
    }
    sort(lst.begin(), lst.end(), [&](int a, int b) {return heur[a] > heur[b];});
    for (auto j : lst) {
        perform_coloring(j, cnt_colors, coloring, gr, heur);
    }
}

pair<int, vector<int>> perform_greedy(int n, int m, const vector<vector<int>>& gr) {
    vector<int> coloring(n, -1);
    int cnt_colors = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        vector<int> heur(n);
        int best = -1;
        for (int i = 0; i < n; ++i) {
            if (coloring[i] != -1) {
                continue;
            }
            heur[i] = gr[i].size();
            for (int j : gr[i]) {
                heur[i] += gr[j].size();
            }
            if (best == -1 || heur[best] < heur[i]) {
                best = i;
            }
        }
        if (best == -1) {
            break;
        }
        changed = true;
        perform_coloring(best, cnt_colors, coloring, gr, heur);
    }
    return {cnt_colors, coloring};
}

int main() {
    int n, m;
    cin >> n >> m;
    vector<vector<int>> gr(n);
    for (int i = 0; i < m; ++i) {
        int u, v;
        cin >> u >> v;
        gr[u].push_back(v);
        gr[v].push_back(u);
    }
    auto ans = perform_greedy(n, m, gr);
    cout << ans.first << "\n";
    for (int i = 0; i < n; ++i) {
        cout << ans.second[i] + 1 << " ";
    }
    cout << "\n";
    return 0;
}
