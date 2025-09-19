#include <gta3sc/model-table.hpp>
#include <gta3sc/util/ctype.hpp>
#include <gta3sc/util/memory.hpp>
#include <string_view>
#include <utility>

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

auto ModelTable::Builder::insert_model(std::string_view name, uint32_t id) -> Builder&&
{
    // FIXME this causes a memory growth proportional to number of inserts if all items are dups.

    auto [key, value] = util::new_object_with_string<ModelDef>(
        name, util::toupper, allocator, private_tag, name.size(), id);

    models.insert_or_assign(std::move(key), std::move(value));

    return std::move(*this);
}
} // namespace gta3sc
