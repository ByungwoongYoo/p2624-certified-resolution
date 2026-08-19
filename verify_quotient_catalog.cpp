#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int Q = 20;
using Counts = std::array<unsigned char, Q>;
constexpr std::array<int, 8> UNITS{1, 3, 7, 9, 11, 13, 17, 19};

struct TypeSpec {
    int zeros;
    int ones;
    int twos;
    int threes;
};

TypeSpec type_spec(const std::string& label) {
    if (label == "p0")   return {6, 14, 0, 0};
    if (label == "p1")   return {7, 12, 1, 0};
    if (label == "p2")   return {8, 10, 2, 0};
    if (label == "p3d")  return {9, 8, 3, 0};
    if (label == "p3t")  return {8, 11, 0, 1};
    if (label == "p4d")  return {10, 6, 4, 0};
    if (label == "p4td") return {9, 9, 1, 1};
    throw std::runtime_error("Unknown type: " + label);
}

Counts transform(const Counts& n, int unit, int shift) {
    Counts result{};
    for (int r = 0; r < Q; ++r) {
        result[(unit * r + shift) % Q] = n[r];
    }
    return result;
}

bool weighted_less(const Counts& a, const Counts& b) {
    for (int r = 0; r < Q; ++r) {
        const bool sa = a[r] != 0;
        const bool sb = b[r] != 0;
        if (sa != sb) return sa < sb;
    }
    return a < b;
}

Counts canonical(const Counts& n) {
    bool first = true;
    Counts best{};
    for (int unit : UNITS) {
        for (int shift = 0; shift < Q; ++shift) {
            const Counts candidate = transform(n, unit, shift);
            if (first || weighted_less(candidate, best)) {
                best = candidate;
                first = false;
            }
        }
    }
    return best;
}

Counts parse_line(const std::string& line) {
    if (line.size() != Q) {
        throw std::runtime_error("Catalog line is not 20 digits: " + line);
    }
    Counts n{};
    for (int i = 0; i < Q; ++i) {
        if (line[i] < '0' || line[i] > '3') {
            throw std::runtime_error("Invalid occupancy digit: " + line);
        }
        n[i] = static_cast<unsigned char>(line[i] - '0');
    }
    return n;
}

std::set<Counts> read_catalog(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Could not open catalog: " + path);
    std::set<Counts> result;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) result.insert(parse_line(line));
    }
    return result;
}

struct DirectEnumerator {
    Counts occupancy{};
    std::array<int, 4> remaining{};
    // quotient_distance[d] equals R_d for d=1..10. For d=10, each
    // antipodal unordered class pair contributes twice, exactly as in R_10.
    std::array<int, 11> quotient_distance{};
    std::set<Counts> representatives;
    std::uint64_t nodes = 0;
    std::uint64_t valid_raw_vectors = 0;

    bool can_place(int position, int value) const {
        if (value == 0) return true;
        std::array<int, 11> delta{};
        for (int previous = 0; previous < position; ++previous) {
            const int old = occupancy[previous];
            if (old == 0) continue;
            const int raw = position - previous;
            const int d = std::min(raw, Q - raw);
            delta[d] += (d == 10 ? 2 : 1) * value * old;
            if (quotient_distance[d] + delta[d] > 10) return false;
        }
        return true;
    }

    void place(int position, int value, int sign) {
        if (value != 0) {
            for (int previous = 0; previous < position; ++previous) {
                const int old = occupancy[previous];
                if (old == 0) continue;
                const int raw = position - previous;
                const int d = std::min(raw, Q - raw);
                quotient_distance[d] += sign * (d == 10 ? 2 : 1) * value * old;
            }
        }
        occupancy[position] = static_cast<unsigned char>(sign > 0 ? value : 0);
    }

    void dfs(int position) {
        ++nodes;
        if (position == Q) {
            ++valid_raw_vectors;
            representatives.insert(canonical(occupancy));
            return;
        }

        const int slots_left_after = Q - position - 1;
        for (int value = 0; value <= 3; ++value) {
            if (remaining[value] == 0 || !can_place(position, value)) continue;

            --remaining[value];
            const int needed_after = remaining[0] + remaining[1] +
                                     remaining[2] + remaining[3];
            if (needed_after == slots_left_after) {
                place(position, value, +1);
                dfs(position + 1);
                place(position, value, -1);
            }
            ++remaining[value];
        }
    }
};
}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "usage: verify_quotient_catalog <type> <catalog.txt>\n";
            return 2;
        }
        const std::string label = argv[1];
        const std::string catalog_path = argv[2];
        const TypeSpec spec = type_spec(label);
        const auto expected = read_catalog(catalog_path);

        DirectEnumerator enumerator;
        enumerator.remaining = {spec.zeros, spec.ones, spec.twos, spec.threes};
        const auto started = std::chrono::steady_clock::now();
        enumerator.dfs(0);
        const double seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();

        if (enumerator.representatives != expected) {
            std::cerr << "MISMATCH type=" << label
                      << " expected_orbits=" << expected.size()
                      << " direct_orbits=" << enumerator.representatives.size() << '\n';
            for (const auto& row : expected) {
                if (!enumerator.representatives.count(row)) {
                    std::cerr << "Missing from direct enumeration: ";
                    for (int x : row) std::cerr << x;
                    std::cerr << '\n';
                    break;
                }
            }
            for (const auto& row : enumerator.representatives) {
                if (!expected.count(row)) {
                    std::cerr << "Missing from catalog: ";
                    for (int x : row) std::cerr << x;
                    std::cerr << '\n';
                    break;
                }
            }
            return 1;
        }

        std::cout << "PASS type=" << label
                  << " raw_valid=" << enumerator.valid_raw_vectors
                  << " affine_orbits=" << enumerator.representatives.size()
                  << " dfs_nodes=" << enumerator.nodes
                  << " seconds=" << seconds << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
