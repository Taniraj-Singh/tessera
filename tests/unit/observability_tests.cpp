#include "support/test_framework.hpp"

#include "tessera/config/config.hpp"
#include "tessera/diag/diagnostics.hpp"
#include "tessera/diag/trace.hpp"
#include "tessera/log/logger.hpp"

#include <string>

namespace tessera::test {

int run_observability_tests() {
    int before = g_failures;

    {
        diag::DiagnosticSink sink;
        sink.emit(diag::Severity::Info, "parse", "started", 0);
        sink.emit(diag::Severity::Error, "parse", "bad length", 42);
        TESSERA_EXPECT_EQ(sink.size(), std::size_t{2});
        TESSERA_EXPECT_EQ(sink.count(diag::Severity::Error), std::size_t{1});
        TESSERA_EXPECT_TRUE(sink.has_errors());
        TESSERA_EXPECT_TRUE(sink.format().find("bad length") != std::string::npos);

        validate::ValidationReport report;
        report.valid = false;
        report.diagnostics.push_back({core::ErrorCode::InvalidLength, "overflow", 7});
        diag::ingest(sink, report, "validate");
        TESSERA_EXPECT_EQ(sink.size(), std::size_t{3});
    }

    {
        diag::Tracer tracer(4);
        {
            diag::ScopedSpan outer(tracer, "outer");
            tracer.mark("checkpoint");
            {
                diag::ScopedSpan inner(tracer, "inner");
            }
        }
        TESSERA_EXPECT_EQ(tracer.depth(), std::uint32_t{0});
        // enter, mark, enter, exit, exit = 5 events into a capacity-4 ring.
        auto events = tracer.events();
        TESSERA_EXPECT_EQ(events.size(), std::size_t{4});
        // Ticks must be strictly increasing in the retained window.
        for (std::size_t i = 1; i < events.size(); ++i) {
            TESSERA_EXPECT_TRUE(events[i].tick > events[i - 1].tick);
        }
    }

    {
        log::Logger logger;
        log::BufferSink sink;
        logger.add_sink(sink);
        logger.set_level(log::Level::Warn);
        logger.info("dropped");
        logger.warn("kept");
        logger.error("kept too");
        TESSERA_EXPECT_EQ(sink.size(), std::size_t{2});
        TESSERA_EXPECT_TRUE(!logger.enabled(log::Level::Info));
        TESSERA_EXPECT_TRUE(logger.enabled(log::Level::Error));
    }

    {
        const std::string text = "# comment\nname = tessera\nthreads=4\nratio = 0.5\nverbose = true\n";
        auto config = config::Config::parse(text);
        TESSERA_EXPECT_TRUE(config.ok());
        TESSERA_EXPECT_EQ(config->get_string("name"), std::string{"tessera"});
        TESSERA_EXPECT_EQ(config->get_int("threads", 0), std::int64_t{4});
        TESSERA_EXPECT_TRUE(config->get_double("ratio", 0.0) > 0.49 && config->get_double("ratio", 0.0) < 0.51);
        TESSERA_EXPECT_TRUE(config->get_bool("verbose", false));
        TESSERA_EXPECT_EQ(config->get_int("missing", 99), std::int64_t{99});

        auto malformed = config::Config::parse("valid = 1\ngarbage line\n");
        TESSERA_EXPECT_TRUE(!malformed.ok());
    }

    return g_failures - before;
}

} // namespace tessera::test
