#include <stdinc.h>
#include "symtable.hpp"
#include "commands.hpp"
#include "program.hpp"
#include "codegen.hpp"

auto SymTable::from_script(Script& script, ProgramContext& program) -> SymTable
{
    SymTable symbols;
    symbols.scan_symbols(script, program);
    return symbols;
}

auto IncluderTable::from_script(const Script& script, ProgramContext& program) -> IncluderTable
{
    IncluderTable ictable;
    ictable.scan_for_includers(script, program);
    return ictable;
}

size_t SymTable::size_global_vars() const
{
    if(auto highest_var = this->highest_global_var())
        return (*highest_var)->end_offset();
    return this->offset_global_vars;
}

void SymTable::apply_offset_to_vars(uint32_t indices)
{
    this->offset_global_vars += indices * 4;
    for(auto& var : global_vars)
        var.second->index += indices;
}

optional<shared_ptr<Var>> SymTable::find_var(const string_view& name, const shared_ptr<Scope>& current_scope) const
{
    auto it = global_vars.find(name);
    if(it != global_vars.end())
        return it->second;

    if(current_scope)
    {
        auto it = current_scope->vars.find(name);
        if(it != current_scope->vars.end())
            return it->second;
    }

    return nullopt;
}

optional<shared_ptr<Label>> SymTable::find_label(const string_view& name) const
{
    auto it = this->labels.find(name);
    if(it != this->labels.end())
        return it->second;
    return nullopt;
}

optional<shared_ptr<Script>> SymTable::find_script(const string_view& filename) const
{
    auto it = this->scripts.find(filename);
    if(it != this->scripts.end())
        return it->second;
    return nullopt;
}

optional<uint16_t> SymTable::find_streamed_id(const string_view& stream_constant) const
{
    auto& streamed_names = this->ictable.streamed_names;
    auto it = std::find_if(streamed_names.begin(), streamed_names.end(), [&](const auto& constant) {
        return iequal_to()(constant, stream_constant);
    });
    if(it != streamed_names.end())
        return uint16_t(it - streamed_names.begin());
    return nullopt;
}

optional<const UserConstant&> SymTable::find_constant(const string_view& name) const
{
    auto it = this->constants.find(name);
    if(it != this->constants.end())
        return it->second;
    return nullopt;
}

auto IncluderTable::script_type(const string_view& filename) const -> optional<ScriptType>
{
    auto searcher = [&](const std::string& other) {
        return iequal_to()(other, filename);
    };

    if(std::any_of(this->extfiles.begin(), this->extfiles.end(), searcher))
        return ScriptType::MainExtension;
    else if(std::any_of(this->subscript.begin(), this->subscript.end(), searcher))
        return ScriptType::Subscript;
    else if(std::any_of(this->mission.begin(), this->mission.end(), searcher))
        return ScriptType::Mission;
    else if(std::any_of(this->streamed.begin(), this->streamed.end(), searcher))
        return ScriptType::StreamedScript;
    else if(std::any_of(this->required.begin(), this->required.end(), searcher))
        return ScriptType::Required;
    else
        return nullopt;
}

bool IncluderTable::add_script(ScriptType type, const SyntaxTree& command, ProgramContext& program)
{
    size_t num_required_childs = 1 + (type == ScriptType::MainExtension? 2 :
                                      type == ScriptType::StreamedScript? 2 : 1);

    if(command.child_count() != num_required_childs)
    {
        Commands::MatchFailure {
            command.shared_from_this(),
            command.child_count() < num_required_childs? Commands::MatchFailure::TooFewArgs :
                                                         Commands::MatchFailure::TooManyArgs
        }.emit(program);
        return false;
    }
    else
    {
        size_t name_child_id = num_required_childs - 1;
        auto script_name = command.child(name_child_id).text();

        if(auto existing_type = this->script_type(script_name))
        {
            if(type != *existing_type)
            {
                program.error(command, "incompatible declaration, script was first seen as {}", to_string(*existing_type));
                return false;
            }
            return true;
        }
        else 
        {
            auto refvector = (type == ScriptType::MainExtension? std::ref(this->extfiles) :
                type == ScriptType::Subscript? std::ref(this->subscript) :
                type == ScriptType::Mission? std::ref(this->mission) :
                type == ScriptType::StreamedScript? std::ref(this->streamed) :
                type == ScriptType::Required? std::ref(this->required) : Unreachable());

            if(script_name.size() <= 3
                || strncasecmp(script_name.end() - 3, ".sc", 3) != 0)
            {
                program.error(command.child(name_child_id), "script file extension must be .sc");
            }

            if(type == ScriptType::StreamedScript)
            {
                this->streamed_names.emplace_back(command.child(1).text());
            }

            refvector.get().emplace_back(script_name);
            return true;
        }
    }
}

optional<shared_ptr<Var>> SymTable::highest_global_var() const
{
    auto fn_comp = [](const auto& apair, const auto& bpair)
    {
        const shared_ptr<Var>& a = apair.second;
        const shared_ptr<Var>& b = bpair.second;
        if(a->index < b->index)
            return true;
        else if(a->index != b->index)
            return false;
        else
            return a->space_taken() < b->space_taken();
    };

    auto it = std::max_element(this->global_vars.begin(), this->global_vars.end(), fn_comp);
    if(it == this->global_vars.end())
        return nullopt;
    return it->second;
}

void SymTable::build_script_table(const std::vector<shared_ptr<Script>>& scripts)
{
    for(auto& script : scripts)
    {
        auto name = script->path.filename().u8string();
        std::transform(name.begin(), name.end(), name.begin(), toupper_ascii);
        assert(script->is_main_script() || this->ictable.script_type(name) != nullopt);
        this->scripts.emplace(std::move(name), script);
    }
}

