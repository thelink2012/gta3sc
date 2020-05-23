#include <stdinc.h>
#include "symtable.hpp"
#include "commands.hpp"
#include "program.hpp"
#include "codegen.hpp"

shared_ptr<Script> Script::create(fs::path path, ScriptType type, ProgramContext& program)
{
    if(auto tstream = TokenStream::tokenize(program, path))
    {
        if(auto tree = SyntaxTree::compile(program, *tstream))
        {
            auto p = std::shared_ptr<Script>(new Script(program, type, std::move(path), std::move(tstream), std::move(tree)));
            p->start_label = std::make_shared<Label>(nullptr, p->shared_from_this());
            p->top_label = std::make_shared<Label>(nullptr, p->shared_from_this());
            return p;
        }
    }
    return nullptr;
}

auto Script::from_subdir(const string_view& filename, const Script::SubDir& subdir,
                         ScriptType type, ProgramContext& program) const -> shared_ptr<Script>
{
    Expects(this->is_main_script());

    auto path_it = subdir.find(filename);
    if(path_it != subdir.end())
    {
        return Script::create(path_it->second, type, program);
    }
    else
    {
        auto stem = this->path.stem().generic_u8string();
        program.error(nocontext, "file {} does not exist in the '{}' subdirectory", filename, stem);
        return nullptr;
    }
}

uint32_t Var::space_taken(VarType type, size_t count)
{
    switch(type)
    {
        case VarType::Int:
            return 1 * count;
        case VarType::Float:
            return 1 * count;
        case VarType::TextLabel:
            return 2 * count;
        case VarType::TextLabel16:
            return 4 * count;
        default:
            Unreachable();
    }
}

uint32_t Var::space_taken()
{
    return Var::space_taken(this->type, this->count.value_or(1));
}

bool Label::may_branch_from(const Script& other_script, ProgramContext& program) const
{
    auto script = this->script.lock();
    if(!script->uses_local_offsets())
        return true;
    return script->on_the_same_space_as(other_script);
}

auto Scope::output_type_from_node(const SyntaxTree& node) -> optional<OutputType>
{
    if(auto opt_var = get_base_var_annotation(node))
        return (*opt_var)->type;
    else if(node.maybe_annotation<const int32_t&>())
        return OutputType::Int;
    else if(node.maybe_annotation<const float&>())
        return OutputType::Float;
    else if(node.maybe_annotation<const TextLabelAnnotation&>())
        return OutputType::TextLabel;
    else
        return nullopt;
}

shared_ptr<Var> Scope::var_at(size_t index) const
{
    size_t offset = index * 4;
    for(auto& vpair : vars)
    {
        if(offset >= vpair.second->offset()
            && offset < vpair.second->end_offset())
            return vpair.second;
    }
    return nullptr;
}

auto Script::find_maximum_locals() const -> std::pair<uint32_t, uint32_t>
{
    uint32_t highest_offset_genl = 0;
    uint32_t highest_offset_call = 0;

    for(auto& scope : this->scopes)
    {
        auto highest_offset = scope->is_call_scope()? std::ref(highest_offset_call) : std::ref(highest_offset_genl);
        for(auto& var : scope->vars)
        {
            highest_offset.get() = std::max(highest_offset.get(), var.second->end_offset());
        }
    }

    for(auto& child : this->children_scripts)
    {
        auto pair = child.lock()->find_maximum_locals();
        highest_offset_genl = std::max(highest_offset_genl, pair.first * 4);
        highest_offset_call = std::max(highest_offset_call, pair.second * 4);
    }

    return { highest_offset_genl / 4, highest_offset_call / 4 };
}

void Script::add_children(shared_ptr<Script> script)
{
    Expects(script.get() != this);
    Expects(script->type == ScriptType::Required);
    Expects(script->parent_script.use_count() == 0);

    this->children_scripts.emplace_back(script);
    script->parent_script = this->shared_from_this();
}

