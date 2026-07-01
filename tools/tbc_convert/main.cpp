#include "tessera/tessera.hpp"

#include <fstream>
#include <iostream>
#include <vector>

namespace {

std::vector<std::uint8_t> read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

bool write_file(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(output);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cout << "tbc_convert <input.tbc> <output> --json|--hex|--canonical\n";
        return 1;
    }

    const std::string input_path = argv[1];
    const std::string output_path = argv[2];
    const std::string format_flag = argv[3];

    const auto bytes = read_file(input_path);
    tessera::convert::ConvertOptions options{};
    if (format_flag == "--json") {
        options.format = tessera::convert::OutputFormat::JsonSummary;
    } else if (format_flag == "--hex") {
        options.format = tessera::convert::OutputFormat::HexDump;
    } else if (format_flag == "--canonical") {
        options.format = tessera::convert::OutputFormat::CanonicalBinary;
    } else {
        std::cerr << "unknown format\n";
        return 2;
    }

    if (auto converted = tessera::convert::convert_bytes(tessera::core::ByteSpan{bytes.data(), bytes.size()}, options); converted) {
        if (!write_file(output_path, *converted)) {
            std::cerr << "failed to write output\n";
            return 3;
        }
        return 0;
    }

    std::cerr << "conversion failed\n";
    return 4;
}
