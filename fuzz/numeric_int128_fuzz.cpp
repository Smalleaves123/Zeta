#include "zeta/numeric/int128.h"

#include <cstddef>
#include <cstdint>

namespace {

zeta::Uint128 ReadUint128(const std::uint8_t* data, std::size_t size,
                          std::size_t offset) {
    std::uint64_t hi = 0;
    std::uint64_t lo = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        hi <<= 8;
        if (offset + i < size) hi |= data[offset + i];
    }
    for (std::size_t i = 0; i < 8; ++i) {
        lo <<= 8;
        if (offset + 8 + i < size) lo |= data[offset + 8 + i];
    }
    return zeta::Uint128::FromPair(hi, lo);
}

zeta::Int128 ReadInt128(const std::uint8_t* data, std::size_t size,
                        std::size_t offset) {
    std::int64_t hi = 0;
    std::uint64_t lo = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        hi <<= 8;
        if (offset + i < size) hi |= static_cast<std::int64_t>(data[offset + i]);
    }
    for (std::size_t i = 0; i < 8; ++i) {
        lo <<= 8;
        if (offset + 8 + i < size) lo |= data[offset + 8 + i];
    }
    return zeta::Int128::FromPair(hi, lo);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    zeta::Uint128 ua = ReadUint128(data, size, 0);
    zeta::Uint128 ub = ReadUint128(data, size, 16);
    zeta::Int128 ia = ReadInt128(data, size, 0);
    zeta::Int128 ib = ReadInt128(data, size, 16);
    int shift = size > 32 ? static_cast<int>(data[32] % 128U) : 0;

    auto usum = ua + ub;
    auto udiff = ua - ub;
    auto uprod = ua * ub;
    auto uand = ua & ub;
    auto uor = ua | ub;
    auto uxor = ua ^ ub;
    auto ul = ua << shift;
    auto ur = ua >> shift;

    if ((usum - ub) != ua) __builtin_trap();
    if (((uand | uxor) != uor)) __builtin_trap();
    if ((ul >> shift) != ua && shift == 0) __builtin_trap();

    auto isum = ia + ib;
    auto idiff = ia - ib;
    auto iprod = ia * ib;
    auto iand = ia & ib;
    auto ior = ia | ib;
    auto ixor = ia ^ ib;
    auto il = ia << shift;
    auto ir = ia >> shift;

    if ((isum - ib) != ia) __builtin_trap();
    if (((iand | ixor) != ior)) __builtin_trap();
    if ((il >> shift) != ia && shift == 0) __builtin_trap();

    if (static_cast<bool>(ub)) {
        auto uq = ua / ub;
        auto urmd = ua % ub;
        if ((uq * ub + urmd) != ua) __builtin_trap();
    }
    if (static_cast<bool>(ib)) {
        auto iq = ia / ib;
        auto irmd = ia % ib;
        if ((iq * ib + irmd) != ia) __builtin_trap();
    }

    auto abs_a = ia.Abs();
    if ((ia == zeta::Int128(0)) != (!static_cast<bool>(abs_a))) __builtin_trap();

    volatile std::uint64_t sink =
        usum.Low64() ^ udiff.Low64() ^ uprod.Low64() ^ uand.Low64() ^
        uor.Low64() ^ uxor.Low64() ^ ur.Low64() ^ abs_a.Low64() ^
        static_cast<std::uint64_t>(isum.Low64()) ^
        static_cast<std::uint64_t>(idiff.Low64()) ^
        static_cast<std::uint64_t>(iprod.Low64()) ^
        static_cast<std::uint64_t>(il.Low64()) ^
        static_cast<std::uint64_t>(ir.Low64());
    (void)sink;
    return 0;
}
