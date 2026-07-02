#include "tessera/tessera.hpp"

#include <fstream>
#include <iostream>
#include <vector>

namespace {

std::vector<std::uint8_t> read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

void print_usage() {
    std::cout << "tbc_inspect <file.tbc> [--json|--validate|--stream|--stats]\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const std::string path = argv[1];
    const bool json_output = argc > 2 && std::string(argv[2]) == "--json";
    const bool validate_only = argc > 2 && std::string(argv[2]) == "--validate";
    const bool stream_mode = argc > 2 && std::string(argv[2]) == "--stream";
    const bool stats_mode = argc > 2 && std::string(argv[2]) == "--stats";

    tessera::mmap::MappedFile mapped{};
    tessera::core::ByteSpan input{};
    std::vector<std::uint8_t> fallback;

    if (auto opened = tessera::mmap::MappedFile::open(path); opened) {
        mapped = std::move(*opened);
        input = mapped.span();
    } else {
        fallback = read_file(path);
        input = tessera::core::ByteSpan{fallback.data(), fallback.size()};
    }

    if (validate_only) {
        const auto report = tessera::validate::validate_bytes(input, {});
        if (!report.valid) {
            for (const tessera::validate::Diagnostic& diagnostic : report.diagnostics) {
                std::cerr << static_cast<int>(diagnostic.code) << " @ " << diagnostic.offset << ": " << diagnostic.message << '\n';
            }
            return 2;
        }
        std::cout << "valid\n";
        return 0;
    }

    if (stream_mode) {
        tessera::stream::StreamReader reader(input);
        while (!reader.finished()) {
            if (auto event = reader.next(); event) {
                switch (event->kind) {
                case tessera::stream::StreamEventKind::Header:
                    std::cout << "header version=" << event->header.version << '\n';
                    break;
                case tessera::stream::StreamEventKind::Catalog:
                    std::cout << "catalog sections=" << event->catalog.entries.size() << '\n';
                    break;
                case tessera::stream::StreamEventKind::SectionBegin:
                    std::cout << "section id=" << event->section.id << " name=" << event->section.name << '\n';
                    break;
                case tessera::stream::StreamEventKind::Complete:
                    std::cout << "complete\n";
                    break;
                default:
                    break;
                }
            } else {
                std::cerr << event.error().message << '\n';
                return 3;
            }
        }
        return 0;
    }

    auto container = tessera::parse::parse_container(input, {});
    if (!container) {
        std::cerr << "parse failed: " << container.error().message << '\n';
        return 5;
    }

    if (stats_mode) {
        const tessera::stats::ContainerStats stats = tessera::stats::collect(*container);
        std::cout << "sections=" << stats.sections << " rows=" << stats.total_rows
                  << " fields=" << stats.total_fields << " value_bytes=" << stats.total_value_bytes
                  << " largest_section_rows=" << stats.largest_section_rows << '\n';
        return 0;
    }

    tessera::convert::ConvertOptions options{};
    options.format = json_output ? tessera::convert::OutputFormat::JsonSummary : tessera::convert::OutputFormat::JsonSummary;
    if (auto output = tessera::convert::convert_container(*container, options); output) {
        std::cout << *output;
        return 0;
    }
    std::cerr << "conversion failed\n";
    return 4;
}
