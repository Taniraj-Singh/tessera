#include "tessera/tessera.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

bool write_file(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(output);
}

void print_usage() {
    std::cout << "tbc_archive list <archive.tba>\n"
                 "tbc_archive extract <archive.tba> <index> <out.tbc>\n"
                 "tbc_archive pack <out.tba> <name> <file.tbc> [<name> <file.tbc> ...]\n";
}

int command_list(const std::string& path) {
    const std::vector<std::uint8_t> bytes = read_file(path);
    auto reader = tessera::archive::ArchiveReader::open(tessera::core::ByteSpan{bytes.data(), bytes.size()});
    if (!reader) {
        std::cerr << "open failed: " << reader.error().message << '\n';
        return 2;
    }
    std::cout << reader->size() << " entries\n";
    for (const tessera::archive::ArchiveEntry& entry : reader->entries()) {
        std::cout << "  id=" << entry.id << " name=" << entry.name
                  << " offset=" << entry.frame_offset << " length=" << entry.frame_length << '\n';
    }
    return 0;
}

int command_extract(const std::string& path, std::size_t index, const std::string& out) {
    const std::vector<std::uint8_t> bytes = read_file(path);
    auto reader = tessera::archive::ArchiveReader::open(tessera::core::ByteSpan{bytes.data(), bytes.size()});
    if (!reader) {
        std::cerr << "open failed: " << reader.error().message << '\n';
        return 2;
    }
    auto container = reader->read_entry(index);
    if (!container) {
        std::cerr << "extract failed: " << container.error().message << '\n';
        return 3;
    }
    if (!write_file(out, *container)) {
        std::cerr << "write failed: " << out << '\n';
        return 4;
    }
    std::cout << "Wrote " << out << " (" << container->size() << " bytes)\n";
    return 0;
}

int command_pack(int argc, char** argv) {
    // argv[2] = out, then (name, file) pairs from argv[3].
    if (argc < 5 || (argc - 3) % 2 != 0) {
        print_usage();
        return 1;
    }
    tessera::archive::ArchiveWriter writer;
    std::uint16_t next_id = 1;
    for (int arg = 3; arg + 1 < argc; arg += 2) {
        const std::string name = argv[arg];
        const std::vector<std::uint8_t> bytes = read_file(argv[arg + 1]);
        auto container = tessera::parse::parse_container(tessera::core::ByteSpan{bytes.data(), bytes.size()}, {});
        if (!container) {
            std::cerr << "parse failed for " << argv[arg + 1] << ": " << container.error().message << '\n';
            return 5;
        }
        if (auto status = writer.add_container(name, next_id, *container); !status) {
            std::cerr << "add failed: " << status.error().message << '\n';
            return 6;
        }
        ++next_id;
    }
    auto built = writer.build();
    if (!built) {
        std::cerr << "build failed: " << built.error().message << '\n';
        return 7;
    }
    if (!write_file(argv[2], *built)) {
        std::cerr << "write failed: " << argv[2] << '\n';
        return 8;
    }
    std::cout << "Wrote " << argv[2] << " (" << built->size() << " bytes, " << writer.size() << " entries)\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage();
        return 1;
    }
    const std::string command = argv[1];
    if (command == "list") {
        return command_list(argv[2]);
    }
    if (command == "extract") {
        if (argc < 5) {
            print_usage();
            return 1;
        }
        return command_extract(argv[2], static_cast<std::size_t>(std::stoul(argv[3])), argv[4]);
    }
    if (command == "pack") {
        return command_pack(argc, argv);
    }
    print_usage();
    return 1;
}
