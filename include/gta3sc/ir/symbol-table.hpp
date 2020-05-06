#pragma once
#include <cstdint>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/util/arena.hpp>
#include <gta3sc/util/container-view.hpp>
#include <gta3sc/util/element-iterator-adaptor.hpp>
#include <optional>
#include <unordered_map>
#include <vector>

namespace gta3sc
{
/// A table storing semantic symbols.
///
/// This table shelters multiple namespaces, each of which the names do not
/// collide with each other.
///
/// This table stores symbols representing labels, variables, used objects and
/// script files.
///
/// Variables are stored in scopes. Each scope is identified by a index. The
/// names from a scope does not collide with names from another scope. The
/// global variable scope is always present on the table.
class SymbolTable
{
private:
    /// This tag is used to make construction of the IR elements private.
    struct PrivateTag
    {
    };
    static constexpr PrivateTag private_tag{};

public:
    class Label;
    class Variable;
    class UsedObject;

    template<typename T>
    class NamespaceView;

    /// The type of a variable.
    enum class VarType : uint8_t
    {               // GTA3script types should be uppercase.
        INT,        // NOLINT(readability-identifier-naming)
        FLOAT,      // NOLINT(readability-identifier-naming)
        TEXT_LABEL, // NOLINT(readability-identifier-naming)
    };

    // Uniquely identifies a variable scope.
    enum class ScopeId : uint32_t
    { // strong typedef
    };

    /// A symbol id is an integer representing the order a symbol was inserted
    /// in its table.
    ///
    /// Symbol ids can be used to enumerate symbols (i.e. map it to a 0-indexed
    /// table).
    using SymbolId = uint32_t;

    /// The index of the global variable scope.
    ///
    /// The global scope is guarated to be the zero-initialized `ScopeId`.
    static constexpr ScopeId global_scope = ScopeId{0};

    /// The index of a scope that is never ever used.
    static constexpr ScopeId invalid_scope = ScopeId{static_cast<uint32_t>(-1)};

public:
    explicit SymbolTable(ArenaAllocator<> allocator) noexcept;

    SymbolTable(const SymbolTable&) = delete;
    auto operator=(const SymbolTable&) -> SymbolTable& = delete;

    SymbolTable(SymbolTable&&) noexcept = default;
    auto operator=(SymbolTable&&) noexcept -> SymbolTable& = default;

    ~SymbolTable() noexcept = default;

    /// Returns the number of variable scopes in the table.
    [[nodiscard]] auto num_scopes() const noexcept -> uint32_t;

    /// Returns a view to the variables in a given scope.
    [[nodiscard]] auto scope(ScopeId scope_id) const noexcept
            -> NamespaceView<Variable>;

    /// Returns a view to the labels stored in the table.
    [[nodiscard]] auto labels() const noexcept -> NamespaceView<Label>;

    /// Returns a view to the used objects stored in the table.
    [[nodiscard]] auto used_objects() const noexcept
            -> NamespaceView<UsedObject>;

    /// Finds a variable in a certain scope.
    ///
    /// Returns the variable or `nullptr` if it does not exist.
    [[nodiscard]] auto
    lookup_var(std::string_view name,
               ScopeId scope_id = global_scope) const noexcept
            -> const Variable*;

    /// Finds a label by name.
    ///
    /// Returns the label or `nullptr` if it does not exist.
    [[nodiscard]] auto lookup_label(std::string_view name) const noexcept
            -> const Label*;

    /// Finds an used object by name.
    ///
    /// Returns the used object or `nullptr` if it does not exist in the table.
    [[nodiscard]] auto lookup_used_object(std::string_view name) const noexcept
            -> const UsedObject*;

    /// Creates a new scope and returns its identifier.
    ///
    /// It is guaranted that successive calls to this function provides
    /// an index that is the successor of the previous returned value.
    auto new_scope() -> ScopeId;

    /// Inserts a new variable into a certain scope.
    ///
    /// No insertion takes place if the variable already exists in the
    /// specified scope.
    ///
    /// \param name the name of the variable.
    /// \param scope_id the scope to insert the variable into.
    /// \param type the typing of the variable.
    /// \param dimensions the array dimensions of the variable if any.
    /// \param source the location of its declaration in the source code.
    ///
    /// Returns a pair with the variable and a boolean indicating whether any
    /// insertion took place.
    auto insert_var(std::string_view name, ScopeId scope_id, VarType type,
                    std::optional<uint16_t> dimensions, SourceRange source)
            -> std::pair<const Variable*, bool>;

