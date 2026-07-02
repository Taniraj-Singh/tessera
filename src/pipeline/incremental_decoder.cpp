#include "tessera/pipeline/incremental_decoder.hpp"

#include "tessera/core/constants.hpp"

namespace tessera::pipeline {

core::Result<void> IncrementalDecoder::fail(core::Error error) {
    state_ = State::Failed;
    error_ = error;
    return error_;
}

core::Result<void> IncrementalDecoder::feed(core::ByteSpan chunk) {
    if (state_ == State::Failed) {
        return error_;
    }
    if (state_ == State::Complete) {
        return core::Ok();
    }
    buffer_.append_bytes(chunk);
    return advance();
}

core::Result<void> IncrementalDecoder::advance() {
    for (;;) {
        switch (state_) {
        case State::NeedHeader: {
            if (buffer_.size() < core::kHeaderSize) {
                return core::Ok();
            }
            auto header = header::parse(buffer_.span());
            if (!header) {
                return fail(header.error());
            }
            header_ = *header;
            state_ = State::NeedCatalogLength;
            break;
        }
        case State::NeedCatalogLength: {
            if (buffer_.size() < core::kHeaderSize + 4) {
                return core::Ok();
            }
            catalog_block_length_ = core::read_le_u32(buffer_.data() + core::kHeaderSize);
            if (catalog_block_length_ < 8 || catalog_block_length_ > core::kMaxBlockSize) {
                return fail(core::Error{core::ErrorCode::InvalidLength, "incremental catalog length invalid", core::kHeaderSize});
            }
            state_ = State::NeedCatalog;
            break;
        }
        case State::NeedCatalog: {
            if (buffer_.size() < core::kHeaderSize + catalog_block_length_) {
                return core::Ok();
            }
            const core::ByteSpan block{buffer_.data() + core::kHeaderSize, buffer_.size() - core::kHeaderSize};
            auto catalog = catalog::parse(block, kMaxIncrementalContainerSize);
            if (!catalog) {
                return fail(catalog.error());
            }

            std::uint64_t expected = static_cast<std::uint64_t>(core::kHeaderSize) + catalog_block_length_;
            for (const catalog::SectionEntry& entry : catalog->entries) {
                const std::uint64_t end = entry.offset + entry.length;
                if (end < entry.offset) {
                    return fail(core::Error{core::ErrorCode::SectionOutOfBounds, "incremental section overflow", 0});
                }
                if (end > expected) {
                    expected = end;
                }
            }
            if (expected > kMaxIncrementalContainerSize) {
                return fail(core::Error{core::ErrorCode::InvalidLength, "incremental container too large", 0});
            }
            expected_size_ = expected;
            state_ = State::NeedSections;
            break;
        }
        case State::NeedSections: {
            if (static_cast<std::uint64_t>(buffer_.size()) < expected_size_) {
                return core::Ok();
            }
            auto parsed = parse::parse_container(buffer_.span(), options_);
            if (!parsed) {
                return fail(parsed.error());
            }
            result_ = std::move(*parsed);
            state_ = State::Complete;
            return core::Ok();
        }
        case State::Complete:
        case State::Failed:
            return core::Ok();
        }
    }
}

core::Result<model::Container> IncrementalDecoder::take() {
    if (state_ == State::Failed) {
        return error_;
    }
    if (state_ != State::Complete) {
        return core::Error{core::ErrorCode::TruncatedInput, "incremental decode not complete", buffer_.size()};
    }
    return std::move(result_);
}

} // namespace tessera::pipeline
