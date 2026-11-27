#include "Vrecord_comparator.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

struct Record {
    std::uint16_t category = 0x1234;
    std::array<std::uint8_t, 14> mpn{};
    std::array<double, 3> params{{1.0, 2.0, 3.0}};
    std::uint32_t sku = 0x89abcdef;
    std::array<std::uint8_t, 18> flags{};
    std::uint16_t supplier = 0x5678;
};

struct Query {
    std::uint16_t category = 0x1234;
    std::array<std::uint8_t, 14> mpn{};
    std::array<std::uint8_t, 14> mpn_mask{};
    std::array<double, 3> min{{0.0, 1.0, 2.0}};
    std::array<double, 3> max{{2.0, 3.0, 4.0}};
    std::uint32_t sku = 0x89abcdef;
    std::array<std::uint8_t, 18> flags_value{};
    std::array<std::uint8_t, 18> flags_mask{};
    std::uint16_t supplier = 0x5678;
};

struct Cycle {
    bool valid;
    Record record;
    Query query;
    std::string label;
};

struct Expected {
    bool valid = false;
    bool match = false;
    std::string label;
};

std::uint64_t double_bits(double value) {
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    static_assert(std::numeric_limits<double>::is_iec559,
                  "The testbench requires IEEE-754 float64 doubles");
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void store_le(std::array<std::uint8_t, 64>& bytes, std::size_t offset,
              std::uint64_t value, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8));
    }
}

std::array<std::uint8_t, 64> pack_record(const Record& record) {
    std::array<std::uint8_t, 64> bytes{};
    store_le(bytes, 0, record.supplier, 2);
    std::memcpy(bytes.data() + 2, record.flags.data(), record.flags.size());
    store_le(bytes, 20, record.sku, 4);
    for (std::size_t i = 0; i < record.params.size(); ++i) {
        store_le(bytes, 24 + i * 8, double_bits(record.params[i]), 8);
    }
    std::memcpy(bytes.data() + 48, record.mpn.data(), record.mpn.size());
    store_le(bytes, 62, record.category, 2);
    return bytes;
}

template <std::size_t N>
void pack_words(const std::uint8_t* bytes, std::size_t byte_count,
                VlWide<N>& words) {
    for (std::size_t i = 0; i < N; ++i) {
        words[i] = 0;
    }
    for (std::size_t i = 0; i < byte_count; ++i) {
        words[i / 4] |= static_cast<std::uint32_t>(bytes[i]) << ((i % 4) * 8);
    }
}

void set_u64(Vrecord_comparator& dut, const Query& query) {
    dut.param0_min = double_bits(query.min[0]);
    dut.param0_max = double_bits(query.max[0]);
    dut.param1_min = double_bits(query.min[1]);
    dut.param1_max = double_bits(query.max[1]);
    dut.param2_min = double_bits(query.min[2]);
    dut.param2_max = double_bits(query.max[2]);
}

bool golden_match(const Record& record, const Query& query) {
    if (record.category != query.category || record.sku != query.sku ||
        record.supplier != query.supplier) {
        return false;
    }

    for (std::size_t i = 0; i < record.mpn.size(); ++i) {
        if ((record.mpn[i] & query.mpn_mask[i]) !=
            (query.mpn[i] & query.mpn_mask[i])) {
            return false;
        }
    }

    for (std::size_t i = 0; i < record.params.size(); ++i) {
        if (!(record.params[i] >= query.min[i] &&
              record.params[i] <= query.max[i])) {
            return false;
        }
    }

    for (std::size_t i = 0; i < record.flags.size(); ++i) {
        if ((record.flags[i] & query.flags_mask[i]) !=
            (query.flags_value[i] & query.flags_mask[i])) {
            return false;
        }
    }
    return true;
}

void drive(Vrecord_comparator& dut, const Cycle& cycle) {
    const auto record_bytes = pack_record(cycle.record);
    pack_words(record_bytes.data(), record_bytes.size(), dut.record_data);
    pack_words(cycle.query.mpn.data(), cycle.query.mpn.size(), dut.mpn_query);
    pack_words(cycle.query.mpn_mask.data(), cycle.query.mpn_mask.size(),
               dut.mpn_mask);
    pack_words(cycle.query.flags_value.data(), cycle.query.flags_value.size(),
               dut.flags_value);
    pack_words(cycle.query.flags_mask.data(), cycle.query.flags_mask.size(),
               dut.flags_mask);
    dut.category_query = cycle.query.category;
    dut.sku_query = cycle.query.sku;
    dut.supplier_query = cycle.query.supplier;
    set_u64(dut, cycle.query);
    dut.in_valid = cycle.valid;
}

