#pragma once
#include <atomic>
#include <cstdint>
#include <string>

namespace gta3sc::util
{
/// Generates unique strings.
///
/// Each instance of this generates strings suffixed by an alphanumeric value
/// that is guaranteed to be unique for all invocation of `generate` in that
/// object instance.
///
/// This class is thread-safe (except for the move and assignment operators).
class NameGenerator
{
public:
    NameGenerator();

    /// Constructs a generator on which the generated strings have a common
    /// `prefix`.
    explicit NameGenerator(std::string prefix);

    NameGenerator(const NameGenerator&) = delete;
    NameGenerator& operator=(const NameGenerator&) = delete;

    NameGenerator(NameGenerator&& rhs) noexcept;
    NameGenerator& operator=(NameGenerator&& rhs) noexcept;

    ~NameGenerator() noexcept = default;

    /// Generates a unique string into `str`.
    void generate(std::string& str);

private:
    std::atomic_uint32_t counter{};
    std::string prefix_format;
};
} // namespace gta3sc::util