void SymTable::merge(SymTable&& t2, ProgramContext& program)
{
    auto& t1 = *this;

    decltype(labels) int_labels;
    decltype(global_vars) int_gvars;
    decltype(constants) int_constants;

    // finds items that are common in both
    std::set_intersection(t1.labels.begin(), t1.labels.end(),
        t2.labels.begin(), t2.labels.end(),
        std::inserter(int_labels, int_labels.begin()),
        t1.labels.value_comp());

    // finds items that are common in both
    std::set_intersection(t1.global_vars.begin(), t1.global_vars.end(),
        t2.global_vars.begin(), t2.global_vars.end(),
        std::inserter(int_gvars, int_gvars.begin()),
        t1.global_vars.value_comp());

    // finds items that are common in both
    std::set_intersection(t1.constants.begin(), t1.constants.end(),
        t2.constants.begin(), t2.constants.end(),
        std::inserter(int_constants, int_constants.begin()),
        t1.constants.value_comp());

    for(auto& kv : int_labels)
    {
        auto where1 = t1.labels.find(kv.first)->second->where;
        auto where2 = t2.labels.find(kv.first)->second->where;
        program.error(where2, "label name exists already");
        program.note(where1, "previously defined here");
    }

    for(auto& kv : int_gvars)
    {
        auto where1 = t1.global_vars.find(kv.first)->second->where;
        auto where2 = t2.global_vars.find(kv.first)->second->where;
        program.error(where2, "variable name exists already");
        program.note(where1, "previously defined here");
    }

    for(auto& kv : int_constants)
    {
        auto where1 = t1.constants.find(kv.first)->second.where;
        auto where2 = t2.constants.find(kv.first)->second.where;
        program.error(where2, "user constant exists already");
        program.note(where1, "previously defined here");
    }

    // All error conditions checked, perform actual merge

    uint32_t begin_t2_vars = t1.size_global_vars() / 4;
    t2.apply_offset_to_vars(begin_t2_vars);

    t1.scripts.insert(std::make_move_iterator(t2.scripts.begin()),
        std::make_move_iterator(t2.scripts.end()));

    t1.labels.insert(std::make_move_iterator(t2.labels.begin()),
        std::make_move_iterator(t2.labels.end()));

    t1.global_vars.insert(std::make_move_iterator(t2.global_vars.begin()),
        std::make_move_iterator(t2.global_vars.end()));

    t1.local_scopes.reserve(t1.local_scopes.size() + t2.local_scopes.size());
    std::move(t2.local_scopes.begin(), t2.local_scopes.end(), std::back_inserter(t1.local_scopes));

    t1.constants.insert(std::make_move_iterator(t2.constants.begin()),
        std::make_move_iterator(t2.constants.end()));

    t1.ictable.merge(std::move(t2.ictable), program);
}

void IncluderTable::merge(IncluderTable&& t2, ProgramContext& program)
{
    auto& t1 = *this;

    auto check_file_conflicts = [&](ScriptType type, const std::vector<std::string>& t2_vec)
    {
        for(auto& filename : t2_vec)
        {
            if(auto existing_type = this->script_type(filename))
            {
                if(*existing_type != type)
                    program.error(nocontext, "script {} was first seen as {} then as {}", filename, to_string(*existing_type), to_string(type));
            }
        }
    };

    check_file_conflicts(ScriptType::Required, t2.required);
    check_file_conflicts(ScriptType::MainExtension, t2.extfiles);
    check_file_conflicts(ScriptType::Subscript, t2.subscript);
    check_file_conflicts(ScriptType::Mission, t2.mission);
    check_file_conflicts(ScriptType::StreamedScript, t2.streamed);

    t1.required.reserve(t1.required.size() + t2.required.size());
    std::move(t2.required.begin(), t2.required.end(), std::back_inserter(t1.required));

    t1.extfiles.reserve(t1.extfiles.size() + t2.extfiles.size());
    std::move(t2.extfiles.begin(), t2.extfiles.end(), std::back_inserter(t1.extfiles));

    t1.subscript.reserve(t1.subscript.size() + t2.subscript.size());
    std::move(t2.subscript.begin(), t2.subscript.end(), std::back_inserter(t1.subscript));

    t1.mission.reserve(t1.mission.size() + t2.mission.size());
    std::move(t2.mission.begin(), t2.mission.end(), std::back_inserter(t1.mission));

    t1.streamed.reserve(t1.streamed.size() + t2.streamed.size());
    std::move(t2.streamed.begin(), t2.streamed.end(), std::back_inserter(t1.streamed));

    t1.streamed_names.reserve(t1.streamed_names.size() + t2.streamed_names.size());
    std::move(t2.streamed_names.begin(), t2.streamed_names.end(), std::back_inserter(t1.streamed_names));
}

void SymTable::check_scope_collisions(ProgramContext& program) const
{
    // XXX this method would probably benefit from parallelism

    decltype(global_vars) int_vars;

    for(auto& scope : this->local_scopes)
    {
        int_vars.clear();

        // finds items that are common in both
        std::set_intersection(global_vars.begin(), global_vars.end(),
            scope->vars.begin(), scope->vars.end(),
            std::inserter(int_vars, int_vars.begin()),
            global_vars.value_comp());

        for(auto& kv : int_vars)
        {
            auto where1 = global_vars.find(kv.first)->second->where;
            auto where2 = scope->vars.find(kv.first)->second->where;
            program.error(where2, "variable name exists already");
            program.note(where1, "previously defined here");
        }
    }
}

void SymTable::check_constant_collisions(ProgramContext& program) const
{
    if(!program.opt.constant_checks)
        return;

    // XXX this method would probably benefit from parallelism

    auto has_constant_with_name = [&](const string_view& name) {
        return program.commands.find_constant_all(name) || program.is_model_from_ide(name);
    };

    for(auto& kv : this->global_vars)
    {
        if(this->find_constant(kv.first) || has_constant_with_name(kv.first))
            program.error(kv.second->where, "variable name exists already as a string constant");
    }

    for(auto& scope : this->local_scopes)
    {
        for(auto& kv : scope->vars)
        {
            if(this->find_constant(kv.first) || has_constant_with_name(kv.first))
                program.error(kv.second->where, "variable name exists already as a string constant");
        }
    }

    for(auto& kv : this->constants)
    {
        if(has_constant_with_name(kv.first))
            program.error(kv.second.where, "user constant exists already as a string constant");
    }
}

