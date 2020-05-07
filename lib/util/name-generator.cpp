#include <cassert>
#include <cinttypes>
#include <gta3sc/util/name-generator.hpp>

namespace gta3sc::util
{
NameGenerator::NameGenerator() : NameGenerator("")
{}

NameGenerator::NameGenerator(std::string prefix) :
    prefix_format(std::move(prefix))
{
    // TODO specify that prefix should not contain special
    // characters, and assert that here.
    this->prefix_format.append("%" PRIu32);
}

NameGenerator::NameGenerator(NameGenerator&& rhs) noexcept
{
    // not thread-safe
    counter.store(rhs.counter.exchange(0));
    this->prefix_format = std::move(rhs.prefix_format);
}

auto NameGenerator::operator=(NameGenerator&& rhs) noexcept -> NameGenerator&
{
    if(this == &rhs)
        return *this;

    // not thread-safe
    counter.store(rhs.counter.exchange(0));
    this->prefix_format = std::move(rhs.prefix_format);
    return *this;
}

void NameGenerator::generate(std::string& str)
{
    const auto id = counter.fetch_add(1, std::memory_order_relaxed);

    // NOLINTNEXTLINE: FIXME replace with std::format
    int result_size = snprintf(nullptr, 0, prefix_format.c_str(), id);
    assert(result_size > 0);

    str.resize(result_size + 1); // +1 to include null terminator
    // NOLINTNEXTLINE:  FIXME replace with std::format
    result_size = snprintf(&str[0], str.size(), prefix_format.c_str(), id);
    assert(result_size > 0
           && static_cast<size_t>(result_size + 1) == str.size());

    str.pop_back(); // exclude null terminator
}
} // namespace gta3sc::util