void tick(Vrecord_comparator& dut) {
    dut.clk = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
    dut.clk = 0;
    dut.eval();
}

Cycle matching_cycle(const std::string& label) {
    Cycle cycle{true, Record{}, Query{}, label};
    for (std::size_t i = 0; i < cycle.record.mpn.size(); ++i) {
        cycle.record.mpn[i] = static_cast<std::uint8_t>(0x30 + i);
        cycle.query.mpn[i] = cycle.record.mpn[i];
        cycle.query.mpn_mask[i] = 0xff;
    }
    cycle.record.flags[0] = 0xa5;
    cycle.query.flags_value[0] = 0xa5;
    cycle.query.flags_mask[0] = 0xff;
    cycle.record.flags[17] = 0x5a;
    cycle.query.flags_mask[17] = 0x0f;
    cycle.query.flags_value[17] = 0x0a;
    return cycle;
}

std::vector<Cycle> make_vectors() {
    std::vector<Cycle> cycles;

    cycles.push_back(matching_cycle("all fields match"));

    auto cycle = matching_cycle("masked MPN and flag differences are ignored");
    cycle.query.mpn_mask[0] = 0x0f;
    cycle.query.mpn[0] ^= 0xf0;
    cycle.record.flags[17] ^= 0xf0;
    cycles.push_back(cycle);

    cycle = matching_cycle("category mismatch");
    cycle.record.category ^= 1;
    cycles.push_back(cycle);

    cycle = matching_cycle("masked MPN mismatch");
    cycle.record.mpn[4] ^= 1;
    cycles.push_back(cycle);

    cycle = matching_cycle("parameter below inclusive range");
    cycle.record.params[0] = cycle.query.min[0] - 0.25;
    cycles.push_back(cycle);

    cycle = matching_cycle("parameter above inclusive range");
    cycle.record.params[2] = cycle.query.max[2] + 0.25;
    cycles.push_back(cycle);

    cycle = matching_cycle("SKU mismatch");
    cycle.record.sku ^= 1;
    cycles.push_back(cycle);

    cycle = matching_cycle("masked flag mismatch");
    cycle.record.flags[0] ^= 1;
    cycles.push_back(cycle);

    cycle = matching_cycle("supplier mismatch");
    cycle.record.supplier ^= 1;
    cycles.push_back(cycle);

    cycle = matching_cycle("float endpoints are inclusive");
    cycle.record.params = cycle.query.min;
    cycles.push_back(cycle);

    cycle = matching_cycle("NaN parameter does not match");
    cycle.record.params[1] = std::numeric_limits<double>::quiet_NaN();
    cycles.push_back(cycle);

    cycle = matching_cycle("invalid input bubble");
    cycle.valid = false;
    cycles.push_back(cycle);

    cycle = matching_cycle("match after bubble");
    cycles.push_back(cycle);

    return cycles;
}

}  // namespace

int main(int argc, char** argv) {
    VerilatedContext context;
    context.commandArgs(argc, argv);
    Vrecord_comparator dut{&context};

    dut.rst_n = 0;
    dut.in_valid = 0;
    tick(dut);
    tick(dut);
    dut.rst_n = 1;

    Expected stage1;
    std::size_t mismatches = 0;
    const auto cycles = make_vectors();

    for (const auto& cycle : cycles) {
        drive(dut, cycle);
        tick(dut);

        const bool actual_valid = dut.out_valid;
        const bool actual_match = dut.out_match;
        if (actual_valid != stage1.valid ||
            (actual_valid && actual_match != stage1.match)) {
            std::cerr << "Mismatch for \"" << stage1.label << "\": expected "
                      << "valid=" << stage1.valid << ", match=" << stage1.match
                      << "; got valid=" << actual_valid
                      << ", match=" << actual_match << '\n';
            ++mismatches;
        }

        stage1 = Expected{
            cycle.valid,
            cycle.valid && golden_match(cycle.record, cycle.query),
            cycle.label,
        };
    }

    dut.in_valid = 0;
    tick(dut);
    if (dut.out_valid != stage1.valid ||
        (dut.out_valid && dut.out_match != stage1.match)) {
        std::cerr << "Mismatch while draining the final pipeline result\n";
        ++mismatches;
    }
    tick(dut);
    if (dut.out_valid || dut.out_match) {
        std::cerr << "Pipeline did not clear after draining\n";
        ++mismatches;
    }

    dut.final();
    if (mismatches != 0) {
        std::cerr << mismatches << " mismatch(es) across " << cycles.size()
                  << " input cycles\n";
        return 1;
    }

    std::cout << "PASS: " << cycles.size()
              << " input cycles matched the C++ golden model\n";
    return 0;
}