//////////////////////////////////////////
// Syntax Tree Traversal
//////////////////////////////////////////

void IncluderTable::scan_for_includers(const Script& script, ProgramContext& program)
{
    auto add_script = [&](ScriptType type, const SyntaxTree& command)
    {
        if(type == ScriptType::Required && script.type == ScriptType::Required)
        {
            program.error(command, "using REQUIRE inside a required file is forbidden");
        }
        else if(type == ScriptType::Required || script.type == ScriptType::Main || script.type == ScriptType::MainExtension)
        {
            this->add_script(type, command, program);
        }
        else if(script.is_child_of_custom())
        {
            program.error(command, "this command is not allowed in custom scripts");
        }
        else
        {
            auto bad_message = "use of this command outside main or extension files is not well-defined";
            if(type == ScriptType::Mission || type == ScriptType::StreamedScript)
                program.error(command, bad_message);
            else
                program.warning(command, bad_message);
        }
    };

    script.tree->depth_first([&](SyntaxTree& node)
    {
        switch(node.type())
        {
            case NodeType::Command:
            {
                auto command_name = node.child(0).text();

                if(iequal_to()(command_name, "GOSUB_FILE"))
                    add_script(ScriptType::MainExtension, node);
                else if(iequal_to()(command_name, "LAUNCH_MISSION"))
                    add_script(ScriptType::Subscript, node);
                else if(iequal_to()(command_name, "LOAD_AND_LAUNCH_MISSION"))
                    add_script(ScriptType::Mission, node);
                else if(iequal_to()(command_name, "REGISTER_STREAMED_SCRIPT"))
                    add_script(ScriptType::StreamedScript, node);
                else if(iequal_to()(command_name, "REQUIRE"))
                {
                    program.pedantic(node, "REQUIRE is a language extension [-pedantic]");
                    add_script(ScriptType::Required, node);
                }

                return false;
            }

            default:
                return true;
        }
    });
}

