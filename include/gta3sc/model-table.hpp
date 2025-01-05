#pragma once
#include <gta3sc/util/memory.hpp>
#include <string_view>
#include <unordered_map>

namespace gta3sc
{
/// A table storing the object model definitions from the game.
///
/// This is an immutable data structure which holds information about the
/// game object models.
class ModelTable
{
private:
    /// This tag is used to make construction of the table elements private.
    struct PrivateTag
    {
    };
    static constexpr PrivateTag private_tag{};

public:
    class ModelDef;
    class Builder;

public:
    /// Constructs an empty model table.
    ///
    /// Use `Builder` to construct a non-empty object.
    ModelTable() noexcept = default;

    ModelTable(const ModelTable&) = delete;
    auto operator=(const ModelTable&) -> ModelTable& = delete;

    ModelTable(ModelTable&&) noexcept = default;
    auto operator=(ModelTable&&) noexcept -> ModelTable& = default;

    ~ModelTable() noexcept = default;

    /// Finds a model in the repository.
    ///
    /// Returns the model definition or `nullptr` if not found.
    [[nodiscard]] auto find_model(std::string_view name) const noexcept
            -> const ModelDef*;

private:
    using ModelMap
            = std::unordered_map<std::string_view, ArenaPtr<const ModelDef>>;

    ModelMap models;

    explicit ModelTable(ModelMap&& models) noexcept;
};

/// A model definition.
class ModelTable::ModelDef : public ArenaObj
{
public:
    explicit ModelDef(PrivateTag /*unused*/, uint8_t name_size,
                      uint32_t model_id) noexcept :
        m_model_id(model_id), m_name_size(name_size)
    {}

    /// Returns the name of the object model.
    [[nodiscard]] auto name() const noexcept -> std::string_view
    {
        return util::get_sibling_string(this, m_name_size);
    }

    /// Returns the id of the object model.
    [[nodiscard]] auto model_id() const noexcept -> uint32_t
    {
        return m_model_id;
    }

private:
    uint32_t m_model_id;
    uint8_t m_name_size;
};

/// A builder capable of constructing a `ModelTable`.
class ModelTable::Builder
{
public:
    /// Constructs a builder.
    ///
    /// Allocation of model definitions happens with the given arena allocator.
    explicit Builder(ArenaAllocator<> allocator) noexcept : allocator(allocator)
    {}

    Builder(const Builder&) = delete;
    auto operator=(const Builder&) -> Builder& = delete;

    Builder(Builder&&) noexcept = default;
    auto operator=(Builder&&) noexcept -> Builder& = default;

    ~Builder() noexcept = default;

    /// Inserts a model into the table.
    auto insert_model(std::string_view name, uint32_t id) -> Builder&&;

    /// Builds the model table.
    auto build() && -> ModelTable;

private:
    ArenaAllocator<> allocator;
    ModelMap models;
};

// The model definition is stored in an arena, thus it should be trivially
// destructible.
static_assert(std::is_trivially_destructible_v<ModelTable::ModelDef>);
} // namespace gta3sc