    /// Inserts a label into the symbol table.
    ///
    /// No insertion takes place if a label of same name already exists.
    ///
    /// \param name the name of the label.
    /// \param scope_id the scope on which the label is in.
    /// \param source the location of its declaration.
    ///
    /// Returns a pair with the label and a boolean indicating whether any
    /// insertion took place.
    auto insert_label(std::string_view name, ScopeId scope_id,
                      SourceRange source) -> std::pair<const Label*, bool>;

    /// Inserts an used object into the symbol table.
    ///
    /// No insertion takes place if an used object of same name already exists.
    ///
    /// \param name the model name of the used object.
    /// \param source the location the used object was seen.
    ///
    /// Returns a pair with the used object and a boolean indicating whether any
    /// insertion took place.
    auto insert_used_object(std::string_view name, SourceRange source)
            -> std::pair<const UsedObject*, bool>;

private:
    template<typename T>
    using SymbolMap = std::unordered_map<std::string_view, ArenaPtr<const T>>;

private:
    ArenaAllocator<> allocator;
    SymbolMap<Label> m_labels;
    SymbolMap<UsedObject> m_used_objects;
    std::vector<SymbolMap<Variable>> m_scopes;
};

/// A view into a symbol table namespace.
///
/// A namespace contains names that does not collide with each other.
template<typename T>
class SymbolTable::NamespaceView
    : public util::ContainerView<
              SymbolMap<T>, util::iterator_adaptor::DereferenceValueAdaptor>
{
private:
    using ContainerView = util::ContainerView<
            SymbolMap<T>, util::iterator_adaptor::DereferenceValueAdaptor>;

public:
    explicit NamespaceView(PrivateTag /*unused*/) : ContainerView() {}

    NamespaceView(PrivateTag /*unused*/, const SymbolMap<T>& symbols) :
        ContainerView(symbols)
    {}
};

/// Represents a declared label.
class SymbolTable::Label : public ArenaObj
{
public:
    /// Please use `SymbolTable::insert_label` to create one.
    Label(PrivateTag /*unused*/, std::string_view name, SourceRange source,
          SymbolId id, ScopeId scope) noexcept :
        m_name(name), m_source(source), m_id(id), m_scope(scope)
    {}

    /// Returns the name of the label.
    [[nodiscard]] auto name() const noexcept -> std::string_view
    {
        return m_name;
    }

    /// Returns the source location the label was declared in.
    [[nodiscard]] auto source() const noexcept -> SourceRange
    {
        return m_source;
    }

    /// Returns the order the label was inserted in the table.
    [[nodiscard]] auto id() const noexcept -> SymbolId { return m_id; }

    /// Returns the scope the label is contained in.
    [[nodiscard]] auto scope() const noexcept -> ScopeId { return m_scope; }

private:
    std::string_view m_name;
    SourceRange m_source;
    SymbolId m_id;
    ScopeId m_scope{};
};

/// Represents a declared variable.
class SymbolTable::Variable : public ArenaObj
{
public:
    /// The type of the variable.
    using Type = VarType;

    /// Please use `SymbolTable::insert_var` to create one.
    Variable(PrivateTag /*unused*/, std::string_view name, SourceRange source,
             SymbolId id, ScopeId scope, Type type,
             std::optional<uint16_t> dimensions) noexcept :
        m_name(name),
        m_source(source),
        m_id(id),
        m_scope(scope),
        m_type(type),
        m_dim(dimensions)
    {}

    /// Returns the name of the variable.
    [[nodiscard]] auto name() const noexcept -> std::string_view
    {
        return m_name;
    }

    /// Returns the location the variable was declared in.
    [[nodiscard]] auto source() const noexcept -> SourceRange
    {
        return m_source;
    }

    /// Returns the order this variable was inserted in its scope.
    [[nodiscard]] auto id() const noexcept -> SymbolId { return m_id; }

    /// Returns the scope the variable is in.
    [[nodiscard]] auto scope() const noexcept -> ScopeId { return m_scope; }

    /// Returns the typing of the variable.
    [[nodiscard]] auto type() const noexcept -> Type { return m_type; }

    /// Checks whether the variable is an array.
    [[nodiscard]] auto is_array() const noexcept -> bool
    {
        return m_dim.has_value();
    }

