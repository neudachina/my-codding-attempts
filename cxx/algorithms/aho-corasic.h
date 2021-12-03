// Комитет По Исследованию Бинарных Вирусов обнаружил, что некоторые последовательности единиц и нулей являются кодами вирусов. 
// Комитет изолировал набор кодов вирусов. Последовательность из единиц и нулей называется безопасной, 
// если никакой ее подотрезок (т.е. последовательность из соседних элементов) не является кодом вируса. 
// Сейчас цель комитета состоит в том, чтобы установить, существует ли бесконечная безопасная последовательность из единиц и нулей.

#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <map>

class Graph {
public:
    Graph() {
        graph_.resize(1);
    }

    void add_virus(std::vector<uint64_t> virus) {
        int v = 0; // сначал смотрим корень
        for (uint64_t bit : virus) {
            if (graph_[v].next.count(bit) == 0) {
                graph_[v].next[bit] = graph_.size();

                // добавляем нашу новую ноду в граф
                node n;
                n.parent = v;
                n.bit_parent = bit;
                graph_.push_back(std::move(n));

                // делаем переход в новую вершину
                v = graph_.size() - 1;
            } else {
                v = graph_[v].next[bit];
            }
        }
        graph_[v].term = true;
    }

    uint64_t suf(uint64_t v) {
        if (graph_[v].suf == -1) {
            if (v == 0 || graph_[v].parent == 0) {
                graph_[v].suf = 0;
            } else {
                graph_[v].suf = go(suf(graph_[v].parent), graph_[v].bit_parent);
            }
        }
        return graph_[v].suf;
    }

    uint64_t go(uint64_t v, uint64_t bit) {
        if (graph_[v].go.count(bit) == 0) {
            if (graph_[v].next.count(bit) != 0) {
                graph_[v].go[bit] = graph_[v].next[bit];
            } else {
                if (v == 0)
                    graph_[v].go[bit] = 0;
                else
                    graph_[v].go[bit] = go(suf(v), bit);
            }
        }
        return graph_[v].go[bit];
    }

    void count() {
        for (uint64_t i = 0; i < graph_.size(); ++i) {
            suf(i);
            go(i, 0);
            go(i, 1);
        }
    }

    uint64_t get_term(uint64_t v) {
        if (graph_[v].term_link == -1) {
            auto step = suf(v);
            while (step != 0 && !graph_[step].term) step = suf(step);
            graph_[v].term_link = step;
        }

        return graph_[v].term_link;
    }

    bool dfs(uint64_t v) {
        if (used[v] == 1) {
            used[v] == 0;
            return true;
        }

        if (used[v] == 2 || graph_[v].term || get_term(v) != 0) {
            return false;
        }

        used[v] = 1;
        bool result = dfs(go(v, 0)) || dfs(go(v, 1));
        used[v] = 2;
        return result;
    }

    std::string findcycle() {
        used.resize(graph_.size(), 0);

        if (dfs(0))
            return "TAK";
        else
            return "NIE";
    }


private:
    struct node {
        std::map<uint64_t, uint64_t> next;
        int64_t parent = -1;
        uint64_t bit_parent;
        bool term = false;
        int64_t suf = -1;
        std::map<uint64_t, uint64_t> go;
        int64_t term_link = -1;
    };

    std::vector<node> graph_;
    std::vector<uint64_t> used;
};

int main() {
    Graph g;

    uint64_t n;
    std::cin >> n;
    for (uint64_t i = 0; i < n; ++i) {
        std::string str;
        std::cin >> str;

        std::vector<uint64_t> virus;
        for (char c : str)
            virus.push_back(std::stoi(std::string{c}));
        g.add_virus(virus);
    }

    g.count();
    std::cout << g.findcycle() << '\n';
}
