#include <gta3sc/ir/symbol-table.hpp>

namespace gta3sc
{
SymbolRepository::SymbolRepository()
{
    const auto global_scope = allocate_scope();
    assert(global_scope == 0);
}

auto SymbolRepository::allocate_scope() -> ScopeId
{
    var_tables.emplace_back();
    return var_tables.size() - 1;
}

auto SymbolRepository::lookup_var(std::string_view name, ScopeId scope_id) const
        -> SymVariable*
{
    assert(scope_id < var_tables.size());
    auto it = var_tables[scope_id].find(name);
    return (it == var_tables[scope_id].end() ? nullptr : it->second);
}

auto SymbolRepository::lookup_label(std::string_view name) const -> SymLabel*
{
    auto it = label_table.find(name);
    return (it == label_table.end() ? nullptr : it->second);
}

auto SymbolRepository::insert_var(std::string_view name_, ScopeId scope_id,
                                  SymVariable::Type type,
                                  std::optional<uint16_t> dim,
                                  SourceRange source)
        -> std::pair<SymVariable*, bool>
{
    assert(scope_id < var_tables.size());

    if(auto v = lookup_var(name_, scope_id))
        return {v, false};

    const auto name = allocate_string(name_, arena);
    const auto symbol = new(arena) SymVariable{source, type, dim};
    const auto [iter, _] = var_tables[scope_id].emplace(name, symbol);
    return {iter->second, true};
}

auto SymbolRepository::insert_label(std::string_view name_, SourceRange source)
        -> std::pair<SymLabel*, bool>
{
    if(auto l = lookup_label(name_))
        return {l, false};

    const auto name = allocate_string(name_, arena);
    const auto symbol = new(arena) SymLabel{source};
    const auto [iter, _] = label_table.emplace(name, symbol);
    return {iter->second, true};
}
}
