#include <gta3sc/ir/symbol-table.hpp>

namespace gta3sc
{
SymbolRepository::SymbolRepository(ArenaMemoryResource& arena) : arena(&arena)
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

auto SymbolRepository::lookup_used_object(std::string_view name) const
        -> SymUsedObject*
{
    auto it = used_objects.find(name);
    return (it == used_objects.end() ? nullptr : it->second);
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

    const auto id = static_cast<uint32_t>(var_tables[scope_id].size());
    const auto name = util::allocate_string(name_, *arena);
    const auto symbol = new(*arena, alignof(SymVariable))
            SymVariable{source, id, scope_id, type, dim};
    const auto [iter, _] = var_tables[scope_id].emplace(name, symbol);
    return {iter->second, true};
}

auto SymbolRepository::insert_label(std::string_view name_, ScopeId scope_id,
                                    SourceRange source)
        -> std::pair<SymLabel*, bool>
{
    if(auto l = lookup_label(name_))
        return {l, false};

    const auto name = util::allocate_string(name_, *arena);
    const auto symbol = new(*arena, alignof(SymLabel))
            SymLabel{source, scope_id};
    const auto [iter, _] = label_table.emplace(name, symbol);
    return {iter->second, true};
}

auto SymbolRepository::insert_used_object(std::string_view name_,
                                          SourceRange source)
        -> std::pair<SymUsedObject*, bool>
{
    if(auto uobj = lookup_used_object(name_))
        return {uobj, false};

    const auto id = static_cast<uint32_t>(used_objects.size());
    const auto name = util::allocate_string(name_, *arena);
    const auto symbol = new(*arena, alignof(SymUsedObject))
            SymUsedObject{source, id};
    const auto [iter, _] = used_objects.emplace(name, symbol);
    return {iter->second, true};
}
}