bool Script::uses_local_offsets() const
{
    switch(this->type)
    {
        case ScriptType::Main:
        case ScriptType::MainExtension:
        case ScriptType::Subscript:
            return false;
        case ScriptType::Mission:
        case ScriptType::StreamedScript:
        case ScriptType::CustomMission:
        case ScriptType::CustomScript:
            return true;
        case ScriptType::Required:
            return this->root_script()->uses_local_offsets();
        default:
            Unreachable();
    }
}

bool Script::on_the_same_space_as(const Script& other) const
{
    if(this == &other)
        return true;

    if(this->type == ScriptType::Required)
        return this->root_script()->on_the_same_space_as(other);

    if(other.type == ScriptType::Required)
        return this->on_the_same_space_as(*other.root_script());

    if(this->type == ScriptType::Main
        || this->type == ScriptType::MainExtension
        || this->type == ScriptType::Subscript)
    {
        switch(other.type)
        {
            case ScriptType::Main:
            case ScriptType::MainExtension:
            case ScriptType::Subscript:
                return true;
            default:
                return false;
        }
    }
    else if(this->type == ScriptType::Mission
        || this->type == ScriptType::StreamedScript
        || this->type == ScriptType::CustomScript
        || this->type == ScriptType::CustomMission)
    {
        return this == &other;
    }
    else
    {
        Unreachable();
    }
}

bool Script::is_main_script() const
{
    return this->type == ScriptType::Main
        || this->type == ScriptType::CustomScript
        || this->type == ScriptType::CustomMission;
}

bool Script::is_child_of(ScriptType type) const
{
    Expects(type != ScriptType::Required);
    if(this->type == ScriptType::Required)
        return this->root_script()->is_child_of(type);
    else
        return this->type == type;
}

bool Script::is_child_of_mission() const
{
    return is_child_of(ScriptType::Mission) || is_child_of(ScriptType::CustomMission);
}

bool Script::is_child_of_custom() const
{
    return is_child_of(ScriptType::CustomScript) || is_child_of(ScriptType::CustomMission);
}

shared_ptr<const Script> Script::root_script() const
{
    if(!this->is_root_script())
        return this->parent_script.lock()->root_script();
    else
        return this->shared_from_this();
}

bool Script::is_root_script() const
{
    return !(this->type == ScriptType::Required);
}

size_t Script::distance_from_root() const
{
    if(this->is_root_script())
        return 0;

    auto parent = this->parent_script.lock();
    return (this->code_offset.value() - parent->code_offset.value()) + parent->distance_from_root();
}

auto Script::scan_subdir() const -> Script::SubDir
{
    auto output = insensitive_map<std::string, fs::path>();
    auto subdir = this->path.parent_path() / this->path.stem();

    if(fs::exists(subdir) && fs::is_directory(subdir))
    {
        for(auto& entry : fs::recursive_directory_iterator(subdir))
        {
            auto filename = entry.path().filename().generic_u8string();
            output.emplace(std::move(filename), entry.path());
        }
    }

    return output;
}

void Script::compute_script_offsets(const std::vector<shared_ptr<Script>>& scripts, const MultiFileHeaderList& headers)
{
    size_t offset = 0;
    for(auto& script_ptr : scripts)
    {
        assert(script_ptr->base == nullopt && script_ptr->code_offset == nullopt);
        script_ptr->base.emplace(offset);
        offset += headers.compiled_size(script_ptr);
        script_ptr->code_offset.emplace(offset);
        offset += script_ptr->code_size.value();
    }
}

auto Script::compute_used_objects(const std::vector<shared_ptr<Script>>& scripts) -> std::vector<std::string>
{
    std::vector<std::string> models;

    for(auto& script : scripts)
    {
        for(auto& umodel : script->models)
        {
            auto it = std::find_if(models.begin(), models.end(), [&](const std::string& m) {
                return iequal_to()(m, umodel.first);
            });

            if(it == models.end())
            {
                if(models.capacity() == 0)
                    models.reserve(200);

                models.emplace_back(umodel.first);
                it = models.begin() + (models.size() - 1);
            }

            umodel.second = -(1 + std::distance(models.begin(), it));
        }
    }

    return models;
}

