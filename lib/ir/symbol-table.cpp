#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/util/memory.hpp>

namespace gta3sc
{
SymbolTable::SymbolTable(ArenaAllocator<> allocator) noexcept :
    allocator(allocator)
{}

auto SymbolTable::new_scope() -> ScopeId
{
    // In case the global scope hasn't been allocated yet,
    // we need to allocate it and the requested scope.
    const auto num_new_scopes = m_scopes.empty() ? 2 : 1;

    m_scopes.resize(m_scopes.size() + num_new_scopes);

    return ScopeId{static_cast<uint32_t>(m_scopes.size() - 1)};
}

auto SymbolTable::lookup_var(std::string_view name,
                             ScopeId scope_id) const noexcept
        -> const SymbolTable::Variable*
{
    if(scope_id == global_scope && m_scopes.empty())
        return nullptr;

    const auto scope_index = to_integer(scope_id);
    assert(scope_index < m_scopes.size());

    const auto it = m_scopes[scope_index].find(name);
    return (it == m_scopes[scope_index].end() ? nullptr : it->second);
}

auto SymbolTable::lookup_label(std::string_view name) const noexcept
        -> const Label*
{
    const auto it = m_labels.find(name);
    return (it == m_labels.end() ? nullptr : it->second);
}

auto SymbolTable::lookup_file(std::string_view name) const noexcept
        -> const File*
{
    const auto it = m_files.find(name);
    return (it == m_files.end() ? nullptr : it->second);
}

auto SymbolTable::lookup_used_object(std::string_view name) const noexcept
        -> const SymbolTable::UsedObject*
{
    const auto it = m_used_objects.find(name);
    return (it == m_used_objects.end() ? nullptr : it->second);
}

auto SymbolTable::insert_var(std::string_view name, ScopeId scope_id,
                             SymbolTable::Variable::Type type,
                             std::optional<uint16_t> dimensions,
                             SourceRange source)
        -> std::pair<const SymbolTable::Variable*, bool>
{
    if(const auto* v = lookup_var(name, scope_id))
        return {v, false};

    if(scope_id == global_scope && m_scopes.empty())
        m_scopes.emplace_back();

    const auto scope_index = to_integer(scope_id);
    assert(scope_index < m_scopes.size());

    const auto var_id = static_cast<uint32_t>(m_scopes[scope_index].size());
    assert(var_id < std::numeric_limits<decltype(var_id)>::max());

    const auto a_name = util::new_string(name, allocator);

    const auto* const symbol = allocator.new_object<SymbolTable::Variable>(
            private_tag, a_name, source, var_id, scope_id, type, dimensions);

    const auto [iter, inserted] = m_scopes[scope_index].emplace(a_name, symbol);
    assert(inserted);

    return {iter->second, true};
}

auto SymbolTable::insert_label(std::string_view name, ScopeId scope_id,
                               SourceRange source)
        -> std::pair<const Label*, bool>
{
    if(const auto* l = lookup_label(name))
        return {l, false};

    const auto label_id = static_cast<uint32_t>(m_labels.size());
    assert(label_id < std::numeric_limits<decltype(label_id)>::max());

    const auto a_name = util::new_string(name, allocator);

    const auto* const symbol = allocator.new_object<Label>(
            private_tag, a_name, source, label_id, scope_id);

    const auto [iter, inserted] = m_labels.emplace(a_name, symbol);
    assert(inserted);

    return {iter->second, true};
}

auto SymbolTable::insert_file(std::string_view name, FileType type,
                              SourceRange source)
        -> std::pair<const File*, bool>
{
    if(const auto* f = lookup_file(name))
        return {f, false};

    const auto file_id = static_cast<uint32_t>(m_files.size());
    assert(file_id < std::numeric_limits<decltype(file_id)>::max());

    const auto type_as_int = static_cast<uint8_t>(type);
    assert(type_as_int < std::size(m_num_files_of_type));

    const auto type_id = m_num_files_of_type[type_as_int]++;

    const auto a_name = util::new_string(name, allocator);

    const auto* const symbol = allocator.new_object<File>(
            private_tag, a_name, source, file_id, type_id, type);

    const auto [iter, inserted] = m_files.emplace(a_name, symbol);
    assert(inserted);

    return {iter->second, true};
}

auto SymbolTable::insert_used_object(std::string_view name, SourceRange source)
        -> std::pair<const SymbolTable::UsedObject*, bool>
{
    if(const auto* uobj = lookup_used_object(name))
        return {uobj, false};

    const auto obj_id = static_cast<uint32_t>(m_used_objects.size());
    assert(obj_id < std::numeric_limits<decltype(obj_id)>::max());

    const auto a_name = util::new_string(name, allocator);

    const auto* const symbol = allocator.new_object<SymbolTable::UsedObject>(
            private_tag, a_name, source, obj_id);

    const auto [iter, inserted] = m_used_objects.emplace(a_name, symbol);
    assert(inserted);

    return {iter->second, true};
}
} // namespace gta3sc