    /// Returns the dimensions of the array or `std::nullopt` if the variable
    /// isn't an array.
    [[nodiscard]] auto dimensions() const -> std::optional<uint16_t>
    {
        return m_dim;
    }

private:
    std::string_view m_name;
    SourceRange m_source;
    SymbolId m_id{};
    ScopeId m_scope{};
    std::optional<uint16_t> m_dim;
    Type m_type{};
};

/// Represents an used object.
class SymbolTable::UsedObject : public ArenaObj
{
public:
    /// Please use `SymbolTable::insert_used_object` to create one.
    UsedObject(PrivateTag /*unused*/, std::string_view name, SourceRange source,
               SymbolId id) noexcept :
        m_name(name), m_source(source), m_id(id)
    {}

    /// Returns the name of the used object.
    [[nodiscard]] auto name() const noexcept -> std::string_view
    {
        return m_name;
    }

    /// Returns the location the object was used for the first time.
    [[nodiscard]] auto source() const noexcept -> SourceRange
    {
        return m_source;
    }

    /// Returns the order the used object was inserted in the table.
    [[nodiscard]] auto id() const noexcept -> SymbolId { return m_id; }

private:
    std::string_view m_name;
    SourceRange m_source;
    SymbolId m_id{};
};

/// Converts the scope identifier to its integer representation.
constexpr auto to_integer(SymbolTable::ScopeId scope_id) noexcept -> uint32_t
{
    return static_cast<std::underlying_type_t<SymbolTable::ScopeId>>(scope_id);
}

constexpr auto operator+(SymbolTable::ScopeId lhs, uint32_t rhs) noexcept
        -> SymbolTable::ScopeId
{
    return SymbolTable::ScopeId{to_integer(lhs) + rhs};
}

constexpr auto operator+=(SymbolTable::ScopeId& lhs, uint32_t rhs) noexcept
        -> SymbolTable::ScopeId&
{
    lhs = lhs + rhs;
    return lhs;
}

constexpr auto operator-(SymbolTable::ScopeId lhs, uint32_t rhs) noexcept
        -> SymbolTable::ScopeId
{
    return SymbolTable::ScopeId{to_integer(lhs) - rhs};
}

constexpr auto operator-=(SymbolTable::ScopeId& lhs, int32_t rhs) noexcept
        -> SymbolTable::ScopeId&
{
    lhs = lhs - rhs;
    return lhs;
}

constexpr auto operator++(SymbolTable::ScopeId& lhs) noexcept
        -> SymbolTable::ScopeId
{
    lhs += 1;
    return lhs;
}

constexpr auto operator--(SymbolTable::ScopeId& lhs) noexcept
        -> SymbolTable::ScopeId&
{
    lhs -= 1;
    return lhs;
}

constexpr auto operator++(SymbolTable::ScopeId& lhs, int) noexcept
        -> SymbolTable::ScopeId
{
    const auto temp = lhs;
    ++lhs;
    return temp;
}

constexpr auto operator--(SymbolTable::ScopeId& lhs, int) noexcept
        -> SymbolTable::ScopeId
{
    const auto temp = lhs;
    --lhs;
    return temp;
}

inline auto SymbolTable::scope(ScopeId scope_id) const noexcept
        -> NamespaceView<Variable>
{
    if(scope_id == global_scope && m_scopes.empty())
        return NamespaceView<Variable>(private_tag);

    const auto scope_index = to_integer(scope_id);
    assert(scope_index < m_scopes.size());

    return NamespaceView<Variable>(private_tag, m_scopes[scope_index]);
}

inline auto SymbolTable::used_objects() const noexcept
        -> NamespaceView<UsedObject>
{
    return NamespaceView<UsedObject>(private_tag, m_used_objects);
}

inline auto SymbolTable::num_scopes() const noexcept -> uint32_t
{
    static_assert(std::is_same_v<std::underlying_type_t<ScopeId>, uint32_t>);
    return m_scopes.empty() ? 1 : static_cast<uint32_t>(m_scopes.size());
}

inline auto SymbolTable::labels() const noexcept -> NamespaceView<Label>
{
    return NamespaceView<Label>(private_tag, m_labels);
}

// These resources are stored in a memory arena. Disposing storage must be
// enough for their destruction.
static_assert(std::is_trivially_destructible_v<SymbolTable::Label>);
static_assert(std::is_trivially_destructible_v<SymbolTable::Variable>);
static_assert(std::is_trivially_destructible_v<SymbolTable::UsedObject>);
} // namespace gta3sc
