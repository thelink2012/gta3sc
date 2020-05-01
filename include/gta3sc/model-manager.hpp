#pragma once
#include <functional>
#include <gta3sc/util/arena-allocator.hpp>
#include <unordered_set>

namespace gta3sc
{
/// Stores the game's object models definitions.
///
/// This is an immutable data structure which holds information about the
/// game object models.
class ModelManager
{
public:
    class Builder;

    /// A model definition.
    struct ModelDef
    {
        std::string_view name;
    };

    // The model definition is stored in an arena, thus it should be
    // trivially destructible.
    static_assert(std::is_trivially_destructible_v<ModelDef>);

public:
    /// Constructs an empty model repository.
    ///
    /// Use `Builder` to construct a non-empty object.
    ModelManager() noexcept = default;

    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    ModelManager(ModelManager&&) noexcept = default;
    ModelManager& operator=(ModelManager&&) noexcept = default;

    ~ModelManager() noexcept = default;

    /// Finds a model in the repository.
    ///
    /// \returns the model definition or `nullptr` if not found.
    auto find_model(std::string_view name) const -> const ModelDef*;

private:
    using ModelsMap = std::unordered_map<std::string_view, arena_ptr<ModelDef>>;
    ModelsMap models;

protected:
    explicit ModelManager(ModelsMap&& models) noexcept;
};

/// A builder capable of constructing a `ModelManager`.
class ModelManager::Builder
{
public:
    /// Constructs a builder.
    ///
    /// Allocation of model definitions will happen in the given arena.
    /// Therefore the arena should live as long as the constructed model
    /// repository.
    explicit Builder(ArenaMemoryResource& arena) noexcept : arena(&arena) {}

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    Builder(Builder&&) noexcept = default;
    Builder& operator=(Builder&&) noexcept = default;

    ~Builder() noexcept = default;

    /// Inserts a model into the  repository.
    auto insert_model(std::string_view name) -> Builder&&;

    /// Builds the model repository.
    auto build() && -> ModelManager;

private:
    ArenaMemoryResource* arena;
    ModelsMap models;
};
} // namespace gta3sc
