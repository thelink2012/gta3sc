#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/util/arena-utility.hpp>

namespace gta3sc
{
SymbolRepository::SymbolRepository(ArenaAllocator<> allocator) :
    allocator(allocator)
{
    // FIXME defer allocation to later when we make members private
    // Note that this will turn the ctor into noexcept.
    const auto global_scope = allocate_scope();
    assert(global_scope == 0);
}

auto SymbolRepository::allocate_scope() -> ScopeId
{
    var_tables.emplace_back();
    return var_tables.size() - 1;
}

auto SymbolRepository::lookup_var(std::string_view name, ScopeId scope_id) const
        -> const SymVariable *
{
    assert(scope_id < var_tables.size());
    auto it = var_tables[scope_id].find(name);
    return (it == var_tables[scope_id].end() ? nullptr : it->second);
}

auto SymbolRepository::lookup_label(std::string_view name) const
        -> const SymLabel *
{
    auto it = label_table.find(name);
    return (it == label_table.end() ? nullptr : it->second);
}

auto SymbolRepository::lookup_used_object(std::string_view name) const
        -> const SymUsedObject *
{
    auto it = used_objects.find(name);
    return (it == used_objects.end() ? nullptr : it->second);
}

auto SymbolRepository::insert_var(std::string_view name, ScopeId scope_id,
                                  SymVariable::Type type,
                                  std::optional<uint16_t> dim,
                                  SourceRange source)
        -> std::pair<const SymVariable *, bool>
{
    assert(scope_id < var_tables.size());

    if(const auto *v = lookup_var(name, scope_id))
        return {v, false};

    const auto id = static_cast<uint32_t>(var_tables[scope_id].size());
    const auto a_name = util::allocate_string(name, allocator);
    const auto *const symbol = allocator.new_object<SymVariable>(
            SymVariable{source, id, scope_id, type, dim});
    const auto [iter, _] = var_tables[scope_id].emplace(a_name, symbol);
    return {iter->second, true};
}

auto SymbolRepository::insert_label(std::string_view name, ScopeId scope_id,
                                    SourceRange source)
        -> std::pair<const SymLabel *, bool>
{
    if(const auto *l = lookup_label(name))
        return {l, false};

    const auto a_name = util::allocate_string(name, allocator);
    const auto *const symbol = allocator.new_object<SymLabel>(
            SymLabel{source, scope_id});
    const auto [iter, _] = label_table.emplace(a_name, symbol);
    return {iter->second, true};
}

auto SymbolRepository::insert_used_object(std::string_view name,
                                          SourceRange source)
        -> std::pair<const SymUsedObject *, bool>
{
    if(const auto *uobj = lookup_used_object(name))
        return {uobj, false};

    const auto id = static_cast<uint32_t>(used_objects.size());
    const auto a_name = util::allocate_string(name, allocator);
    const auto *const symbol = allocator.new_object<SymUsedObject>(
            SymUsedObject{source, id});
    const auto [iter, _] = used_objects.emplace(a_name, symbol);
    return {iter->second, true};
}
} // namespace gta3sc