void Script::handle_special_commands(const std::vector<shared_ptr<Script>>& scripts, SymTable& symbols, ProgramContext& program)
{
    std::vector<std::pair<shared_ptr<const Scope>, size_t>> scope_num_inputs;
    shared_ptr<const Scope> last_scope_entered;
    insensitive_map<std::string, shared_ptr<const SyntaxTree>> script_names;

    shared_ptr<SyntaxTree> node_set_progress_total;
    shared_ptr<SyntaxTree> node_set_total_number_of_missions;
    shared_ptr<SyntaxTree> node_set_collectable1_total;
    shared_ptr<SyntaxTree> node_set_mission_respect_total;

    int32_t count_collectable1 = 0;
    int32_t count_mission_passed = 0;
    int32_t count_progress = 0;
    int32_t count_respect = 0;

    auto handle_script_input = [&program](const SyntaxTree& arg_node, const shared_ptr<Var>& lvar, bool is_cleo_call) -> bool
    {
        if(auto opt_arg_var = get_base_var_annotation(arg_node))
        {
            auto& argvar = *opt_arg_var;

            if(argvar->type != lvar->type
                && !(false && is_cleo_call && argvar->is_text_var() && lvar->type == VarType::Int))
            {
                program.error(arg_node, "type mismatch in target label");
                // TODO more info
                return false;
            }

            if(lvar->entity && argvar->entity != lvar->entity)
            {
                program.error(arg_node, "entity type mismatch in target label");
                // TODO more info
                return false;
            }

            lvar->entity = argvar->entity;
            return true;
        }
        else
        {
            program.error(arg_node, "type mismatch in target label");
            // TODO more info
            return false;
        }
    };

    auto send_input_vars = [&](const SyntaxTree::const_iterator& input_begin,
        const SyntaxTree::const_iterator& input_end,
        shared_ptr<const Scope>& target_scope, bool is_cleo_call)
    {
        size_t target_var_index = 0;
        for(auto arginput = input_begin; arginput != input_end; ++arginput)
        {
            auto lvar = target_scope->var_at(target_var_index);

            if(!lvar)
            {
                program.error(**arginput, "not enough local variables in target label");
                break;
            }

            switch(lvar->type)
            {
                case VarType::Int:
                    ++target_var_index;
                    if((**arginput).maybe_annotation<const int32_t&>())
                        break;
                    if(is_cleo_call && (**arginput).maybe_annotation<const TextLabelAnnotation&>())
                    {
                        program.error(**arginput, "CLEO_CALL semantics for TEXT_LABEL inputs are still unspecified");
                        break;
                    }
                    handle_script_input(**arginput, lvar, is_cleo_call);
                    break;
                case VarType::Float:
                    ++target_var_index;
                    if((**arginput).maybe_annotation<const float&>())
                        break;
                    handle_script_input(**arginput, lvar, is_cleo_call);
                    break;
                case VarType::TextLabel:
                    target_var_index += 2;
                    program.error(**arginput, "type mismatch in target label because LVAR_TEXT_LABEL is not allowed");
                    break;
                case VarType::TextLabel16:
                    target_var_index += 4;
                    program.error(**arginput, "type mismatch in target label because LVAR_TEXT_LABEL16 is not allowed");
                    break;
                default:
                    Unreachable();
            }
        }
    };

    auto recv_output_vars = [&](const SyntaxTree::const_iterator& output_begin,
        const SyntaxTree::const_iterator& output_end,
        shared_ptr<const Scope>& target_scope, bool is_cleo_call)
    {
        assert(is_cleo_call == true);

        size_t i = 0;
        for(auto argoutput = output_begin; argoutput != output_end; ++argoutput, ++i)
        {
            if(auto opt_outvar = get_base_var_annotation(**argoutput))
            {
                auto& outvar = *opt_outvar;
                auto& scope_output = (*target_scope->outputs)[i];
                auto output_type = scope_output.first;
                auto output_entity = scope_output.second.expired()? 0 : scope_output.second.lock()->entity;

                assert(output_type == Scope::OutputType::Int || output_type == Scope::OutputType::Float);

                if(output_type != outvar->type)
                {
                    program.error(**argoutput, "type mismatch");
                    // TODO more info
                    continue;
                }

                if(outvar->entity && output_entity != outvar->entity)
                {
                    program.error(**argoutput, "entity type mismatch");
                    // TODO more info
                    continue;
                }

                outvar->entity = output_entity;
            }
            else
            {
                program.error(**argoutput, "expected a variable as output");
            }
        }
    };

    auto handle_start_new_script = [&](const SyntaxTree& node, const Command& command)
    {
        if(!program.opt.entity_tracking || node.child_count() < 2)
            return;

        auto& arglabel_node = node.child(1);

        auto opt_target_label = arglabel_node.maybe_annotation<const shared_ptr<Label>&>();
        if(!opt_target_label)
        {
            program.warning(arglabel_node, "target label is not a label identifier");
            return;
        }

        auto& target_scope = (*opt_target_label)->scope;
        if(!target_scope)
        {
            auto where = program.opt.scope_then_label? "before" : "after";
            program.error(arglabel_node, "expected scope in target label");
            program.note(arglabel_node, "add a '{{' {} the target label", where);
            return;
        }

        send_input_vars(std::next(node.begin(), 2), node.end(), target_scope, false);
    };

    auto handle_start_new_streamed_script = [&](const SyntaxTree& node, const Command& command)
    {
        if(!program.opt.entity_tracking || node.child_count() < 2)
            return;

        // TODO
        return;
    };

    auto handle_entity_command = [&](const SyntaxTree& node, const Command& command)
    {
        if(!program.opt.entity_tracking)
            return;

        size_t i = 0;
        for(auto it = std::next(node.begin()); it != node.end(); ++it, ++i)
        {
            auto& arginfo = command.arg(i).value();
            if(arginfo.entity_type != 0)
            {
                if(auto opt_argvar = get_base_var_annotation(**it))
                {
                    auto& argvar = **opt_argvar;
                    if(arginfo.is_output)
                    {
                        if(argvar.entity && argvar.entity != arginfo.entity_type)
                        {
                            auto type_old = program.commands.find_entity_name(argvar.entity).value();
                            auto type_new = program.commands.find_entity_name(arginfo.entity_type).value();
                            program.error(**it, "variable has already been used to create a entity of type {}", type_old);
                        }

                        argvar.entity = arginfo.entity_type;
                    }
                    else
                    {
                        if(argvar.entity != arginfo.entity_type)
                        {
                            auto expect = program.commands.find_entity_name(arginfo.entity_type).value();
                            auto got = program.commands.find_entity_name(argvar.entity).value();
                            program.error(**it, "expected variable of type {} but got {}", expect, got);
                        }
                    }
                }
            }
        }
    };

    auto handle_cleo_call = [&](SyntaxTree& node, const Command& command)
    {
        if(node.child_count() < 3)
            return;

        auto& arglabel_node = node.child(1);
        auto& argcount_node = node.child(2);

        auto opt_target_label = arglabel_node.maybe_annotation<const shared_ptr<Label>&>();
        if(!opt_target_label)
        {
            program.warning(arglabel_node, "target label is not a label identifier");
            return;
        }

        auto& target_scope = (*opt_target_label)->scope;
        if(!target_scope)
        {
            auto where = program.opt.scope_then_label? "before" : "after";
            program.error(arglabel_node, "expected scope in target label");
            program.note(arglabel_node, "add a '{{' {} the target label", where);
            return;
        }

        if(!target_scope->is_call_scope())
        {
            program.error(arglabel_node, "target scope does not have a CLEO_RETURN");
            return;
        }

        if(argcount_node.maybe_annotation<int32_t>().value_or(99) != 0)
            program.error(argcount_node, "this argument shall be set to 0");

        auto& outputs = *target_scope->outputs;
        size_t num_args = node.child_count() - 3;

        if(num_args < outputs.size())
        {
            program.error(node, "too few arguments for this function call");
            return;
        }

        size_t num_inputs = num_args - outputs.size();

        // TODO maybe store it in the scope object
        auto it_num_input = std::find_if(scope_num_inputs.begin(), scope_num_inputs.end(), [&](const auto& v) {
            return v.first == target_scope;
        });

        if(it_num_input == scope_num_inputs.end())
        {
            scope_num_inputs.emplace_back(std::make_pair(target_scope, num_inputs));
        }
        else if(num_inputs < it_num_input->second)
        {
            program.error(node, "too few arguments for this function call");
            return;
        }
        else if(num_inputs > it_num_input->second)
        {
            program.error(node, "too many arguments for this function call");
            return;
        }

        send_input_vars(std::next(node.begin(), 3), std::next(node.begin(), 3 + num_inputs), target_scope, true);
        recv_output_vars(std::next(node.begin(), 3 + num_inputs), node.end(), target_scope, true);

        argcount_node.set_annotation(int32_t(num_inputs));
    };

    auto handle_cleo_return = [&](SyntaxTree& node, const Command& command)
    {
        if(node.child_count() < 2)
            return;

        auto& outputs = last_scope_entered->outputs.value();

        size_t num_outputs = node.child_count() - 2;

        auto& argcount_node = node.child(1);

        if(argcount_node.maybe_annotation<int32_t>().value_or(99) != 0)
            program.error(argcount_node, "this argument shall be set to 0");

        argcount_node.set_annotation(int32_t(num_outputs));

        if(num_outputs > outputs.size())
        {
            program.error(node, "too many return arguments");
            return;
        }
        else if(num_outputs < outputs.size())
        {
            program.error(node, "too few return arguments");
            return;
        }

        size_t i = 0;
        for(auto it = std::next(node.begin(), 2); it != node.end(); ++it, ++i)
        {
            auto& output = outputs[i];
            if(auto opt_type = Scope::output_type_from_node(**it))
            {
                if(*opt_type != output.first)
                {
                    program.error(**it, "type mismatch");
                    // TODO more info
                    continue;
                }

                if(auto opt_var = get_base_var_annotation(**it))
                {
                    if(!output.second.expired())
                    {
                        auto output_var = output.second.lock();
                        auto& return_var = *opt_var;

                        if(output_var->entity != return_var->entity)
                        {
                            program.error(**it, "entity type mismatch");
                            // TODO more info
                            continue;
                        }
                    }
                }
            }
            else
            {
                program.error(node, "unrecognized output type");
            }
        }
    };

    auto handle_script_name = [&](const SyntaxTree& node, const Command&)
    {
        if(!program.opt.script_name_check)
            return;

        if(node.child_count() == 2)
        {
            if(auto opt_script_name = node.child(1).maybe_annotation<const TextLabelAnnotation&>())
            {
                auto pair = script_names.emplace(opt_script_name->string, node.shared_from_this());
                if(!pair.second)
                {
                    program.error(node, "duplicate script name {}", opt_script_name->string);
                    program.note(*pair.first->second, "previously seen here");
                }
            }
        }
    };

    auto try_handle_set_total = [&](SyntaxTree& node, const Command& command,
                                   const optional<const Command&>& expected,
                                   shared_ptr<SyntaxTree>& had_node)
    {
        if(program.commands.equal(command, expected))
        {
            if(had_node)
            {
                program.error(node, "{} happens multiple times", command.name);
                program.note(*had_node, "previously seen here");
            }
            else
            {
                had_node = node.shared_from_this();
            }
            return true;
        }
        return false;
    };

    auto set_total_annotation = [&](shared_ptr<SyntaxTree>& node, int32_t count)
    {
        if(node && node->child_count() >= 2)
        {
            auto& count_node = node->child(1);
            if(count_node.maybe_annotation<int32_t>() != 0)
            {
                program.error(*node, "set total argument must be 0, "
                                     "the compiler will tweak the value automatically");
            }
            count_node.set_annotation(int32_t(count));
        }
    };

    auto try_handle_increase_counter = [&](SyntaxTree& node, const Command& command,
                                           const optional<const Command&>& expected,
                                           int32_t& counter)
    {
        if(program.commands.equal(command, expected))
        {
            if(node.child_count() >= 2)
            {
                if(auto inc = node.child(1).maybe_annotation<int32_t>())
                    counter += *inc;
                else
                    program.warning(node, "value is not a constant");
            }
            return true;
        }
        return false;
    };

    for(auto& script : scripts)
    {
        script->tree->depth_first([&](SyntaxTree& node)
        {
            switch(node.type())
            {
                case NodeType::Scope:
                {
                    // scope checking already happened at this point, so no need for handling entering/exiting
                    last_scope_entered = node.annotation<shared_ptr<Scope>>();
                    return true;
                }

                case NodeType::Command:
                {
                    if(auto opt_command = node.maybe_annotation<std::reference_wrapper<const Command>>())
                    {
                        const bool is_child_of_custom = script->is_child_of_custom();
                        const bool is_child_of_custom_script = script->is_child_of(ScriptType::CustomScript);

                        auto& command = (*opt_command).get();
                        if(program.commands.equal(command, program.commands.script_name))
                        {
                            handle_script_name(node, command);
                        }
                        else if(try_handle_set_total(node, command, program.commands.set_progress_total, node_set_progress_total)
                             || try_handle_set_total(node, command, program.commands.set_total_number_of_missions, node_set_total_number_of_missions)
                             || try_handle_set_total(node, command, program.commands.set_collectable1_total, node_set_collectable1_total)
                             || try_handle_set_total(node, command, program.commands.set_mission_respect_total, node_set_mission_respect_total))
                            {}
                        else if(program.commands.equal(command, program.commands.register_mission_passed)
                             || program.commands.equal(command, program.commands.register_oddjob_mission_passed))
                            { ++count_mission_passed; }
                        else if(program.commands.equal(command, program.commands.create_collectable1))
                            { ++count_collectable1; }
                        else if(try_handle_increase_counter(node, command, program.commands.player_made_progress, count_progress)
                             || try_handle_increase_counter(node, command, program.commands.award_player_mission_respect, count_respect))
                            {}
                        else if(is_child_of_custom && program.commands.equal(command, program.commands.start_new_script))
                        {
                            program.error(node, "this command is not allowed in {} scripts", to_string(script->type));
                        }
                        else if(is_child_of_custom_script
                            && program.commands.equal(command, program.commands.terminate_this_script))
                        {
                            program.error(node, "this command is not allowed in {} scripts", to_string(script->type));
                        }
                        else if(!is_child_of_custom_script
                            && program.commands.equal(command, program.commands.terminate_this_custom_script))
                        {
                            program.error(node, "this command is not allowed in {} scripts", to_string(script->type));
                        }
                        else if(program.commands.equal(command, program.commands.start_new_script))
                        {
                            handle_start_new_script(node, command);
                        }
                        else if(program.commands.equal(command, program.commands.start_new_streamed_script))
                        {
                            handle_start_new_streamed_script(node, command);
                        }
                        else if(program.commands.equal(command, program.commands.cleo_call))
                        {
                            handle_cleo_call(node, command);
                        }
                        else if(program.commands.equal(command, program.commands.cleo_return))
                        {
                            handle_cleo_return(node, command);
                        }
                        else
                        {
                            handle_entity_command(node, command);
                        }
                    }
                    return false;
                }

                case NodeType::Equal:
                {
                    auto& a = node.child(0);
                    auto& b = node.child(1);

                    // handle only a = b, not a = b op c (TODO that should be handled as well).
                    if(!b.maybe_annotation<std::reference_wrapper<const Command>>())
                    {
                        auto& command = node.annotation<std::reference_wrapper<const Command>>().get();
                        if(program.commands.is_alternator(command, program.commands.set))
                        {
                            auto opt_avar = get_base_var_annotation(a);
                            auto opt_bvar = get_base_var_annotation(b);
                            if(opt_avar && opt_bvar)
                            {
                                auto& avar = **opt_avar;
                                auto& bvar = **opt_bvar;

                                if(avar.entity && avar.entity != bvar.entity)
                                {
                                    auto type_a = program.commands.find_entity_name(avar.entity).value();
                                    auto type_b = program.commands.find_entity_name(bvar.entity).value();
                                    program.error(node, "assignment of variable of type {} into one of type {}", type_b, type_a);
                                }

                                avar.entity = bvar.entity;
                            }
                        }
                    }

                    return false;
                }

                default:
                    return true;
            }
        });
    }

    set_total_annotation(node_set_collectable1_total, count_collectable1);
    set_total_annotation(node_set_total_number_of_missions, count_mission_passed);
    set_total_annotation(node_set_progress_total, count_progress);
    set_total_annotation(node_set_mission_respect_total, count_respect);
}

