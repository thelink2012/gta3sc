#include <doctest/doctest.h>
#include <gta3sc/codegen/storage-table.hpp>

using gta3sc::SourceManager;
using gta3sc::SymbolTable;
using gta3sc::codegen::LocalStorageTable;
using gta3sc::codegen::StorageTable;
using VarType = gta3sc::SymbolTable::VarType;

namespace
{
class BaseStorageTableFixture
{
public:
    BaseStorageTableFixture() : symtable(&arena) {}

    auto make_scope() -> SymbolTable::ScopeId { return symtable.new_scope(); }

    auto make_var(SymbolTable::ScopeId scope_id, VarType var_type,
                  std::optional<uint16_t> dimensions = std::nullopt)
            -> const SymbolTable::Variable&
    {
        const auto [var, inserted] = symtable.insert_var(
                std::to_string(next_symbol_id++), scope_id, var_type,
                dimensions, SourceManager::no_source_range);
        REQUIRE(inserted);
        return *var;
    }

protected:
    gta3sc::ArenaMemoryResource arena; // NOLINT: Protected member is fine since
    gta3sc::SymbolTable symtable;      // NOLINT: the number of deriveds is
    uint32_t next_symbol_id{};         // NOLINT: limited within this file.
};

class LocalStorageTableFixture : public BaseStorageTableFixture
{
public:
    static constexpr LocalStorageTable::Options default_options{
            .first_storage_index = 2,
            .max_storage_index = 16383,
    };

    auto make_var(VarType var_type,
                  std::optional<uint16_t> dimensions = std::nullopt)
            -> const SymbolTable::Variable&
    {
        return BaseStorageTableFixture::make_var(SymbolTable::global_scope,
                                                 var_type, dimensions);
    }

    auto make_n_vars(size_t n, VarType var_type,
                     std::optional<uint16_t> dimensions = std::nullopt)
            -> const SymbolTable::Variable&
    {
        const SymbolTable::Variable* last_var{};
        for(size_t i = 0; i < n; ++i)
            last_var = &make_var(var_type, dimensions);
        return *last_var;
    }

    auto make_storage_table(const LocalStorageTable::Options& options)
            -> LocalStorageTable
    {
        auto table = LocalStorageTable::from_symbols(
                symtable, SymbolTable::global_scope, options);
        REQUIRE(table != std::nullopt);
        return std::move(table).value();
    }

    auto make_storage_table() -> LocalStorageTable
    {
        return make_storage_table(default_options);
    }

    void fail_to_make_storage_table(const LocalStorageTable::Options& options)
    {
        auto table = LocalStorageTable::from_symbols(
                symtable, SymbolTable::global_scope, options);
        REQUIRE(table == std::nullopt);
    }

    void fail_to_make_storage_table()
    {
        return fail_to_make_storage_table(default_options);
    }
};

class StorageTableFixture : public BaseStorageTableFixture
{
public:
    auto make_storage_table(const StorageTable::Options& options)
            -> StorageTable
    {
        auto table = StorageTable::from_symbols(symtable, options);
        REQUIRE(table != std::nullopt);
        return std::move(table).value();
    }
};
} // namespace

TEST_CASE_FIXTURE(LocalStorageTableFixture, "basic storage")
{
    const auto first_storage_index = default_options.first_storage_index;

    SUBCASE("storage starts from first storage index")
    {
        const auto& var0 = make_var(VarType::INT);
        REQUIRE(make_storage_table().var_index(var0) == first_storage_index);
    }

    SUBCASE("integer takes a single index of storage")
    {
        const auto& var0 = make_var(VarType::INT);
        const auto& var1 = make_var(VarType::INT);
        const auto table = make_storage_table();
        REQUIRE(table.var_index(var0) == first_storage_index);
        REQUIRE(table.var_index(var1) == first_storage_index + 1);
    }

    SUBCASE("float variable takes a single index of storage")
    {
        const auto& var0 = make_var(VarType::FLOAT);
        const auto& var1 = make_var(VarType::INT);
        const auto table = make_storage_table();
        REQUIRE(table.var_index(var0) == first_storage_index);
        REQUIRE(table.var_index(var1) == first_storage_index + 1);
    }

    SUBCASE("text label variable takes two indices of storage")
    {
        const auto& var0 = make_var(VarType::TEXT_LABEL);
        const auto& var1 = make_var(VarType::INT);
        const auto table = make_storage_table();
        REQUIRE(table.var_index(var0) == first_storage_index);
        REQUIRE(table.var_index(var1) == first_storage_index + 2);
    }
}

