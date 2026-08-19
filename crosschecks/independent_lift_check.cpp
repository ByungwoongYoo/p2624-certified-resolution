#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int N = 100;
constexpr int Q = 20;
constexpr int F = 5;
constexpr int K = 14;
using Counts = std::array<unsigned char, Q>;
using Signature = std::array<unsigned char, 51>;

int circular_distance(int a, int b) {
    int d = std::abs(a - b);
    return std::min(d, N - d);
}

int capacity(int d) {
    return d == 50 ? 1 : 2;
}

std::array<std::vector<unsigned>, 4> make_mask_domains() {
    std::array<std::vector<unsigned>, 4> result;
    for (unsigned mask = 1; mask < (1u << F); ++mask) {
        int weight = __builtin_popcount(mask);
        if (weight <= 3) result[weight].push_back(mask);
    }
    return result;
}
const auto MASK_DOMAINS = make_mask_domains();

std::vector<int> points_for(int residue, unsigned mask) {
    std::vector<int> points;
    for (int q = 0; q < F; ++q) {
        if ((mask >> q) & 1u) points.push_back(residue + Q * q);
    }
    return points;
}

Signature internal_signature(int residue, unsigned mask) {
    Signature sig{};
    const auto points = points_for(residue, mask);
    for (std::size_t i = 0; i < points.size(); ++i) {
        for (std::size_t j = i + 1; j < points.size(); ++j) {
            ++sig[circular_distance(points[i], points[j])];
        }
    }
    return sig;
}

Signature cross_signature(int residue_a, unsigned mask_a,
                          int residue_b, unsigned mask_b) {
    Signature sig{};
    const auto a = points_for(residue_a, mask_a);
    const auto b = points_for(residue_b, mask_b);
    for (int x : a) {
        for (int y : b) {
            ++sig[circular_distance(x, y)];
        }
    }
    return sig;
}

bool verify_full_set(const std::vector<int>& selected) {
    if (selected.size() != K) return false;
    std::array<int, N> ordered{};
    for (int x : selected) {
        for (int y : selected) {
            if (x != y) ++ordered[(x - y + N) % N];
        }
    }
    return *std::max_element(ordered.begin() + 1, ordered.end()) <= 2;
}

struct Solver {
    Counts occupancy{};
    std::vector<int> residues;
    std::vector<std::vector<unsigned>> domains;
    std::vector<std::vector<Signature>> internal;
    // cross[i][j] is populated only for i<j. Its flattened index is
    // domain_index_i * domains[j].size() + domain_index_j.
    std::vector<std::vector<std::vector<Signature>>> cross;

    std::vector<int> assigned_domain;
    std::array<unsigned char, 51> used{};
    int assigned_count = 0;
    std::uint64_t nodes = 0;

    void prepare() {
        for (int r = 0; r < Q; ++r) {
            if (occupancy[r]) residues.push_back(r);
        }
        const int classes = static_cast<int>(residues.size());
        domains.resize(classes);
        internal.resize(classes);
        assigned_domain.assign(classes, -1);
        cross.resize(classes, std::vector<std::vector<Signature>>(classes));

        for (int i = 0; i < classes; ++i) {
            domains[i] = MASK_DOMAINS[occupancy[residues[i]]];
            internal[i].reserve(domains[i].size());
            for (unsigned mask : domains[i]) {
                internal[i].push_back(internal_signature(residues[i], mask));
            }
        }

        for (int i = 0; i < classes; ++i) {
            for (int j = i + 1; j < classes; ++j) {
                auto& table = cross[i][j];
                table.reserve(domains[i].size() * domains[j].size());
                for (unsigned mask_i : domains[i]) {
                    for (unsigned mask_j : domains[j]) {
                        table.push_back(cross_signature(
                            residues[i], mask_i, residues[j], mask_j));
                    }
                }
            }
        }
    }

    const Signature& cross_sig(int i, int di, int j, int dj) const {
        if (i < j) {
            return cross[i][j][di * domains[j].size() + dj];
        }
        return cross[j][i][dj * domains[i].size() + di];
    }

