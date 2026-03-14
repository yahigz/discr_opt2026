#include <algorithm>
#include <iostream>
#include <set>
#include <vector>

using namespace std;


pair<int, vector<int>> perform_rlf(int n, int m, const vector<vector<int>>& gr) {
    vector<int> coloring(n, -1);
    set<pair<int, int>> st;
    vector<int> cnt_neigh(n);
    for (int i = 0; i < n; ++i) {
        cnt_neigh[i] = gr[i].size();
        st.insert({-gr[i].size(), i});
    }
    int cnt_colors = 0;
    while (!st.empty()) {
        int v = st.begin()->second;
        st.erase(st.begin());
        coloring[v] = cnt_colors++;
        bool changed = true;
        vector<int> cnt_neigh_to_s(n);
        set<pair<int, int>> st_s;
        vector<bool> spoiled(n, false);
        while (changed) {
            changed = false;
            for (auto j : gr[v]) {
                if (coloring[j] != -1) {
                    continue;
                }
                spoiled[j] = true;
                st.erase({-cnt_neigh[j], j});
                --cnt_neigh[j];
                st.insert({-cnt_neigh[j], j});
            }
            for (auto j : gr[v]) {
                if (coloring[j] != -1) {
                    continue;
                }
                for (auto k : gr[j]) {
                    if (coloring[k] != -1 || spoiled[k]) {
                        continue;
                    }
                    st_s.erase({-cnt_neigh_to_s[k], k});
                    ++cnt_neigh_to_s[k];
                    st_s.insert({-cnt_neigh_to_s[k], k});
                }
            }
            while (!st_s.empty()) {
                int candidate = st_s.begin()->second;
                if (coloring[candidate] == -1 && !spoiled[candidate]) {
                    break;
                }
                st_s.erase(st_s.begin());
            }
            int next_v = -1;
            if (!st_s.empty()) {
                next_v = st_s.begin()->second;
                st_s.erase(st_s.begin());
            } else {
                for (const auto& [neg_degree, candidate] : st) {
                    if (coloring[candidate] == -1 && !spoiled[candidate]) {
                        next_v = candidate;
                        break;
                    }
                }
            }
            if (next_v != -1) {
                v = next_v;
                st.erase({-cnt_neigh[v], v});
                coloring[v] = cnt_colors - 1;
                changed = true;
            }
        }
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
    auto ans = perform_rlf(n, m, gr);
    cout << ans.first << "\n";
    for (int i = 0; i < n; ++i) {
        cout << ans.second[i] + 1 << " ";
    }
    cout << "\n";
    return 0;
}
