#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// TEST(suite, name) registers a case, CHECK reports and continues, REQUIRE returns from the
// case. The binary takes --suite, --filter, --list and --help.
namespace tf {
    namespace detail {
        template <typename T> std::string toText(const T& value) {
            if constexpr (std::is_same_v<T, bool>) {
                return value ? "true" : "false";
            } else if constexpr (std::is_enum_v<T>) {
                return std::to_string(static_cast<std::underlying_type_t<T>>(value));
            } else if constexpr (std::is_constructible_v<std::string_view, const T&>) {
                return '"' + std::string{std::string_view{value}} + '"';
            } else if constexpr (std::is_arithmetic_v<T>) {
                return std::to_string(value);
            } else {
                return "<?>";
            }
        }

        template <typename T> std::string toText(const std::optional<T>& value) {
            return value ? "optional(" + toText(*value) + ')' : "nullopt";
        }
    } // namespace detail

    class Context {
    public:
        bool check(bool condition, std::string_view expression, const char* file, int line);

        template <typename TLeft, typename TRight>
        bool checkEqual(
            const TLeft& left,
            const TRight& right,
            std::string_view leftText,
            std::string_view rightText,
            const char* file,
            int line
        ) {
            if (left == right) return true;
            reportFailure(
                std::string{leftText} + " == " + std::string{rightText} + "  (" +
                    detail::toText(left) + " vs " + detail::toText(right) + ')',
                file,
                line
            );
            return false;
        }

        void reportFailure(std::string_view what, const char* file, int line);
        [[nodiscard]] std::size_t failureCount() const noexcept { return m_failures; }

    private:
        std::size_t m_failures = 0;
    };

    using TTestFunction = void (*)(Context&);

    struct STestCase {
        std::string_view suite;
        std::string_view name;
        TTestFunction function;
    };

    class Registry {
    public:
        static Registry& instance();

        ~Registry() = default;

        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;
        Registry(Registry&&) = delete;
        Registry& operator=(Registry&&) = delete;

        void add(const STestCase& testCase);
        [[nodiscard]] const std::vector<STestCase>& cases() const noexcept { return m_cases; }

    private:
        Registry() = default;
        std::vector<STestCase> m_cases;
    };

    struct SRegistrar {
        SRegistrar(std::string_view suite, std::string_view name, TTestFunction function) {
            Registry::instance().add({suite, name, function});
        }
    };

    struct SOptions {
        std::string suite;
        std::string filter;
        bool listOnly = false;
        bool helpRequested = false;
    };

    void printUsage(const char* program, std::ostream& stream);
    bool parseOptions(int argc, char** argv, SOptions& out);
    int run(const SOptions& options);
} // namespace tf

#define TEST(suite, name)                                                                          \
    static void test_##suite##_##name(tf::Context& ctx_);                                          \
    static tf::SRegistrar reg_##suite##_##name{#suite, #name, &test_##suite##_##name};             \
    static void test_##suite##_##name(tf::Context& ctx_)

#define CHECK(expression) ctx_.check((expression), #expression, __FILE__, __LINE__)
#define CHECK_EQ(left, right) ctx_.checkEqual((left), (right), #left, #right, __FILE__, __LINE__)

#define REQUIRE(expression)                                                                        \
    do {                                                                                           \
        if (!CHECK(expression)) return;                                                            \
    } while (false)
#define REQUIRE_EQ(left, right)                                                                    \
    do {                                                                                           \
        if (!CHECK_EQ(left, right)) return;                                                        \
    } while (false)
