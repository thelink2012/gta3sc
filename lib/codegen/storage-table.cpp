#include <functional>
#include <gta3sc/codegen/storage-table.hpp>

namespace gta3sc::codegen
{
/// Type used in index computations to avoid overflow.
using BigIndexType = uint32_t;
static_assert(sizeof(BigIndexType) > sizeof(LocalStorageTable::IndexType));

// An index that can never be reached. This is the case since
// `sizeof(BigIndexType) > sizeof(LocalStorageTable::IndexType)`.
constexpr BigIndexType impossible_index
        = std::numeric_limits<BigIndexType>::max();

/// Computes a lookup table to find variables from its id.
///
/// That is, `output` will be a vector such that indexing its i-th element
/// is equivalent to finding the variable `var` with `var->id() == i` in
/// `vars`.
static auto build_lookup_by_id(SymbolTable::VariableNamespaceView vars) noexcept
        -> std::vector<const SymbolTable::Variable*>;

/// Computes the number of storage indices taken by a given variable.
static auto num_indices_for_var(const SymbolTable::Variable& var) noexcept
        -> BigIndexType;

/// Computes the number of storage indices taken by a given variable type.
static auto num_indices_for_type(SymbolTable::VarType type) noexcept
        -> BigIndexType;

/// Unwraps the structure in `timer_opt` or returns impossible values.
static auto unwrap_timer_options(
        const std::optional<LocalStorageTable::TimerOptions>& timer_opt,
        const SymbolTable& symtable, SymbolTable::ScopeId scope_id) noexcept
        -> std::pair<BigIndexType, const SymbolTable::Variable*>;

/// Converts a `StorageTable::Options` to `LocalStorage::Options`.
static auto options_for_scope(SymbolTable::ScopeId scope_id,
                              const StorageTable::Options& options) noexcept
        -> LocalStorageTable::Options;

auto LocalStorageTable::var_index(
        const SymbolTable::Variable& var) const noexcept -> IndexType
{
    assert(var.id() < index_for_vars.size());
    return index_for_vars[var.id()];
}

auto LocalStorageTable::from_symbols(const SymbolTable& symtable,
                                     SymbolTable::ScopeId scope_id,
                                     const Options& options) noexcept
        -> std::optional<LocalStorageTable>
{
    LocalStorageTable storage;

    // Convert options from `IndexType` to `BigIndexType`.
    const BigIndexType first_storage_index = options.first_storage_index;
    const BigIndexType max_var_index = options.max_storage_index;

    assert(std::size(options.timers) == 2);

    const auto [timera_index, timera_var] = unwrap_timer_options(
            options.timers[0], symtable, scope_id);
    const auto [timerb_index, timerb_var] = unwrap_timer_options(
            options.timers[1], symtable, scope_id);

    assert(!timera_var || num_indices_for_var(*timera_var) == 1);
    assert(!timerb_var || num_indices_for_var(*timerb_var) == 1);

    const auto vars = symtable.scope(scope_id);

    BigIndexType current_index = first_storage_index;

    const auto var_lookup_by_id = build_lookup_by_id(vars);

    storage.index_for_vars.resize(var_lookup_by_id.size());
    for(size_t i = 0; i < var_lookup_by_id.size(); ++i)
    {
        const auto& var = *var_lookup_by_id[i];
        if(&var == timera_var)
        {
            storage.index_for_vars[i] = timera_index;
        }
        else if(&var == timerb_var)
        {
            storage.index_for_vars[i] = timerb_index;
        }
        else
        {
            while(current_index == timera_index
                  || current_index == timerb_index)
                ++current_index;

            storage.index_for_vars[i] = static_cast<IndexType>(current_index);

            current_index += num_indices_for_var(var);
            if(current_index > max_var_index + 1)
                return std::nullopt;
        }
    }

    return storage;
}

auto StorageTable::var_index(const SymbolTable::Variable& var) const noexcept
        -> IndexType
{
    const auto table_idx = to_integer(var.scope());
    assert(table_idx < table_for_scopes.size());
    return table_for_scopes[table_idx].var_index(var);
}

auto StorageTable::from_symbols(const SymbolTable& symtable,
                                const Options& options) noexcept
        -> std::optional<StorageTable>
{
    StorageTable storage;
    storage.table_for_scopes.reserve(symtable.num_scopes());

    for(uint32_t i = 0; i < symtable.num_scopes(); ++i)
    {
        const SymbolTable::ScopeId scope_id{i};

        if(auto local_table = LocalStorageTable::from_symbols(
                   symtable, scope_id, options_for_scope(scope_id, options));
           local_table)
        {
            storage.table_for_scopes.emplace_back(std::move(*local_table));
        }
        else
        {
            return std::nullopt;
        }
    }

    return storage;
}

auto build_lookup_by_id(SymbolTable::VariableNamespaceView vars) noexcept
        -> std::vector<const SymbolTable::Variable*>
{
    std::vector<const SymbolTable::Variable*> result(vars.size());
    for(const auto& var : vars)
    {
        assert(var.id() < result.size());
        result[var.id()] = &var;
    }
    return result;
}

auto num_indices_for_var(const SymbolTable::Variable& var) noexcept
        -> BigIndexType
{
    return num_indices_for_type(var.type()) * var.dimensions().value_or(1);
}

auto num_indices_for_type(SymbolTable::VarType type) noexcept -> BigIndexType
{
    switch(type)
    {
        case SymbolTable::Variable::Type::INT:
        case SymbolTable::Variable::Type::FLOAT:
            return 1;
        case SymbolTable::Variable::Type::TEXT_LABEL:
            return 2;
    }
    assert(false);
}

auto unwrap_timer_options(
        const std::optional<LocalStorageTable::TimerOptions>& timer_opt,
        const SymbolTable& symtable, SymbolTable::ScopeId scope_id) noexcept
        -> std::pair<BigIndexType, const SymbolTable::Variable*>
{
    if(!timer_opt.has_value())
        return {impossible_index, nullptr};

    if(const auto var = symtable.lookup_var(timer_opt->name, scope_id))
        return {timer_opt->index, var};

    return {timer_opt->index, nullptr};
}

auto options_for_scope(SymbolTable::ScopeId scope_id,
                       const StorageTable::Options& options) noexcept
        -> LocalStorageTable::Options
{
    if(scope_id == SymbolTable::global_scope)
    {
        return LocalStorageTable::Options{
                .first_storage_index = options.first_var_storage_index,
                .max_storage_index = options.max_var_storage_index,
        };
    }
    else
    {
        return LocalStorageTable::Options{
                .first_storage_index = options.first_lvar_storage_index,
                .max_storage_index = options.max_lvar_storage_index,
                .timers{options.timers[0], options.timers[1]}};
    }
}
} // namespace gta3sc::codegen
