#pragma once
#include <utility>

namespace gta3sc::util
{
struct VisitExpandTag
{
};
constexpr VisitExpandTag visit_expand{};

/// Invokes `vis`.
///
/// This is the base case for `visit`. See it for more details.
template<typename Visitor>
constexpr auto visit(Visitor&& vis)
{
    return (std::forward<Visitor>(vis))();
}

/// Equivalent to `visit(bound_vis, std::forward<OtherVariants>(other_vars)...)`
/// where `bound_vis` is `vis` with its first argument bound to `var_value`.
///
/// This is effectively a helper to implement `visit` for variant-like types.
/// The `visit` of the variant-like type calls this with its first variant
/// expanded to `var_value`. This function then (if needed) invokes `visit`
/// again (through argument-dependent lookup) to expand more variants.
template<typename Visitor, typename T, typename... OtherVariants>
constexpr auto visit(Visitor&& vis, VisitExpandTag /*unused*/, T&& var_value,
                     OtherVariants&&... other_vars)
{
    return visit(
            [&var_value, &vis](auto&&... other_vars_value) {
                return (std::forward<Visitor>(vis))(
                        std::forward<T>(var_value),
                        std::forward<decltype(other_vars_value)>(
                                other_vars_value)...);
            },
            std::forward<OtherVariants>(other_vars)...);
}
} // namespace gta3sc::util