void SymTable::scan_symbols(Script& script, ProgramContext& program)
{
    std::function<bool(SyntaxTree&)> walker;

    shared_ptr<Scope> current_scope;
    shared_ptr<SyntaxTree> next_scoped_label;
    size_t global_index = 0, local_index = 0;

    auto token_to_vartype = [&](NodeType token_type)
    {
        switch(token_type)
        {
            case NodeType::VAR_INT:
                return std::make_pair(true, VarType::Int);
            case NodeType::VAR_FLOAT:
                return std::make_pair(true, VarType::Float);
            case NodeType::VAR_TEXT_LABEL:
                return std::make_pair(true, VarType::TextLabel);
            case NodeType::VAR_TEXT_LABEL16:
                return std::make_pair(true, VarType::TextLabel16);
            case NodeType::LVAR_INT:
                return std::make_pair(false, VarType::Int);
            case NodeType::LVAR_FLOAT:
                return std::make_pair(false, VarType::Float);
            case NodeType::LVAR_TEXT_LABEL:
                return std::make_pair(false, VarType::TextLabel);
            case NodeType::LVAR_TEXT_LABEL16:
                return std::make_pair(false, VarType::TextLabel16);
            default:
                Unreachable();
        }
    };

    auto add_label = [&](SyntaxTree& node)
    {
        auto label_name = node.text();
        auto label_ptr = this->add_label(node.shared_from_this(), current_scope, script.shared_from_this());
        if(!label_ptr)
        {
            label_ptr = this->find_label(label_name).value();
            program.error(node, "label name exists already");
            program.note(label_ptr->where, "previously defined here");
        }

        assert(label_ptr != nullptr);
        node.set_annotation(std::move(label_ptr));
    };

    walker = [&](SyntaxTree& node)
    {
        switch(node.type())
        {
            case NodeType::Label:
            {
                auto label_name = node.text();

                if(!current_scope && !program.opt.scope_then_label)
                {
                    auto parent = node.parent();
                    auto next = std::next(std::find(parent->begin(), parent->end(), node.shared_from_this()));
                    assert(std::prev(next)->get() == std::addressof(node));

                    if(next != parent->end() && (*next)->type() == NodeType::Scope)
                    {
                        next_scoped_label = node.shared_from_this();
                        return false;
                    }
                }

                add_label(node);
                return false;
            }

            case NodeType::Scope:
            {
                auto guard = make_scope_guard([&, old_current_scope = current_scope] {
                    current_scope = old_current_scope;
                });

                if(!current_scope)
                {
                    local_index = (!script.is_child_of_mission()? 0 : program.opt.mission_var_begin);
                    current_scope = this->add_scope(node);
                    current_scope->vars.emplace("TIMERA", std::make_shared<Var>(false, VarType::Int, program.opt.timer_index + 0, nullopt));
                    current_scope->vars.emplace("TIMERB", std::make_shared<Var>(false, VarType::Int, program.opt.timer_index + 1, nullopt));
                    script.scopes.emplace_back(current_scope);
                }
                else
                {
                    program.error(node, "already inside a scope");
                }

                if(next_scoped_label)
                {
                    add_label(*next_scoped_label);
                    next_scoped_label = nullptr;
                }

                node.child(0).depth_first(std::ref(walker));
                node.set_annotation(std::move(current_scope));
                return false;
            }

            case NodeType::VAR_INT: case NodeType::LVAR_INT:
            case NodeType::VAR_FLOAT: case NodeType::LVAR_FLOAT:
            case NodeType::VAR_TEXT_LABEL: case NodeType::LVAR_TEXT_LABEL:
            case NodeType::VAR_TEXT_LABEL16: case NodeType::LVAR_TEXT_LABEL16:
            {
                bool global; VarType vartype;
                std::tie(global, vartype) = token_to_vartype(node.type());

                if(global && script.is_child_of_custom())
                {
                    program.error(node, "declaring global variables in custom scripts is illegal");
                    return false;
                }
                else if(!global && !current_scope)
                {
                    program.error(node, "local variable definition outside of scope");
                    return false;
                }
                else if(!program.opt.text_label_vars && (vartype == VarType::TextLabel || vartype == VarType::TextLabel16))
                {
                    program.error(node, "text label variables are not supported");
                    return false;
                }

                auto& target = global? this->global_vars : current_scope->vars;
                auto& index = global? global_index : local_index;

                size_t max_index = [&] {
                    if(global)
                        return uint32_t(65536 / 4);
                    else if(script.is_child_of_mission())
                        return program.opt.mission_var_limit.value_or(program.opt.local_var_limit);
                    else
                        return program.opt.local_var_limit;
                }();

                for(auto& varnode : node)
                {
                    if(auto opt_token = Miss2Identifier::match(varnode->text(), program.opt))
                    {
                        auto name = opt_token->identifier;

                        auto count = [&]() -> optional<uint32_t> {
                            if(opt_token->index != nullopt)
                            {
                                if(is<size_t>(*opt_token->index))
                                {
                                    auto count = get<size_t>(*opt_token->index);
                                    if(count == 0)
                                    {
                                        program.error(*varnode, "declaring a zero-sized array");
                                        return 1;
                                    }
                                    else if(program.opt.array_elem_limit && count > *program.opt.array_elem_limit)
                                    {
                                        auto elem_limit = *program.opt.array_elem_limit;
                                        program.error(*varnode, "arrays are limited to a maximum of {} elements", elem_limit);
                                        return elem_limit;
                                    }
                                    return count;
                                }
                                else
                                {
                                    // TODO allow enum?
                                    program.error(*varnode, "index must be constant");
                                    return 1;
                                }
                            }
                            return nullopt;
                        }();

                        if(!program.opt.farrays && count)
                        {
                            program.error(*varnode, "arrays are not supported [-farrays]");
                        }

                        if(iequal_to()(name, "TIMERA") || iequal_to()(name, "TIMERB"))
                        {
                            program.error(*varnode, "variable name exists already");
                            continue;
                        }

                        auto pair = target.emplace(name, std::make_shared<Var>(varnode, global, vartype, index, count));
                        auto var = pair.first->second;

                        if(!pair.second)
                        {
                            program.error(*varnode, "variable name exists already");
                            program.note(var->where, "previously defined here");
                            varnode->set_annotation(std::move(var));
                        }
                        else
                        {
                            index += var->space_taken();
                            varnode->set_annotation(std::move(var));
                        }

                        if(index > max_index)
                            program.error(*varnode, "reached maximum {} variable limit ({})", (global? "global" : "local"), max_index);
                    }
                    else
                    {
                        program.error(*varnode, to_string(opt_token.error()));
                        continue;
                    }
                }
                return false;
            }

            case NodeType::CONST_INT:
            case NodeType::CONST_FLOAT:
            {
                if(!program.opt.fconst)
                    program.error(node, "user constants are not supported [-fconst]");

                auto& node_ident = node.child(1);
                auto& node_value = node.child(2);

                auto value = [&]() -> decltype(UserConstant::value) {
                    if(node.type() == NodeType::CONST_INT)
                        return to_integer(node_value, program).value_or(0);
                    else if(node.type() == NodeType::CONST_FLOAT)
                        return to_float(node_value, program).value_or(.0f);
                    else
                        Unreachable();
                }();

                if(!this->add_constant(node.shared_from_this(), node_ident.text().to_string(), value))
                {
                    auto& uconst = this->find_constant(node_ident.text()).value();
                    program.error(node, "user constant exists already");
                    program.note(uconst.where, "previously defined here");
                }

                return false;
            }

            default:
                return true;
        }
    };

    script.tree->depth_first(std::ref(walker));
}

