#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <gta3sc/filesystem/file-location.hpp>
#include <gta3sc/fwd.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace gta3sc::syntax
{
enum class Category : uint8_t;
} // namespace gta3sc::syntax

namespace gta3sc
{
/// Severity of a diagnostic.
///
/// The enumeration is ordered from least to most severe.
enum class DiagnosticSeverity : uint8_t
{
    info,
    warning,
    error
};

/// Describes a diagnostic.
///
/// Descriptors are essentially the unique identifier of a diagnostic.
///
/// Instances MUST be created in static (global) storage because they are stored
/// as pointers in a diagnostic and the descriptor must outlive the diagnostics.
class DiagnosticDescriptor
{
public:
    /// \param default_severity The severity of the diagnostic if not overriden
    /// by the compiler driver.
    /// \param title A short title for the diagnostic.
    /// \param message_format The message to be formatted (`std::format`-style)
    /// on diagnostic.
    DiagnosticDescriptor(DiagnosticSeverity default_severity,
                         std::string_view title,
                         std::string_view message_format) :
        default_severity_(default_severity),
        title_(title),
        message_format_(message_format)
    {}

    DiagnosticDescriptor(const DiagnosticDescriptor&) = delete;
    auto
    operator=(const DiagnosticDescriptor&) -> DiagnosticDescriptor& = delete;

    DiagnosticDescriptor(DiagnosticDescriptor&&) noexcept = delete;
    auto operator=(DiagnosticDescriptor&&) noexcept
            -> DiagnosticDescriptor& = delete;

    auto default_severity() const -> DiagnosticSeverity
    {
        return default_severity_;
    }
    auto title() const -> std::string_view { return title_; }
    auto message_format() const -> std::string_view { return message_format_; }

private:
    // TODO: normalize to m_ prefix or no suffix to match other domain objects
    DiagnosticSeverity default_severity_;
    std::string_view title_;
    std::string_view message_format_;
};

/// A diagnostic, such as a compiler error/warning, alongside location and
/// context.
struct Diagnostic
{
    using Arg = std::variant<int64_t, syntax::Category, std::string,
                             std::vector<std::string>>;

    /// The diagnostic descriptor.
    const DiagnosticDescriptor* descriptor;
    /// Location from where the diagnostic was reported.
    FileLoc location;
    /// Locations related to the diagnostic.
    std::vector<FileRange> ranges;
    /// Arguments for formatting the message.
    std::vector<Arg> args;

    Diagnostic(FileLoc location,
               const DiagnosticDescriptor& descriptor) noexcept :
        descriptor(&descriptor), location(location)
    {}

public:
    class Builder;
};

/// An abstract diagnostic handler.
///
/// Diagnostics are reported and treated through derived handlers. Once
/// reported, a diagnostic is passed to an abstract emit method that can treat
/// it however it wants. For example, it may ignore the error entirely or print
/// it into a output stream.
class DiagnosticHandler
{
public:
    DiagnosticHandler() noexcept = default;

    DiagnosticHandler(const DiagnosticHandler&) = delete;
    auto operator=(const DiagnosticHandler&) -> DiagnosticHandler& = delete;

    DiagnosticHandler(DiagnosticHandler&&) noexcept = default;
    auto
    operator=(DiagnosticHandler&&) noexcept -> DiagnosticHandler& = default;

    virtual ~DiagnosticHandler() = default;

    /// Sends a diagnostic upstream.
    virtual void emit(Diagnostic) = 0;

    /// Reports a diagnostic to this handler.
    ///
    /// Returns a diagnostic builder that will emit the diagnostic to this
    /// handler upon destruction.
    ///
    /// \example
    /// ```cpp
    /// handler.report(location, diagnostic1).range(range).args(...);
    /// // automatically emits the diagnostic1 to the handler
    /// handler.report(location, diagnostic2);
    /// // automatically emits the diagnostic2 to the handler
    /// ```
    auto report(FileLoc loc, const DiagnosticDescriptor& descriptor) noexcept
            -> Diagnostic::Builder;

    /// Same as \ref report(FileLoc, const DiagnosticDescriptor&) but
    /// taking the location from the given range and adding the range to the
    /// diagnostic.
    auto report(FileRange range,
                const DiagnosticDescriptor& descriptor) noexcept
            -> Diagnostic::Builder;
};

/// A diagnostic handler that sends the diagnostic to a function callback.
class CallbackDiagnosticHandler final : public DiagnosticHandler
{
public:
    /// Callback function prototype.
    using CallbackFunction = std::function<void(Diagnostic)>;

    CallbackDiagnosticHandler() noexcept = default;

    /// \param callback Function to send the diagnostic to.
    explicit CallbackDiagnosticHandler(CallbackFunction callback) noexcept :
        callback(std::move(callback))
    {}

    /// Sends a diagnostic upstream.
    void emit(Diagnostic diag) override { callback(std::move(diag)); }

