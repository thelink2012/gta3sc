#pragma once
#include <cstdint>
#include <gta3sc/ir/symbol-table.hpp>
#include <vector>

namespace gta3sc::codegen
{
/// Table holding information about storage of variables in a scope.
///
/// This data structure is immutable and holds information about the storage
/// location of each variable in a given scope from a symbol table.
class LocalStorageTable
{
public:
    using IndexType = uint16_t;

    struct TimerOptions
    {
        /// The index the timer variable should be allocated.
        IndexType index{};
        /// The name of the timer variable in the symbol table.
        std::string_view name;
    };

    struct Options
    {
        /// The index where the first variable will be allocated.
        IndexType first_storage_index{0};
        /// The maximum index that can be used to allocate variables.
        IndexType max_storage_index{0};
        /// Options about timer variables.
        std::optional<TimerOptions> timers[2];
    };

public:
    /// Constructs an empty table.
    LocalStorageTable() noexcept = default;

    LocalStorageTable(const LocalStorageTable&) = delete;
    auto operator=(const LocalStorageTable&) -> LocalStorageTable& = delete;

    LocalStorageTable(LocalStorageTable&&) noexcept = default;
    auto operator=(LocalStorageTable&&) noexcept
            -> LocalStorageTable& = default;

    ~LocalStorageTable() noexcept = default;

    /// Computes the storage assigment for the variables in a given scope from a
    /// symbol table.
    ///
    /// Returns the storage table or `std::nullopt` in case not enough storage
    /// is available for the given variables.
    [[nodiscard]] static auto from_symbols(const SymbolTable& symtable,
                                           SymbolTable::ScopeId scope_id,
                                           const Options& options) noexcept
            -> std::optional<LocalStorageTable>;

    /// Returns the index for the given variable.
    ///
    /// Behaviour is undefined if the given variable isn't part of the input
    /// symbol table scope.
    [[nodiscard]] auto
    var_index(const SymbolTable::Variable& var) const noexcept -> IndexType;

private:
    std::vector<IndexType> index_for_vars;
};

/// Table holding information about storage of variables in multiple scopes.
///
/// This behaves mostly the same as `LocalStorageTable` except it can handle
/// variables from multiple scopes.
class StorageTable
{
public:
    using IndexType = typename LocalStorageTable::IndexType;

    using TimerOptions = typename LocalStorageTable::TimerOptions;

    struct Options
    {
        /// The index where the first global variable will be allocated.
        IndexType first_var_storage_index{2};
        /// The maximum index that can be used to allocate global variables.
        IndexType max_var_storage_index{16383};
        /// The index where the first local variable will be allocated.
        IndexType first_lvar_storage_index{0};
        /// The maximum index that can be used to allocate local variables.
        IndexType max_lvar_storage_index{17};
        /// Options about local timer variables.
        std::optional<TimerOptions> timers[2]{
                TimerOptions{.index = 16, .name = "TIMERA"},
                TimerOptions{.index = 17, .name = "TIMERB"}};
    };

public:
    /// Constructs an empty table.
    StorageTable() noexcept = default;

    StorageTable(const StorageTable&) = delete;
    auto operator=(const StorageTable&) -> StorageTable& = delete;

    StorageTable(StorageTable&&) noexcept = default;
    auto operator=(StorageTable&&) noexcept -> StorageTable& = default;

    ~StorageTable() noexcept = default;

    /// Computes the storage assigment for the variables in all scopes of
    /// the given symbol table.
    ///
    /// Returns the storage table or `std::nullopt` in case not enough
    /// storage is available for the given variables.
    [[nodiscard]] static auto from_symbols(const SymbolTable& symtable,
                                           const Options& options) noexcept
            -> std::optional<StorageTable>;

    /// Returns the index for the given variable.
    ///
    /// \note The index sequence of each scope is independent from each
    /// other, hence calling this with two variables from different scopes
    /// may result in the same index.
    ///
    /// Behaviour is undefined if the given variable isn't part of the input
    /// symbol table.
    [[nodiscard]] auto
    var_index(const SymbolTable::Variable& var) const noexcept -> IndexType;

private:
    std::vector<LocalStorageTable> table_for_scopes;
};
} // namespace gta3sc::codegen