void Script::compute_scope_outputs(const SymTable& symbols, ProgramContext& program)
{
    if(!program.opt.cleo)
        return;

    for(auto& scope : this->scopes)
    {
        scope->tree.lock()->depth_first([&](const SyntaxTree& node)
        {
            // already found the outputs for this scope
            if(scope->outputs)
                return false;

            switch(node.type())
            {
                case NodeType::Command:
                {
                    if(auto opt_command = node.maybe_annotation<std::reference_wrapper<const Command>>())
                    {
                        if(!program.commands.equal(*opt_command, program.commands.cleo_return))
                            return false;

                        if(node.child_count() < 2)
                        {
                            program.error(node, "CLEO_RETURN must have at least one parameter, which is a 0 placeholder");
                            return false;
                        }

                        scope->outputs.emplace();
                        auto& outputs = *scope->outputs;

                        for(auto it = std::next(node.begin(), 2); it != node.end(); ++it)
                        {
                            if((*it)->type() == NodeType::Text)
                            {
                                auto opt_match = Miss2Identifier::match((*it)->text(), program.opt);
                                if(opt_match)
                                {
                                    string_view varname;
                                    if(opt_match->identifier.front() == '$')
                                        varname = opt_match->identifier.substr(1);
                                    else
                                        varname = opt_match->identifier;

                                    if(auto opt_var = symbols.find_var(varname, scope))
                                    {
                                        if((*opt_var)->is_text_var() && opt_match->identifier.front() != '$')
                                            outputs.emplace_back(Scope::OutputType::TextLabel, *opt_var);
                                        else
                                            outputs.emplace_back((*opt_var)->type, *opt_var);
                                    }
                                    else
                                        outputs.emplace_back(Scope::OutputType::TextLabel, weak_ptr<Var>());

                                    if(outputs.back().first == Scope::OutputType::TextLabel)
                                        program.error(**it, "this output type is not supported");
                                }
                                else
                                {
                                    program.error(**it, ::to_string(opt_match.error()));
                                }
                            }
                            else if((*it)->type() == NodeType::Integer)
                            {
                                outputs.emplace_back(Scope::OutputType::Int, weak_ptr<Var>());
                            }
                            else if((*it)->type() == NodeType::Float)
                            {
                                outputs.emplace_back(Scope::OutputType::Float, weak_ptr<Var>());
                            }
                            else if((*it)->type() == NodeType::String)
                            {
                                program.error(**it, "this output type is not supported");
                            }
                            else
                            {
                                program.error(**it, "unrecognized output type");
                            }
                        }
                    }
                    return false;
                }

                default:
                    return true;
            }
        });
    }
}

void Script::fix_call_scope_variables(ProgramContext& program)
{
    if(program.opt.mission_var_begin == 0 || !this->is_child_of_mission())
        return;

    for(auto& scope : this->scopes)
    {
        if(scope->is_call_scope())
        {
            for(auto& var : scope->vars)
            {
                auto& var_index = var.second->index;

                if(var_index >= uint32_t(program.opt.timer_index) && var_index <= uint32_t(program.opt.timer_index + 1))
                    continue;

                assert(var_index >= program.opt.mission_var_begin);
                var_index -= program.opt.mission_var_begin;
            }
        }
    }
}

// Script::annotate_tree is within symtable.cpp
