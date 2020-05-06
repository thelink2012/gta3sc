#include <gta3sc/model-table.hpp>
#include <gta3sc/util/ctype.hpp>

namespace gta3sc
{
ModelTable::ModelTable(ModelMap&& models) noexcept : models(std::move(models))
{}

auto ModelTable::find_model(std::string_view name) const noexcept
        -> const ModelDef*
{
    auto it = models.find(name);
    return it != models.end() ? it->second : nullptr;
}

auto ModelTable::Builder::build() && -> ModelTable
{
    return ModelTable(std::move(this->models));
}

auto ModelTable::Builder::insert_model(std::string_view name) -> Builder&&
{
    // FIXME this does not handle the case of name being lowercase
    if(this->models.contains(name))
        return std::move(*this);

    models.emplace(util::new_object_with_string<ModelDef>(
            name, util::toupper, allocator, private_tag, name.size()));

    return std::move(*this);
}
} // namespace gta3sc