    bool build_delta(int variable, int domain_index, Signature& delta) const {
        delta = internal[variable][domain_index];
        for (int other = 0; other < static_cast<int>(residues.size()); ++other) {
            const int other_domain = assigned_domain[other];
            if (other_domain < 0) continue;
            const Signature& pair = cross_sig(
                variable, domain_index, other, other_domain);
            for (int d = 1; d <= 50; ++d) {
                delta[d] = static_cast<unsigned char>(delta[d] + pair[d]);
            }
        }
        for (int d = 1; d <= 50; ++d) {
            if (used[d] + delta[d] > capacity(d)) return false;
        }
        return true;
    }

    void apply(const Signature& delta, int sign) {
        for (int d = 1; d <= 50; ++d) {
            if (sign > 0) {
                used[d] = static_cast<unsigned char>(used[d] + delta[d]);
            } else {
                used[d] = static_cast<unsigned char>(used[d] - delta[d]);
            }
        }
    }

    bool dfs() {
        ++nodes;
        const int classes = static_cast<int>(residues.size());
        if (assigned_count == classes) return true;

        int best_variable = -1;
        std::vector<std::pair<int, Signature>> best_values;

        for (int variable = 0; variable < classes; ++variable) {
            if (assigned_domain[variable] >= 0) continue;
            std::vector<std::pair<int, Signature>> valid;
            valid.reserve(domains[variable].size());
            for (int di = 0; di < static_cast<int>(domains[variable].size()); ++di) {
                Signature delta{};
                if (build_delta(variable, di, delta)) {
                    valid.emplace_back(di, delta);
                }
            }
            if (valid.empty()) return false;
            if (best_variable < 0 || valid.size() < best_values.size()) {
                best_variable = variable;
                best_values = std::move(valid);
            }
        }

        for (const auto& [di, delta] : best_values) {
            assigned_domain[best_variable] = di;
            ++assigned_count;
            apply(delta, +1);
            if (dfs()) return true;
            apply(delta, -1);
            --assigned_count;
            assigned_domain[best_variable] = -1;
        }
        return false;
    }

    std::vector<int> witness() const {
        std::vector<int> result;
        for (int i = 0; i < static_cast<int>(residues.size()); ++i) {
            if (assigned_domain[i] < 0) continue;
            const unsigned mask = domains[i][assigned_domain[i]];
            const auto points = points_for(residues[i], mask);
            result.insert(result.end(), points.begin(), points.end());
        }
        std::sort(result.begin(), result.end());
        return result;
    }

    bool solve() {
        prepare();
        return dfs();
    }
};

Counts parse_line(const std::string& line) {
    if (line.size() != Q) throw std::runtime_error("Catalog line is not 20 digits");
    Counts result{};
    int total = 0;
    for (int i = 0; i < Q; ++i) {
        if (line[i] < '0' || line[i] > '3') {
            throw std::runtime_error("Invalid occupancy digit");
        }
        result[i] = static_cast<unsigned char>(line[i] - '0');
        total += result[i];
    }
    if (total != K) throw std::runtime_error("Catalog line does not sum to 14");
    return result;
}
}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: independent_lift_check <catalog.txt>\n";
            return 2;
        }
        const std::string path = argv[1];
        std::ifstream input(path);
        if (!input) throw std::runtime_error("Could not open catalog: " + path);

        std::vector<Counts> catalog;
        std::string line;
        while (std::getline(input, line)) {
            if (!line.empty()) catalog.push_back(parse_line(line));
        }
        if (catalog.empty()) throw std::runtime_error("Empty catalog");

        const auto started = std::chrono::steady_clock::now();
        std::uint64_t total_nodes = 0;
        for (std::size_t index = 0; index < catalog.size(); ++index) {
            Solver solver;
            solver.occupancy = catalog[index];
            const bool found = solver.solve();
            total_nodes += solver.nodes;
            if (found) {
                const auto witness = solver.witness();
                if (!verify_full_set(witness)) {
                    throw std::runtime_error("Internal witness verification failed");
                }
                std::cout << "SAT catalog=" << path << " config=" << index
                          << " nodes=" << solver.nodes << " set";
                for (int x : witness) std::cout << ' ' << x;
                std::cout << '\n';
                return 10;
            }
        }

        const double seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();
        std::cout << "UNSAT-INDEPENDENT catalog=" << path
                  << " configs=" << catalog.size()
                  << " nodes=" << total_nodes
                  << " seconds=" << seconds << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