    /// Sets a new callback to be called at diagnostic emission.
    void set_callback(CallbackFunction callback)
    {
        this->callback = std::move(callback);
    }

private:
    CallbackFunction callback;
};

/// Helpful builder for diagnostics.
///
/// Provides an easy and idiomatic way to build a \ref Diagnostic.
///
/// The builder can either be used to build a \ref Diagnostic directly or to
/// push it to a \ref DiagnosticHandler. In the former case, you just need to
/// call \ref build().
///
/// To target a \ref DiagnosticHandler, you must pass it in the constructor and
/// let the destructor push it to the handler. No need to call \ref build().
class Diagnostic::Builder
{
public:
    /// Constructor used to build a \ref Diagnostic directly.
    ///
    /// At the end of the chain you must call \ref build().
    Builder(FileLoc loc, const DiagnosticDescriptor& descriptor);

    /// Constructor used to build a \ref Diagnostic and push it to a \ref
    /// DiagnosticHandler.
    ///
    /// Don't call \ref build() at the end of the chain. Let the destructor push
    /// it to the handler.
    Builder(FileLoc loc, const DiagnosticDescriptor& descriptor,
            DiagnosticHandler& target);

    Builder(const Builder&) = delete;
    auto operator=(const Builder&) -> Builder& = delete;

    Builder(Builder&& other) noexcept;
    auto operator=(Builder&& other) noexcept -> Builder&;

    ~Builder();

    /// Builds the diagnostic and returns it.
    ///
    /// \note Not supported if the builder was constructed with a \ref
    /// DiagnosticHandler target.
    auto build() && -> Diagnostic;

    /// Adds a source range to provide more context to the diagnostic.
    auto range(FileRange range) && -> Builder&&;

    /// Adds an argument to the diagnostic.
    template<typename Arg, typename... Args>
    auto args(Arg&& arg, Args&&... args) && -> Builder&&
    {
        // auto args(Args&&... args) =>
        //  ((void)diag->args.push_back(convert(std::forward<Args>(args))), ...)
        //
        // The line you see above using fold expressions crashes MSVC, thus we
        // use recursion instead to append the arguments to the diagnostic.
        //
        /// TODO has it been fixed there?
        diag.args.push_back(convert(std::forward<Arg>(arg)));
        return std::move(*this).args(std::forward<Args>(args)...);
    }

    auto args() && -> Builder&&
    {
        // Sink of recursion
        return std::move(*this);
    }

private:
    template<typename T>
    static auto convert(T&& arg) -> Diagnostic::Arg;

private:
    DiagnosticHandler* target{}; ///< Optional handler that will receive the
                                 ///< diagnostic on build.
    Diagnostic diag;             ///< Diagnostic being constructed.
};

inline auto DiagnosticHandler::report(
        FileLoc loc,
        const DiagnosticDescriptor& descriptor) noexcept -> Diagnostic::Builder
{
    return Diagnostic::Builder(loc, descriptor, *this);
}

inline auto DiagnosticHandler::report(
        FileRange range,
        const DiagnosticDescriptor& descriptor) noexcept -> Diagnostic::Builder
{
    return report(range.begin, descriptor).range(range);
}

inline Diagnostic::Builder::Builder(FileLoc loc,
                                    const DiagnosticDescriptor& descriptor) :
    diag(loc, descriptor)
{}

inline Diagnostic::Builder::Builder(FileLoc loc,
                                    const DiagnosticDescriptor& descriptor,
                                    DiagnosticHandler& target) :
    Builder(loc, descriptor)
{
    this->target = &target;
}

inline Diagnostic::Builder::Builder(Builder&& other) noexcept :
    target(std::exchange(other.target, nullptr)), diag(std::move(other.diag))
{}

inline auto Diagnostic::Builder::operator=(Builder&& other) noexcept -> Builder&
{
    this->diag = std::move(other.diag);
    this->target = std::exchange(other.target, nullptr);
    return *this;
}

inline auto Diagnostic::Builder::build() && -> Diagnostic
{
    assert(this->target == nullptr);
    this->target = nullptr; // disable target on destructor
    return std::move(diag);
}

inline auto Diagnostic::Builder::range(FileRange range) && -> Builder&&
{
    diag.ranges.push_back(range);
    return std::move(*this);
}

template<typename T>
inline auto Diagnostic::Builder::convert(T&& arg) -> Diagnostic::Arg
{
    using Ty = std::decay_t<T>;
    if constexpr(std::is_same_v<Ty, std::string_view>)
    {
        return std::string(arg);
    }
    else if constexpr(std::is_same_v<Ty, std::vector<std::string_view>>)
    {
        std::vector<std::string> vec(arg.size());
        std::transform(arg.begin(), arg.end(), vec.begin(),
                       [](const auto& view) { return view; });
        return vec;
    }
    else if constexpr(std::is_integral_v<Ty>)
    {
        return Diagnostic::Arg(std::in_place_type_t<int64_t>(),
                               std::forward<T>(arg));
    }
    else
    {
        return std::forward<T>(arg);
    }
}
} // namespace gta3sc

// Generic diagnostics used across multiple modules
namespace gta3sc::diag
{
extern const DiagnosticDescriptor internal_compiler_error;
extern const DiagnosticDescriptor
        could_not_open_file; // %0 => string (filepath)
/// Unlike could_not_open_file, there could be multiple reasons for a file not
/// being able to be loaded.
extern const DiagnosticDescriptor
        could_not_load_file; // %0 => string (filename)
} // namespace gta3sc::diag
