#pragma once
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace gta3sc::test
{
/// Saves and restores an environment variable for the scope lifetime.
class ScopedEnvVar
{
public:
    /// \p value of `std::nullopt` removes the variable; otherwise sets it
    /// (including to an empty string).
    explicit ScopedEnvVar(std::string_view var_name,
                          std::optional<std::string> value) :
        name(var_name)
    {
        if(const char* prev_value = std::getenv(name.c_str()))
            previous = prev_value;

        apply(value);
    }

    ScopedEnvVar(const ScopedEnvVar&) = delete;
    auto operator=(const ScopedEnvVar&) -> ScopedEnvVar& = delete;
    ScopedEnvVar(ScopedEnvVar&&) = delete;
    auto operator=(ScopedEnvVar&&) -> ScopedEnvVar& = delete;

    ~ScopedEnvVar() { apply(previous); }

private:
    void apply(const std::optional<std::string>& value)
    {
#if defined(_WIN32)
        if(value)
            _putenv_s(name.c_str(), value->c_str());
        else
            // safe to use _putenv since the envvar is being removed.
            _putenv((name + "=").c_str());
#else
        if(value)
            setenv(name.c_str(), value->c_str(), 1);
        else
            unsetenv(name.c_str());
#endif
    }

private:
    std::string name;
    std::optional<std::string> previous;
};
} // namespace gta3sc::test