void Script::annotate_tree(const SymTable& symbols, ProgramContext& program)
{
    const Commands& commands = program.commands;

    std::function<bool(SyntaxTree&)> walker;

    bool had_mission_start = false;
    bool had_mission_end   = false;
    bool had_script_start  = false;
    bool had_script_end    = false;
    
    shared_ptr<Scope> current_scope;
    shared_ptr<Label> cutscene_skip;
    bool is_condition_block = false;
    uint32_t num_statements = 0;
    uint32_t num_directives = 0;
    uint32_t loop_depth_continue = 0;
    uint32_t loop_depth_break = 0;

    auto directive_info = [&](NodeType type)
    {
        switch(type)
        {
            case NodeType::MISSION_START:
            case NodeType::MISSION_END:
                return std::make_tuple(&had_mission_start, &had_mission_end, "MISSION_START", "MISSION_END");
            case NodeType::SCRIPT_START:
            case NodeType::SCRIPT_END:
                return std::make_tuple(&had_script_start, &had_script_end, "SCRIPT_START", "SCRIPT_END");
            default:
                Unreachable();
        }
    };

    auto alternator_for_expr = [&](const SyntaxTree& op) -> optional<const Commands::Alternator&>
    {
        switch(op.type())
        {
            case NodeType::Equal:
                if(is_condition_block)
                    return commands.is_thing_equal_to_thing;
                else
                    return commands.set;
            case NodeType::Cast:
                return commands.cset;
            case NodeType::Add:
                return commands.add_thing_to_thing;
            case NodeType::Sub:
                return commands.sub_thing_from_thing;
            case NodeType::Times:
                return commands.mult_thing_by_thing;
            case NodeType::Divide:
                return commands.div_thing_by_thing;
            case NodeType::TimedAdd:
                return commands.add_thing_to_thing_timed;
            case NodeType::TimedSub:
                return commands.sub_thing_from_thing_timed;
            case NodeType::Greater:
            case NodeType::Lesser: // with arguments inverted
                return commands.is_thing_greater_than_thing;
            case NodeType::GreaterEqual:
            case NodeType::LesserEqual: // with arguments inverted
                return commands.is_thing_greater_or_equal_to_thing;
            case NodeType::Increment:
                return commands.add_thing_to_thing;
            case NodeType::Decrement:
                return commands.sub_thing_from_thing;
            default:
                return nullopt;
        }
    };

    auto traverse_condition_list = [&](SyntaxTree& node)
    {
        auto guard = make_scope_guard([&, was_cond_before = is_condition_block] {
            is_condition_block = was_cond_before;
        });

        is_condition_block = true;

        if(node.type() == NodeType::AND || node.type() == NodeType::OR)
        {
            if(node.child_count() > 8)
                program.error(node, "use of more than 8 conditions is not supported");

            node.depth_first(std::ref(walker));
        }
        else
        {
            walker(node);
        }
    };

    auto traverse_loop_body = [&](SyntaxTree& node, bool allow_break, bool allow_continue)
    {
        auto guard = make_scope_guard([&, prev_break = loop_depth_break, prev_cont = loop_depth_continue] {
            loop_depth_break = prev_break;
            loop_depth_continue = prev_cont;
        });

        loop_depth_break += allow_break? 1 : 0;
        loop_depth_continue += allow_continue? 1 : 0;
        node.depth_first(std::ref(walker));
    };

    walker = [&](SyntaxTree& node)
    {
        ++num_statements;
        switch(node.type())
        {
            case NodeType::Label:
                // already annotated in SymTable::scan_symbols
                return false;

            case NodeType::VAR_INT: case NodeType::LVAR_INT:
            case NodeType::VAR_FLOAT: case NodeType::LVAR_FLOAT:
            case NodeType::VAR_TEXT_LABEL: case NodeType::LVAR_TEXT_LABEL:
            case NodeType::VAR_TEXT_LABEL16: case NodeType::LVAR_TEXT_LABEL16:
                // already annotated in SymTable::scan_symbols
                return false;

            case NodeType::Scope:
            {
                // already annotated in SymTable::scan_symbols, but let's inform about current scope.

                assert(current_scope == nullptr);

                auto guard = make_scope_guard([&] {
                    current_scope = nullptr;
                });

                current_scope = node.annotation<shared_ptr<Scope>>();
                node.child(0).depth_first(std::ref(walker));
                return false;
            }

            case NodeType::IFNOT:
                if(!program.opt.fifnot)
                    program.error(node, "IFNOT not supported [-fifnot]");
                [[fallthrough]];
            case NodeType::IF:
            {
                traverse_condition_list(node.child(0));
                node.child(1).depth_first(std::ref(walker));
                if(node.child_count() == 3)
                {
                    assert(node.child(2).type() == NodeType::ELSE);
                    node.child(2).depth_first(std::ref(walker));
                }
                return false;
            }

            case NodeType::WHILENOT:
                if(!program.opt.fifnot)
                    program.error(node, "WHILENOT not supported [-fifnot]");
                [[fallthrough]];
            case NodeType::WHILE:
            {
                traverse_condition_list(node.child(0));
                traverse_loop_body(node.child(1), true, true);
                return false;
            }

            case NodeType::MISSION_START:
            case NodeType::SCRIPT_START:
            {
                bool *had_start, *had_end; const char *dir_start, *dir_end;
                std::tie(had_start, had_end, dir_start, dir_end) = directive_info(node.type());

                if(*had_start == false)
                {
                    *had_start = true;
                    if(num_statements != 1)
                        program.error(node, "{} must be the first statement in script", dir_start);
                }
                else
                {
                    program.error(node, "more than one {} in script", dir_start);
                }
                return false;
            }

            case NodeType::MISSION_END:
            case NodeType::SCRIPT_END:
            {
                bool *had_start, *had_end; const char *dir_start, *dir_end;
                std::tie(had_start, had_end, dir_start, dir_end) = directive_info(node.type());

                if(*had_end == false)
                {
                    *had_end = true;

                    if(*had_start == false)
                        program.error(node, "{} without a {}", dir_end, dir_start);

                    if(this->is_child_of(ScriptType::CustomScript))
                    {
                        const Command& command = program.supported_or_fatal(node, commands.terminate_this_custom_script, "TERMINATE_THIS_CUSTOM_SCRIPT");
                        node.set_annotation(std::cref(command));
                    }
                    else
                    {
                        const Command& command = program.supported_or_fatal(node, commands.terminate_this_script, "TERMINATE_THIS_SCRIPT");
                        node.set_annotation(std::cref(command));
                    }
                }
                else
                {
                    program.error(node, "more than one {} in script", dir_end);
                }
                return false;
            }

            case NodeType::NOT:
            {
                if(!is_condition_block && !program.opt.relax_not)
                    program.error(node, "NOT outside of a conditional statement [-frelax-not]");

                node.child(0).depth_first(std::ref(walker));
                return false;
            }

            case NodeType::Command:
            {
                auto command_name = node.child(0).text();
                auto use_filenames = (this->type == ScriptType::Main || this->type == ScriptType::MainExtension);

                if(use_filenames && iequal_to()(command_name, "LOAD_AND_LAUNCH_MISSION"))
                {
                    const Command& command = program.supported_or_fatal(node, commands.load_and_launch_mission_internal,
                                                                        "LOAD_AND_LAUNCH_MISSION_INTERNAL");
                    shared_ptr<Script> script = symbols.find_script(node.child(1).text()).value();
                    node.set_annotation(ReplacedCommandAnnotation { std::cref(command), { int32_t(script->mission_id.value()) } });
                }
                else if(use_filenames && iequal_to()(command_name, "LAUNCH_MISSION"))
                {
                    const Command& command = program.supported_or_fatal(node, commands.launch_mission,
                                                                        "LAUNCH_MISSION");
                    shared_ptr<Script> script = symbols.find_script(node.child(1).text()).value();
                    node.child(1).set_annotation(script->start_label);
                    node.set_annotation(std::cref(command));
                }
                else if(use_filenames && iequal_to()(command_name, "GOSUB_FILE"))
                {
                    const Command& command = program.supported_or_fatal(node, commands.gosub_file,
                                                                        "GOSUB_FILE");
                    shared_ptr<Label>  label  = symbols.find_label(node.child(1).text()).value();
                    node.child(1).set_annotation(label);
                    node.child(2).set_annotation(label);
                    node.set_annotation(std::cref(command));
                }
                else if(use_filenames && iequal_to()(command_name, "REGISTER_STREAMED_SCRIPT"))
                {
                    const Command& command = program.supported_or_fatal(node, commands.register_streamed_script_internal,
                                                                        "REGISTER_STREAMED_SCRIPT_INTERNAL");
                    auto streamed_id = symbols.find_streamed_id(node.child(1).text()).value();
                    node.set_annotation(ReplacedCommandAnnotation { std::cref(command), { int32_t(streamed_id) } });
                }
                else if(iequal_to()(command_name, "REQUIRE"))
                {
                    const Command& command = program.supported_or_fatal(node, commands.require, "REQUIRE");
                    shared_ptr<Script> script = symbols.find_script(node.child(1).text()).value();
                    node.child(1).set_annotation(script->top_label);
                    node.set_annotation(DummyCommandAnnotation{});
                }
                else
                {
                    auto exp_command = commands.match(node, symbols, current_scope, program.opt);
                    if(exp_command)
                    {
                        const Command& command = **exp_command;

                        if(!command.supported)
                            program.error(node, "unsupported command");

                        if(command.internal)
                            program.error(node, "unexpected use of internal command");
                        
                        if(command.extension && program.opt.pedantic)
                            program.pedantic(node, "this command is a language extension [-pedantic]");

                        commands.annotate(node, command, symbols, current_scope, *this, program);
                        node.set_annotation(std::cref(command));

                        if(commands.equal(command, commands.skip_cutscene_start))
                        {
                            const Command& internal = program.supported_or_fatal(node, commands.skip_cutscene_start_internal,
                                                                                 "SKIP_CUTSCENE_START_INTERNAL");

                            if(!program.opt.skip_cutscene)
                                program.error(node, "{} not supported [-fskip-cutscene]", command_name);

                            if(cutscene_skip)
                            {
                                program.error(node, "{} inside another {}", command_name, command_name);
                            }
                            else
                            {
                                cutscene_skip = std::make_shared<Label>(current_scope, this->shared_from_this());
                                node.set_annotation(ReplacedCommandAnnotation { internal, {cutscene_skip} } );
                            }
                        }
                        else if(commands.equal(command, commands.skip_cutscene_end))
                        {
                            if(!cutscene_skip)
                                program.error(node, "{} without SKIP_CUTSCENE_START", command_name);
                            else
                                cutscene_skip = nullptr;
                        }
                        else if(commands.equal(command, commands.cleo_return))
                        {
                            if(!current_scope)
                                program.error(node, "CLEO_RETURN must be inside a scope");
                        }
                    }
                    else
                    {
                        exp_command.error().emit(program);
                    }
                }

                return false;
            }

            case NodeType::REPEAT:
            {
                const Command& repeat = program.supported_or_fatal(node, commands.repeat, "REPEAT");

                auto& times = node.child(0);
                auto& var = node.child(1);

                auto exp_command = commands.match(repeat, node, { &times, &var }, symbols, current_scope, program.opt);
                if(!exp_command)
                {
                    exp_command.error().emit(program);
                    return false;
                }

                assert(*exp_command == &repeat);
                commands.annotate({ &times, &var }, **exp_command, symbols, current_scope, *this, program);

                traverse_loop_body(node.child(2), true, true);

                auto number_zero = Commands::MatchArgument(0);
                auto number_one = Commands::MatchArgument(1);

                auto& alt_set = program.supported_or_fatal(node, commands.set, "SET");
                auto& alt_add_thing_to_thing = program.supported_or_fatal(node, commands.add_thing_to_thing, "ADD_THING_TO_THING");
                auto& alt_is_thing_greater_or_equal_to_thing = program.supported_or_fatal(node, commands.is_thing_greater_or_equal_to_thing,
                                                                                          "IS_THING_GREATER_OR_EQUAL_TO_THING");

                auto exp_set_var_to_zero = commands.match(alt_set, node, { &var, number_zero }, symbols, current_scope, program.opt);
                auto exp_add_var_with_one = commands.match(alt_add_thing_to_thing, node, { &var, number_one }, symbols, current_scope, program.opt);
                auto exp_is_var_geq_times = commands.match(alt_is_thing_greater_or_equal_to_thing, node, { &var, &times }, symbols, current_scope, program.opt);

                if(exp_set_var_to_zero && exp_add_var_with_one && exp_is_var_geq_times)
                {
                    commands.annotate({ &var, nullopt }, **exp_set_var_to_zero, symbols, current_scope, *this, program);
                    commands.annotate({ &var, nullopt }, **exp_add_var_with_one, symbols, current_scope, *this, program);
                    commands.annotate({ &var, &times }, **exp_is_var_geq_times, symbols, current_scope, *this, program);

                    node.set_annotation(RepeatAnnotation {
                        **exp_set_var_to_zero, **exp_add_var_with_one, **exp_is_var_geq_times,
                        std::move(number_zero), std::move(number_one)
                    });

                    if(program.opt.pedantic)
                    {
                        if(auto varinfo = get_base_var_annotation(var))
                        {
                            if((*varinfo)->global == false)
                                program.pedantic(var, "REPEAT only allows global variables [-pedantic]");
                        }
                    }
                }
                else
                {
                    if(!exp_set_var_to_zero)  exp_set_var_to_zero.error().emit(program);
                    if(!exp_add_var_with_one) exp_add_var_with_one.error().emit(program);
                    if(!exp_is_var_geq_times) exp_is_var_geq_times.error().emit(program);
                }

                return false;
            }

            case NodeType::SWITCH:
            {
                if(!program.opt.fswitch)
                    program.error(node, "SWITCH not supported [-fswitch]");

                std::vector<int32_t> case_values;
                bool had_default = false;
                bool last_statement_was_break = true;

                shared_ptr<const SyntaxTree> last_case;
                auto& var = node.child(0);

                const Command& switch_command = program.supported_or_fatal(node, commands.switch_, "SWITCH");
                const Command& case_command   = program.supported_or_fatal(node, commands.case_, "CASE");

                auto exp_switch = commands.match(switch_command, node, { &var }, symbols, current_scope, program.opt);
                if(exp_switch)
                    commands.annotate({ &var }, **exp_switch, symbols, current_scope, *this, program);
                else
                    exp_switch.error().emit(program);

                auto guard = [&, prev_break = loop_depth_break] {
                    loop_depth_break = prev_break;
                };

                ++loop_depth_break;

                for(auto& body_node : node.child(1))
                {
                    switch(body_node->type())
                    {
                        case NodeType::CASE:
                        {
                            if(last_case && !last_statement_was_break)
                                program.error(*last_case, "CASE does not end with a BREAK");

                            auto& case_node = body_node;
                            last_case       = body_node;

                            auto& case_value = case_node->child(0);

                            auto exp_case = commands.match(case_command, *case_node, { &case_value }, symbols, current_scope, program.opt);
                            if(exp_case)
                            {
                                commands.annotate({ &case_value }, **exp_case, symbols, current_scope, *this, program);
                                case_node->set_annotation(SwitchCaseAnnotation{ nullptr });
                            }
                            else
                            {
                                exp_case.error().emit(program);
                            }

                            if(!commands.switch_start)
                            {
                                auto& alt_is_thing_equal_to_thing = program.supported_or_fatal(node, commands.is_thing_equal_to_thing,
                                                                                                "IS_THING_EQUAL_TO_THING");
                                auto exp_is_var_eq_int  = commands.match(alt_is_thing_equal_to_thing, *case_node, { &var, &case_value },
                                                                         symbols, current_scope, program.opt);
                                if(exp_is_var_eq_int)
                                {
                                    commands.annotate({ &var, &case_value }, **exp_is_var_eq_int, symbols, current_scope, *this, program);
                                    case_node->set_annotation(SwitchCaseAnnotation{ *exp_is_var_eq_int });
                                }
                                else if(exp_case) // if CASE matching didn't fail but alternator did
                                {
                                    exp_is_var_eq_int.error().emit(program);
                                }
                            }

                            if(case_value.is_annotated())
                            {
                                if(auto v = case_value.maybe_annotation<const int32_t&>())
                                {
                                    if(std::find(case_values.begin(), case_values.end(), *v) != case_values.end())
                                        program.error(*case_node, "duplicate CASE value {}", *v);
                                    else
                                        case_values.emplace_back(*v);
                                }
                            }

                            break;
                        }

                        case NodeType::DEFAULT:
                        {
                            if(had_default)
                                program.error(*body_node, "multiple DEFAULT labels in one SWITCH");

                            if(last_case && !last_statement_was_break)
                                program.error(*last_case, "CASE does not end with a BREAK");

                            last_case = body_node;
                            had_default = true;
                            break;
                        }

                        case NodeType::BREAK:
                            if(last_case)
                                last_statement_was_break = true;
                            else
                                program.error(*body_node, "BREAK not within a CASE or DEFAULT label");
                            break;

                        default: // statement
                            if(last_case)
                            {
                                last_statement_was_break = false;
                                body_node->depth_first(std::ref(walker));
                            }
                            else
                            {
                                program.error(*body_node, "statement not within a CASE or DEFAULT label");
                            }
                            break;
                    }
                }

                if(last_case && !last_statement_was_break)
                    program.error(*last_case, "CASE does not end with a BREAK");

                if(auto switch_case_limit = program.opt.switch_case_limit)
                {
                    if(case_values.size() > static_cast<size_t>(*switch_case_limit))
                        program.error(node, "SWITCH contains more than {} cases [-fswitch-case-limit]", *switch_case_limit);
                }

                node.set_annotation(SwitchAnnotation { case_values.size(), had_default });
                return false;
            }

            case NodeType::Equal:
            case NodeType::Cast:
            case NodeType::Greater:
            case NodeType::GreaterEqual:
            case NodeType::Lesser:
            case NodeType::LesserEqual:
            {
                const Commands::Alternator& alter_cmds1 = program.supported_or_fatal(node, alternator_for_expr(node), "<unknown>");

                if(auto alter_op = alternator_for_expr(node.child(1)))
                {
                    // either 'a = b OP c' or 'a OP= c'

                    auto& op = node.child(1);
                    auto& a = node.child(0);
                    auto& b = op.child(0);
                    auto& c = op.child(1);

                    auto exp_cmd_set = commands.match(alter_cmds1, node, { &a, &b }, symbols, current_scope, program.opt);
                    auto exp_cmd_op  = commands.match(*alter_op, node, { &a, &c }, symbols, current_scope, program.opt);

                    if(is_condition_block)
                        program.error(node, "expression not allowed in this context");

                    if(exp_cmd_set && exp_cmd_op)
                    {
                        commands.annotate({ &a, &b }, **exp_cmd_set, symbols, current_scope, *this, program);
                        commands.annotate({ &a, &c }, **exp_cmd_op, symbols, current_scope, *this, program);

                        const char* message = nullptr;
                        switch(op.type())
                        {
                            case NodeType::Sub:
                                message = "cannot do VAR1 = THING - VAR1";
                                break;
                            case NodeType::Divide:
                                message = "cannot do VAR1 = THING / VAR1";
                                break;
                            case NodeType::TimedAdd:
                                message = "cannot do VAR1 = THING +@ VAR1";
                                break;
                            case NodeType::TimedSub:
                                message = "cannot do VAR1 = THING -@ VAR1";
                                break;
                        }

                        auto a_var = a.maybe_annotation<shared_ptr<Var>>();
                        auto b_var = b.maybe_annotation<shared_ptr<Var>>();
                        auto c_var = c.maybe_annotation<shared_ptr<Var>>();

                        if(a_var && c_var && a_var == c_var) // Y = THING op Y
                        {
                            if(message)
                            {
                                if(a_var != b_var || b_var != c_var) // allow Y = Y / Y
                                    program.error(node, message);
                            }

                            auto exp_cmd_set2 = commands.match(alter_cmds1, node, { &a, &c }, symbols, current_scope, program.opt);
                            auto exp_cmd_op2  = commands.match(*alter_op, node, { &a, &b }, symbols, current_scope, program.opt);

                            if(exp_cmd_set2 && exp_cmd_op2)
                            {
                                commands.annotate({ &a, &c }, **exp_cmd_set2, symbols, current_scope, *this, program);
                                commands.annotate({ &a, &b }, **exp_cmd_op2, symbols, current_scope, *this, program);

                                node.set_annotation(std::cref(**exp_cmd_set2));
                                op.set_annotation(std::cref(**exp_cmd_op2));
                            }
                            else
                            {
                                if(!exp_cmd_set2) exp_cmd_set2.error().emit(program);
                                if(!exp_cmd_op2) exp_cmd_op2.error().emit(program);
                            }
                        }
                        else
                        {
                            node.set_annotation(std::cref(**exp_cmd_set));
                            op.set_annotation(std::cref(**exp_cmd_op));
                        }
                    }
                    else
                    {
                        if(!exp_cmd_set) exp_cmd_set.error().emit(program);
                        if(!exp_cmd_op) exp_cmd_op.error().emit(program);
                    }
                }
                else
                {
                    // 'a = b' or 'a =# b' or 'a > b' (and such)

                    bool invert = (node.type() == NodeType::Lesser || node.type() == NodeType::LesserEqual);

                    auto& a = node.child(!invert? 0 : 1);
                    auto& b = node.child(!invert? 1 : 0);

                    if(is_condition_block && node.type() == NodeType::Cast)
                        program.error(node, "expression not allowed in this context");

                    auto exp_command = commands.match(alter_cmds1, node, { &a, &b }, symbols, current_scope, program.opt);
                    if(exp_command)
                    {
                        commands.annotate({ &a, &b }, **exp_command, symbols, current_scope, *this, program);
                        node.set_annotation(std::cref(**exp_command));
                    }
                    else
                    {
                        exp_command.error().emit(program);
                    }
                }
                return false;
            }

            case NodeType::Increment:
            case NodeType::Decrement:
            {
                auto opkind = (node.type() == NodeType::Increment? "increment" : "decrement");
                auto& alternator_thing = program.supported_or_fatal(node, alternator_for_expr(node).value(), "<unknown>");

                auto& var_ident = node.child(0);
                
                if(is_condition_block)
                    program.error(node, "expression not allowed in this context");

                auto exp_op_var_with_one = commands.match(alternator_thing, node, { &var_ident, 1 }, symbols, current_scope, program.opt);
                if(exp_op_var_with_one)
                {
                    commands.annotate({ &var_ident, nullopt }, **exp_op_var_with_one, symbols, current_scope, *this, program);
                    node.set_annotation(IncDecAnnotation { **exp_op_var_with_one, 1 });
                }
                else
                {
                    exp_op_var_with_one.error().emit(program);
                }

                return false;
            }

            case NodeType::BREAK:
            {
                if(!program.opt.allow_break_continue)
                    program.error(node, "BREAK only allowed at the end of a SWITCH CASE [-fbreak-continue]");
                else if(!loop_depth_break)
                    program.error(node, "BREAK not in a loop or SWITCH statement");
                else
                    program.pedantic(node, "BREAK anywhere is a language extension [-pedantic]");
                return false;
            }

            case NodeType::CONTINUE:
            {
                program.pedantic(node, "CONTINUE is a language extension [-pedantic]");

                if(!program.opt.allow_break_continue)
                    program.error(node, "CONTINUE is not supported [-fbreak-continue]");
                else if(!loop_depth_continue)
                    program.error(node, "CONTINUE not in a loop");
                return false;
            }

            case NodeType::DUMP:
            {
                program.pedantic(node, "DUMP is a language extension [-pedantic]");
                return false;
            }

            default:
                // go into my childs to find a proper statement node
                assert(num_statements > 0);
                --num_statements;
                return true;
        }
    };

    this->tree->depth_first(std::ref(walker));

    if(this->type == ScriptType::Mission || this->type == ScriptType::CustomMission || this->type == ScriptType::Subscript)
    {
        if(!had_mission_start)
            program.error(*this, "{} script does not contain MISSION_START", to_string(this->type));
        else if(!had_mission_end)
            program.error(*this, "{} script does not contain MISSION_END", to_string(this->type));
    }
    else if(this->type == ScriptType::CustomScript || this->type == ScriptType::StreamedScript)
    {
        if(!had_script_start)
            program.error(*this, "{} script does not contain SCRIPT_START", to_string(this->type));
        else if(!had_script_end)
            program.error(*this, "{} script does not contain SCRIPT_END", to_string(this->type));
    }
    else if(had_mission_start || had_script_start)
    {
        program.error(*this, "cannot use {} in {} scripts",
                      had_mission_start? "MISION_START" : "SCRIPT_START",
                      to_string(this->type));
    }

    if(cutscene_skip)
    {
        program.error(*this, "missing SKIP_CUTSCENE_END");
    }
}