TEST_CASE_FIXTURE(LocalStorageTableFixture, "array storage")
{
    const auto first_storage_index = default_options.first_storage_index;
    const auto var0_dim = 10;

    SUBCASE("integer array takes N times the storage of the non-array")
    {
        const auto& var0 = make_var(VarType::INT, var0_dim);
        const auto& var1 = make_var(VarType::INT);
        const auto table = make_storage_table();
        REQUIRE(table.var_index(var0) == first_storage_index);
        REQUIRE(table.var_index(var1) == first_storage_index + var0_dim);
    }

    SUBCASE("floating-point array takes N times the storage of the non-array")
    {
        const auto& var0 = make_var(VarType::FLOAT, var0_dim);
        const auto& var1 = make_var(VarType::INT);
        const auto table = make_storage_table();
        REQUIRE(table.var_index(var0) == first_storage_index);
        REQUIRE(table.var_index(var1) == first_storage_index + var0_dim);
    }

    SUBCASE("text label array takes 2N times the storage of the non-array")
    {
        const auto& var0 = make_var(VarType::TEXT_LABEL, var0_dim);
        const auto& var1 = make_var(VarType::INT);
        const auto table = make_storage_table();
        REQUIRE(table.var_index(var0) == first_storage_index);
        REQUIRE(table.var_index(var1) == first_storage_index + 2 * var0_dim);
    }
}

TEST_CASE_FIXTURE(LocalStorageTableFixture, "storage limits")
{
    const uint32_t max_var_index = default_options.max_storage_index;
    const uint32_t max_num_int_vars = max_var_index - 1;
    const uint32_t max_num_float_vars = max_var_index - 1;
    const uint32_t max_num_text_label_vars = (max_var_index - 1) / 2;

    SUBCASE("storage is limited by maximum integer variables")
    {
        const auto& last_var = make_n_vars(max_num_int_vars, VarType::INT);
        REQUIRE(make_storage_table().var_index(last_var) == max_var_index);
        make_var(VarType::INT);
        fail_to_make_storage_table();
    }

    SUBCASE("storage is limited by maximum float variables")
    {
        const auto& last_var = make_n_vars(max_num_float_vars, VarType::FLOAT);
        REQUIRE(make_storage_table().var_index(last_var) == max_var_index);
        make_var(VarType::INT);
        fail_to_make_storage_table();
    }

    SUBCASE("storage is limited by maximum text label variables")
    {
        const auto& last_text_label_var = make_n_vars(max_num_text_label_vars,
                                                      VarType::TEXT_LABEL);
        REQUIRE(make_storage_table().var_index(last_text_label_var)
                == max_var_index - 1);
        make_var(VarType::INT);
        fail_to_make_storage_table();
    }
}

TEST_CASE_FIXTURE(LocalStorageTableFixture, "storage limits with arrays")
{
    const uint32_t max_var_index = default_options.max_storage_index;
    const uint32_t max_num_int_vars = max_var_index - 1;
    const uint32_t max_num_float_vars = max_var_index - 1;
    const uint32_t max_num_text_label_vars = (max_var_index - 1) / 2;

    SUBCASE("can make integer array with index size of maximum integer "
            "variables")
    {
        const auto& last_var = make_var(VarType::INT, max_num_int_vars);
        REQUIRE(make_storage_table().var_index(last_var) == 2);
    }

    SUBCASE("cannot make integer array with index size off maximum integer "
            "variables")
    {
        make_var(VarType::INT, max_num_int_vars + 1);
        fail_to_make_storage_table();
    }

    SUBCASE("can make float array with index size of maximum float variables")
    {
        const auto& last_var = make_var(VarType::FLOAT, max_num_float_vars);
        REQUIRE(make_storage_table().var_index(last_var) == 2);
    }

    SUBCASE("cannot make float array with index size off maximum float "
            "variables")
    {
        make_var(VarType::FLOAT, max_num_float_vars + 1);
        fail_to_make_storage_table();
    }

    SUBCASE("can make text label array with index size of maximum text label "
            "variables")
    {
        const auto& last_var = make_var(VarType::TEXT_LABEL,
                                        max_num_text_label_vars);
        REQUIRE(make_storage_table().var_index(last_var) == 2);
    }

    SUBCASE("cannot make text label array with index size off maximum text "
            "label variables")
    {
        make_var(VarType::TEXT_LABEL, max_num_text_label_vars + 1);
        fail_to_make_storage_table();
    }
}

TEST_CASE_FIXTURE(LocalStorageTableFixture,
                  "first storage index greater than maximum storage index "
                  "cannot make any var")
{
    const LocalStorageTable::Options options{.first_storage_index = 3,
                                             .max_storage_index = 2};
    make_storage_table(options);
    make_var(VarType::INT);
    fail_to_make_storage_table(options);
}

TEST_CASE_FIXTURE(LocalStorageTableFixture, "storage with one index")
{
    const LocalStorageTable::Options options{.first_storage_index = 2,
                                             .max_storage_index = 2};
    make_storage_table(options);

    SUBCASE("can make one integer var")
    {
        make_var(VarType::INT);
        make_storage_table(options);
        make_var(VarType::INT);
        fail_to_make_storage_table(options);
    }

    SUBCASE("can make one float var")
    {
        make_var(VarType::FLOAT);
        make_storage_table(options);
        make_var(VarType::FLOAT);
        fail_to_make_storage_table(options);
    }

    SUBCASE("cannot make text label var")
    {
        make_var(VarType::TEXT_LABEL);
        fail_to_make_storage_table(options);
    }
}

