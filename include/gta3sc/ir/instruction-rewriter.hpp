#pragma once
#include <gta3sc/ir/instruction-visitor.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace gta3sc
{
// Base class for instruction rewriters.
//
// An instruction rewriter visits instructions and possibly produces a rewrite
// of said instruction as another chain of instructions.
template<typename IRType>
class InstructionRewriter
    : public InstructionVisitor<IRType, std::optional<LinkedIR<IRType>>>
{
public:
    /// Rewrites the \p ir line at \p it.
    ///
    /// If a rewrite is produced, puts the replacement chain in place
    /// of the original line and returns the iterator to the first replacement.
    ///
    /// Otherwise, if no rewrite is taken, the original iterator is returned.
    auto rewrite(LinkedIR<IRType>& ir,
                 typename LinkedIR<IRType>::iterator it) ->
            typename LinkedIR<IRType>::iterator;

    /// Walks \p ir, calling \ref rewrite on each line.
    void rewrite_each(LinkedIR<IRType>& ir);
};

/// Composes multiple instruction rewriters into one.
template<typename IRType, typename... Rewriters>
class CompositeInstructionRewriter final : public InstructionRewriter<IRType>
{
public:
    using Result = typename InstructionRewriter<IRType>::Result;

    CompositeInstructionRewriter(Rewriters... rewriters) :
        rewriters(std::move(rewriters)...)
    {
        static_assert((std::is_base_of_v<InstructionRewriter<IRType>,
                                         std::decay_t<Rewriters>>
                       && ...));
    }

    auto visit(const IRType& line) -> Result override;

private:
    template<size_t I>
    void visit_impl(const IRType*& current_line, Result& result);

private:
    std::tuple<Rewriters...> rewriters;
};
} // namespace gta3sc

namespace gta3sc
{
// Template Argument Deduction Guide.
template<typename FirstRewriter, typename... RestRewriters>
CompositeInstructionRewriter(FirstRewriter&&, RestRewriters&&...)
        -> CompositeInstructionRewriter<
                typename std::decay_t<FirstRewriter>::IRType,
                std::decay_t<FirstRewriter>, std::decay_t<RestRewriters>...>;

template<typename IRType>
auto InstructionRewriter<IRType>::rewrite(
        LinkedIR<IRType>& ir, typename LinkedIR<IRType>::iterator it) ->
        typename LinkedIR<IRType>::iterator
{
    auto replacement = this->visit(*it);
    if(!replacement)
        return it;

    if(replacement->empty())
        return ir.erase(it);

    auto first_replacement = replacement->begin();
    ir.splice(it, std::move(*replacement));
    ir.erase(it);
    return first_replacement;
}

template<typename IRType>
void InstructionRewriter<IRType>::rewrite_each(LinkedIR<IRType>& ir)
{
    auto it = ir.begin();
    while(it != ir.end())
    {
        auto next = rewrite(ir, it);

        // Truly advance only after reaching a fixed point.
        it = next != it ? next : std::next(it);
    }
}

template<typename IRType, typename... Rewriters>
auto CompositeInstructionRewriter<IRType, Rewriters...>::visit(
        const IRType& line) -> Result
{
    Result result{std::nullopt};
    const IRType* current_line = &line;
    visit_impl<0>(current_line, result);
    return result;
}

template<typename IRType, typename... Rewriters>
template<size_t I>
void CompositeInstructionRewriter<IRType, Rewriters...>::visit_impl(
        const IRType*& current_line, Result& result)
{
    if constexpr(I < sizeof...(Rewriters))
    {
        auto& rewriter = std::get<I>(rewriters);
        if(auto replacement = rewriter.visit(*current_line))
        {
            if(!result)
            {
                result = std::move(replacement);
            }
            else
            {
                auto next = result->erase(result->begin());
                result->splice(next, std::move(*replacement));
            }

            if(result->empty())
                return;

            current_line = &result->front();
        }
        visit_impl<I + 1>(current_line, result);
    }
}
} // namespace gta3sc
