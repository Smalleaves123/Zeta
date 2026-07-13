#include "zeta/crc/crc32c.h"

#include <iomanip>
#include <iostream>

int main() {
    const auto checksum = zeta::ComputeCrc32c("123456789");
    std::cout << "crc32c(123456789)=0x"
              << std::hex << std::setw(8) << std::setfill('0') << checksum
              << "\n";
}