TEST_CASE_FIXTURE(LocalStorageTableFixture, "storage with timers")
{
    const auto first_storage_index = default_options.first_storage_index;

    SUBCASE("timer at first index is skipped correctly")
    {
        const LocalStorageTable::Options options{
                .first_storage_index = first_storage_index,
                .max_storage_index = first_storage_index + 1,
                .timers = {LocalStorageTable::TimerOptions{
                        .index = first_storage_index,
                }}};

        const auto& var0 = make_var(VarType::INT);
        REQUIRE(make_storage_table(options).var_index(var0)
                == first_storage_index + 1);
    }

    SUBCASE("timer at middle index is skipped correctly")
    {
        const LocalStorageTable::Options options{
                .first_storage_index = first_storage_index,
                .max_storage_index = first_storage_index + 2,
                .timers = {LocalStorageTable::TimerOptions{
                        .index = first_storage_index + 1,
                }}};

        const auto& var0 = make_var(VarType::INT);
        const auto& var1 = make_var(VarType::INT);
        REQUIRE(make_storage_table(options).var_index(var0)
                == first_storage_index);
        REQUIRE(make_storage_table(options).var_index(var1)
                == first_storage_index + 2);
    }

    SUBCASE("timer at last index is skipped correctly")
    {
        const LocalStorageTable::Options options{
                .first_storage_index = first_storage_index,
                .max_storage_index = first_storage_index + 1,
                .timers = {LocalStorageTable::TimerOptions{
                        .index = first_storage_index + 1,
                }}};

        const auto& var0 = make_var(VarType::INT);
        REQUIRE(make_storage_table(options).var_index(var0)
                == first_storage_index);
        make_var(VarType::INT);
        fail_to_make_storage_table(options);
    }

    SUBCASE("timers are allocated to the option's indicated index")
    {
        const auto& var0 = make_var(VarType::INT);
        const auto& timera = make_var(VarType::INT);
        const auto& var1 = make_var(VarType::INT);
        const auto& timerb = make_var(VarType::INT);
        const auto& var2 = make_var(VarType::INT);

        const LocalStorageTable::Options options{
                .first_storage_index = first_storage_index,
                .max_storage_index = first_storage_index + 4,
                .timers = {LocalStorageTable::TimerOptions{
                                   .index = first_storage_index + 1,
                                   .name = timera.name(),
                           },
                           LocalStorageTable::TimerOptions{
                                   .index = first_storage_index + 2,
                                   .name = timerb.name()}}};

        const auto table = make_storage_table(options);
        REQUIRE(table.var_index(var0) == first_storage_index);
        REQUIRE(table.var_index(timera) == first_storage_index + 1);
        REQUIRE(table.var_index(timerb) == first_storage_index + 2);
        REQUIRE(table.var_index(var1) == first_storage_index + 3);
        REQUIRE(table.var_index(var2) == first_storage_index + 4);
    }

    SUBCASE("timer index may be outside the storage range")
    {
        const auto& var0 = make_var(VarType::INT);
        const auto& timera = make_var(VarType::INT);

        const LocalStorageTable::Options options{
                .first_storage_index = first_storage_index,
                .max_storage_index = first_storage_index,
                .timers = {LocalStorageTable::TimerOptions{
                        .index = first_storage_index + 10,
                        .name = timera.name(),
                }}};

        const auto table = make_storage_table(options);
        REQUIRE(table.var_index(var0) == first_storage_index);
        REQUIRE(table.var_index(timera) == first_storage_index + 10);
    }
}

TEST_CASE_FIXTURE(StorageTableFixture, "storage with multiple scopes")
{
    const auto scope0 = SymbolTable::global_scope;
    const auto& gvar0 = make_var(scope0, VarType::INT);
    const auto& gvar1 = make_var(scope0, VarType::FLOAT);

    const auto scope1 = make_scope();
    const auto& lvar0_scope1 = make_var(scope1, VarType::INT);
    const auto& lvar1_scope1 = make_var(scope1, VarType::FLOAT);

    const auto scope2 = make_scope();
    const auto& lvar0_scope2 = make_var(scope2, VarType::INT);
    const auto& lvar1_scope2 = make_var(scope2, VarType::FLOAT);
    const auto& timera_scope2 = make_var(scope2, VarType::INT);

    StorageTable::Options options;
    options.timers[0].value().name = timera_scope2.name();

    const auto first_gvar_storage_index = options.first_var_storage_index;
    const auto first_lvar_storage_index = options.first_lvar_storage_index;
    const auto timera_storage_index = options.timers[0].value().index;

    const auto table = make_storage_table(options);

    REQUIRE(table.var_index(gvar0) == first_gvar_storage_index);
    REQUIRE(table.var_index(gvar1) == first_gvar_storage_index + 1);
    REQUIRE(table.var_index(lvar0_scope1) == first_lvar_storage_index);
    REQUIRE(table.var_index(lvar1_scope1) == first_lvar_storage_index + 1);
    REQUIRE(table.var_index(lvar0_scope2) == first_lvar_storage_index);
    REQUIRE(table.var_index(lvar1_scope2) == first_lvar_storage_index + 1);
    REQUIRE(table.var_index(timera_scope2) == timera_storage_index);
}
