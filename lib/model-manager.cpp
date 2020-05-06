#include <gta3sc/model-manager.hpp>
#include <gta3sc/util/arena-utility.hpp>
#include <gta3sc/util/ctype.hpp>

namespace gta3sc
{
ModelManager::ModelManager(ModelsMap&& models) noexcept :
    models(std::move(models))
{}

auto ModelManager::find_model(std::string_view name) const -> const ModelDef*
{
    auto it = models.find(name);
    return it != models.end() ? it->second : nullptr;
}

auto ModelManager::Builder::build() && -> ModelManager
{
    return ModelManager(std::move(this->models));
}

auto ModelManager::Builder::insert_model(std::string_view name) -> Builder&&
{
    // FIXME this does not handle the case of name being lowercase
    if(!this->models.count(name))
    {
        auto name_a = util::new_string(name, allocator, util::toupper);
        auto* model_a = allocator.new_object<ModelDef>(ModelDef{name_a});
        models.emplace(name_a, model_a);
    }
    return std::move(*this);
}
} // namespace gta3sc
