#include <gta3sc/model-manager.hpp>

namespace gta3sc
{
inline ModelManager::ModelManager(ModelManager::Builder&& builder) :
    models(std::move(builder.models))
{}

auto ModelManager::find_model(std::string_view name) const -> const ModelDef*
{
    auto it = models.find(name);
    return it != models.end() ? it->second : nullptr;
}

auto ModelManager::Builder::build() && -> ModelManager
{
    return ModelManager(std::move(*this));
}

auto ModelManager::Builder::insert_model(std::string_view name) -> Builder&&
{
    auto name_a = util::allocate_string_upper(name, *arena);
    auto model_a = new(*arena, alignof(ModelDef)) ModelDef{name_a};
    models.emplace(name_a, model_a);
    return std::move(*this);
}
}
