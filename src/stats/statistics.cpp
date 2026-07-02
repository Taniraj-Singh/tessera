#include "tessera/stats/statistics.hpp"

#include "tessera/reflect/field_ref.hpp"
#include "tessera/visit/visitor.hpp"

namespace tessera::stats {

namespace {

class StatsVisitor final : public visit::Visitor {
public:
    explicit StatsVisitor(ContainerStats& stats) : stats_(&stats) {}

    void enter_section(const model::SectionData& section, std::size_t index) override {
        (void)index;
        current_ = &section;
        ++stats_->sections;
        const std::size_t rows = section.records.rows.size();
        stats_->total_rows += rows;
        if (rows > stats_->largest_section_rows) {
            stats_->largest_section_rows = rows;
        }
    }

    void visit_field(const schema::FieldDescriptor& field, std::size_t index) override {
        (void)index;
        ++stats_->total_fields;
        const auto slot = static_cast<std::size_t>(field.type);
        if (slot < stats_->type_counts.size()) {
            ++stats_->type_counts[slot];
        }
    }

    void visit_row(const records::RecordRow& row, std::size_t index) override {
        (void)index;
        if (current_ == nullptr) {
            return;
        }
        const std::size_t field_count = current_->schema.fields.size();
        for (std::size_t column = 0; column < row.values.size() && column < field_count; ++column) {
            const reflect::FieldRef ref(current_->schema.fields[column], row.values[column]);
            stats_->total_value_bytes += ref.encoded_size();
        }
    }

    void leave_section(const model::SectionData& section, std::size_t index) override {
        (void)section;
        (void)index;
        current_ = nullptr;
    }

private:
    ContainerStats* stats_;
    const model::SectionData* current_{nullptr};
};

} // namespace

ContainerStats collect(const model::Container& container) {
    ContainerStats stats{};
    StatsVisitor visitor(stats);
    visit::traverse(container, visitor);
    return stats;
}

} // namespace tessera::stats
