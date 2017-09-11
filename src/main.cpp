#include <stdinc.h>
#include "program.hpp"
#include "system.hpp"
#include "cpp/argv.hpp"

const char* GTA3SC_HELP_MESSAGE =
R"(Usage: gta3sc [compile|decompile] --config=<name> file [options]
Options:
  --help                   Display this information.
  --version                Displays version information.
  -o <file>                Place the output into <file>.
  --cs                     Outputs a CLEO script. This also sets -fcleo.
  --cm                     Outputs a CLEO custom mission.
                           This also sets -fcleo and -fmission-script.
  --config=<name>          Which compilation configurations to use (gta3,gtavc,
                           gtasa). This effectively reads the data files at
                           '/config/<name>/' and sets some appropriate flags.
  --datadir=<path>         Path to where IDE and DAT files are in.
                           The compiler will still try to behave properly
                           without this, but this is still recommended.
  --levelfile=<name>       Name of the level data file in the data directory.
  --add-config=<path>      Adds an additional XML definition file.
                           If the path is not absolute or starts with './' or
                           '../', uses a path relative to 'config/<name>/'.
  -pedantic                Warns when using extensions not in R* language.
  -pedantic-errors         Errors when using extensions not in R* language.
  --guesser                Allows the use of language features not completly
                           known or understood by the modding community.
  -D <name>                Defines the preprocessor directive <name>.
  --define=<name>          Ditto.
  -U <name>                Undefines the preprocessor directive <name>.
  --undefine=<name>        Ditto.
  -O                       Enables optimizations.
  -emit-ir2                Emits a explicit IR based on Sanny Builder syntax.
  -fsyntax-only            Only checks the syntax, i.e. doesn't generate code.
  --recursive-traversal    Disassembler scans the code by the means of a
                           recursive traversal instead of linear-sweep.
  --expect-var=<info>

Language Options:
  -fswitch                 Enables the SWITCH statement.
  -farrays                 Enables the use of arrays.
  -fconst                  Enables the use of CONST_INT and CONST_FLOAT.
  -ftext-label-vars        Enables VAR_TEXT_LABEL and VAR_TEXT_LABEL16.
  -fskip-cutscene          Enables the use of SKIP_CUTSCENE_START.
  -fscript-name-check      Checks for duplicate SCRIPT_NAMEs.
  -fentity-tracking        Tracks entity types in variables.
  -fbreak-continue         Allows the use of BREAK and CONTINUE in all
                           statements, including WHILE and REPEAT.
  -fstreamed-scripts       Enables the use of streamed scripts and generates an
                           associated script.img archive.
  -fscope-then-label       When combined with -pedantic, outputs a error message
                           whenever a label is used before a curly bracket
                           instead of after.
  -funderscore-idents      Allows identifiers to begin with a underscore.
  -flocal-var-limit=<n>    The index limit of local variables.
  -fmission-var-limit=<n>  The index limit of mission local variables. Defaults
                           to -flocal-var-limit if not set. Use -1 to unset.
  -fmission-var-begin=<n>  Mission variables will start from the index <n>.
  -ftimer-index=<n>        The local variable index of TIMERA.
  -fswitch-case-limit=<n>  The limit on the number of CASE in a SWITCH.
  -farray-elem-limit=<n>   The limit of array elements in a single array.
  -frelax-not              Allows the use of NOT outside of conditions.
  -fcleo                   Enables the use of CLEO features.
  -fmission-script         Compiling a mission script.

Machine Options:
  -mno-header              Does not generate a header on the output SCM.
  -mheader=<version>       Generates the specified header version (gta3,gtavc,
                           gtasa).
  -mlocal-offsets          Label offsets are referenced locally and relative
                           to the offset 0 of the compiled script.
  -mq11.4                  Codegen uses GTA III half-float format.
  -mtyped-text-label       Codegen uses GTA SA text label data type.
  -moptimize-andor         Omits compiling ANDOR on single condition statements.
  -moptimize-zero          Compiles 0.0 as 0, using a 8 bit data type.
  -moatc                   Uses the Custom Commands Header whenever possible.

Error Message Options:
  --error-format=<format>  The error formating for the compiler errors.
                           May be `default` or `json`. Do note the JSON format
                           may contain some pre-compilation messages in the
                           default format (i.e. gta3sc: type:? message).
  -Werror                  Turns warnings into errors.
  -Wconflict-text-label-var Warns when text labels conflicts with variable
                            names.
  -Wexpect-var             Warns if any of the variables specified with
                           the --expect-var option is out of place.
  -fconstant-checks        Checks whether variables collides with constants.
)";

#ifdef GTA3SC_USING_GIT_DESCRIBE
extern const char* GTA3SC_GIT_SHA1;
extern const char* GTA3SC_GIT_BRANCH;
extern const char* GTA3SC_GIT_DESCRIBE_TAG;
#else
const char* GTA3SC_GIT_SHA1 = "";
const char* GTA3SC_GIT_BRANCH = "";
const char* GTA3SC_GIT_DESCRIBE_TAG = "";
#endif

enum class Action
{
    None,
    Compile,
    Decompile,
    QueryConfigPath,
    QueryModels,
};


struct DataInfo
{
    fs::path    datadir;
    std::string levelfile;
};

struct ConfigInfo
{
    std::string           config_name;
    std::vector<fs::path> add_config_files;
};

bool parse_args(char**& argv, fs::path& input, fs::path& output, DataInfo& data, ConfigInfo& conf, Options& options)
{
    try
    {
        bool flag;
        int32_t temp_i32;

        while(*argv)
        {
            if(**argv != '-')
            {
                if(!input.empty())
                {
                    fprintf(stderr, "gta3sc: error: input file appears twice\n");
                    return false;
                }

                input = *argv;
                ++argv;
            }
            else if(optget(argv, "-h", "--help", 0))
            {
                options.help = true;
                return true;
            }
            else if(optget(argv, nullptr, "--version", 0))
            {
                options.version = true;
                return true;
            }
            else if(const char* o = optget(argv, "-o", nullptr, 1))
            {
                output = o;
            }
            else if(optget(argv, nullptr, "-pedantic-errors", 0))
            {
                options.pedantic = true;
                options.pedantic_errors = true;
            }
            else if(optget(argv, nullptr, "-pedantic", 0))
            {
                options.pedantic = true;
            }
            else if(optget(argv, nullptr, "--guesser", 0))
            {
                options.guesser = true;
            }
            else if(const char* info = optget(argv, nullptr, "--expect-var", 1))
            {
                if(!options.push_expect_var(info))
                {
                    fprintf(stderr, "gta3sc: error: failed to parse --expect-var entry\n");
                    return false;
                }
            }
            else if(optget(argv, nullptr, "--recursive-traversal", 0))
            {
                options.linear_sweep = false;
            }
            else if(const char* name = optget(argv, nullptr, "--config", 1))
            {
                // avoid infinite recursion of parse_args(...) calls
                if(iequal_to()(conf.config_name, name))
                    continue;

                conf.config_name = name;

                if(auto opt_cmdline = read_file_utf8(config_path() / conf.config_name / "commandline.txt"))
                {
                    auto& cmdline = *opt_cmdline;
                    small_vector<char*, 128> args;

                    auto it = !cmdline.empty()? &cmdline[0] : nullptr;
                    auto end = it + cmdline.size();
                    for(; it != end; )
                    {
                        it = std::find_if_not(it, end, ::isspace);
                        args.emplace_back(it);
                        it = std::find_if(it, end, ::isspace);
                        if(it != end) *it++ = '\0';
                    }
                    args.emplace_back(nullptr);

                    char** argv2 = args.data();
                    if(!parse_args(argv2, input, output, data, conf, options))
                        return false;
                }
                else
                {
                    fprintf(stderr, "gta3sc: error: config path is missing commandline.txt file\n");
                    return false;
                }
            }
            else if(const char* path = optget(argv, nullptr, "--add-config", 1))
            {
                conf.add_config_files.emplace_back(path);
            }
            else if(const char* path = optget(argv, nullptr, "--datadir", 1))
            {
                data.datadir = path;
            }
            else if(const char* name = optget(argv, nullptr, "--levelfile", 1))
            {
                data.levelfile = name;
            }
            else if(const char* name = optget(argv, nullptr, "--error-format", 1))
            {
                if(!strcmp(name, "default"))
                    options.error_format = Options::ErrorFormat::Default;
                else if(!strcmp(name, "json"))
                    options.error_format = Options::ErrorFormat::JSON;
                else
                {
                    fprintf(stderr, "gta3sc: error: invalid error-format\n");
                    return false;
                }
            }
            else if(const char* ver = optget(argv, nullptr, "-mheader", 1))
            {
                if(!strcmp(ver, "gta3"))
                    options.header = Options::HeaderVersion::GTA3;
                else if(!strcmp(ver, "gtavc"))
                    options.header = Options::HeaderVersion::GTAVC;
                else if(!strcmp(ver, "gtasa"))
                    options.header = Options::HeaderVersion::GTASA;
                else
                {
                    fprintf(stderr, "gta3sc: error: invalid header version, must be 'gta3', 'gtavc' or 'gtasa'\n");
                    return false;
                }
            }
            else if(optflag(argv, "-mno-header", nullptr))
            {
                options.headerless = true;
            }
            else if(optflag(argv, "-moatc", &flag))
            {
                options.oatc = flag;
            }
            else if(optflag(argv, "-mq11.4", &flag))
            {
                options.use_half_float = flag;
            }
            else if(optflag(argv, "-mtyped-text-label", &flag))
            {
                options.has_text_label_prefix = flag;
            }
            else if(optflag(argv, "-moptimize-andor", &flag))
            {
                options.optimize_andor = flag;
            }
            else if(optflag(argv, "-moptimize-zero", &flag))
            {
                options.optimize_zero_floats = flag;
            }
            else if(optget(argv, nullptr, "-O", 0))
            {
                options.optimize_andor = true;
                options.optimize_zero_floats = true;
            }
            else if(optflag(argv, "-fentity-tracking", &flag))
            {
                options.entity_tracking = flag;
            }
            else if(optflag(argv, "-fscript-name-check", &flag))
            {
                options.script_name_check = flag;
            }
            else if(optflag(argv, "-frelax-not", &flag))
            {
                options.relax_not = flag;
            }
            else if(optflag(argv, "-fswitch", &flag))
            {
                options.fswitch = flag;
            }
            else if(optflag(argv, "-fbreak-continue", nullptr))
            {
                options.allow_break_continue = true;
            }
            else if(optflag(argv, "-fscope-then-label", &flag))
            {
                options.scope_then_label = flag;
            }
            else if(optflag(argv, "-funderscore-idents", &flag))
            {
                options.allow_underscore_identifiers = flag;
            }
            else if(optflag(argv, "-farrays", &flag))
            {
                options.farrays = flag;
            }
            else if(optflag(argv, "-fconst", &flag))
            {
                options.fconst = flag;
            }
            else if(optflag(argv, "-fstreamed-scripts", &flag))
            {
                options.streamed_scripts = flag;
            }
            else if(optflag(argv, "-ftext-label-vars", &flag))
            {
                options.text_label_vars = flag;
            }
            else if(optflag(argv, "-fskip-cutscene", &flag))
            {
                options.skip_cutscene = flag;
            }
            else if(optflag(argv, "-mlocal-offsets", nullptr))
            {
                options.use_local_offsets = true;
            }
            else if(optint(argv, "-ftimer-index", &options.timer_index)) {}
            else if(optint(argv, "-flocal-var-limit", &options.local_var_limit)) {}
            else if(optint(argv, "-fmission-var-limit", &temp_i32))
            {
                options.mission_var_limit = temp_i32 < 0? nullopt : optional<uint32_t>(temp_i32);
            }
            else if(optint(argv, "-fmission-var-begin", &temp_i32))
            {
                options.mission_var_begin = std::max(0, temp_i32);
            }
            else if(optint(argv, "-fswitch-case-limit", &temp_i32))
            {
                options.switch_case_limit = temp_i32 < 0? nullopt : optional<uint32_t>(temp_i32);
            }
            else if(optint(argv, "-farray-elem-limit", &temp_i32))
            {
                options.array_elem_limit = temp_i32 < 0? nullopt : optional<uint32_t>(temp_i32);
            }
            else if(optflag(argv, "-fsyntax-only", nullptr))
            {
                options.fsyntax_only = true;
            }
            else if(optflag(argv, "-emit-ir2", nullptr))
            {
                options.emit_ir2 = true;
            }
            else if(optflag(argv, "-fcleo", nullptr))
            {
                options.cleo.emplace(0);
            }
            else if(optget(argv, nullptr, "--cs", 0))
            {
                options.cleo.emplace(0);
                options.output_cleo = true;
                options.mission_script = false;
                options.headerless = true;
                options.use_local_offsets = true;
            }
            else if(optget(argv, nullptr, "--cm", 0))
            {
                options.cleo.emplace(0);
                options.output_cleo = true;
                options.mission_script = true;
                options.headerless = true;
                options.use_local_offsets = true;
            }
            else if(optflag(argv, "-fmission-script", nullptr))
            {
                options.mission_script = true;
            }
            else if(optflag(argv, "-Werror", &flag))
            {
                options.warning_is_error = flag;
            }
            else if(optflag(argv, "-Wconflict-text-label-var", &flag))
            {
                options.warn_conflict_text_label_var = flag;
            }
            else if(optflag(argv, "-Wexpect-var", &flag))
            {
                options.warn_expect_var = flag;
            }
            else if(optflag(argv, "-fconstant-checks", &flag))
            {
                options.constant_checks = flag;
            }
            else if(const char* name = optget(argv, "-D", "--define", 1))
            {
                options.define(name);
            }
            else if(const char* name = optget(argv, "-U", "--undefine", 1))
            {
                options.undefine(name);
            }
            else
            {
                fprintf(stderr, "gta3sc: error: unregonized argument '%s'\n", *argv);
                return false;
            }
        }

        return true;
    }
    catch(const invalid_opt& e)
    {
        fprintf(stderr, "gta3sc: error: %s\n", e.what());
        return false;
    }
}

int main(int argc, char** argv)
{
    // Due to main() not having a ProgramContext yet, error reporting must be done using fprintf(stderr, ...).

    Action action = Action::None;
    Options options;
    fs::path input, output;
    ConfigInfo conf;
    DataInfo data;

    optional<ProgramContext> program; // delay construction of ProgramContext
    std::map<std::string, uint32_t, iless> default_models;
    std::map<std::string, uint32_t, iless> level_models;

    ++argv;

    if(*argv && **argv != '-')
    {
        if(!strcmp(*argv, "compile"))
        {
            ++argv;
            action = Action::Compile;
        }
        else if(!strcmp(*argv, "decompile"))
        {
            ++argv;
            action = Action::Decompile;
        }
        else if(!strcmp(*argv, "query-config-path"))
        {
            ++argv;
            action = Action::QueryConfigPath;
            fprintf(stdout, "%s", config_path().generic_u8string().c_str());
            return EXIT_SUCCESS;
        }
        else if(!strcmp(*argv, "query-models"))
        {
            ++argv;
            action = Action::QueryModels;
        }
    }

    if(!parse_args(argv, input, output, data, conf, options))
        return EXIT_FAILURE;

    if(options.help)
    {
        fprintf(stdout, "%s", GTA3SC_HELP_MESSAGE);
        return EXIT_SUCCESS;
    }

    if(options.version)
    {
        auto version = GTA3SC_GIT_DESCRIBE_TAG[0] != '\0'? std::string(GTA3SC_GIT_DESCRIBE_TAG) :
                       GTA3SC_GIT_SHA1[0] != '\0'? std::string(GTA3SC_GIT_BRANCH) + '-' + GTA3SC_GIT_SHA1 : "unknown-version";
        fprintf(stdout, "%s %s", "gta3sc", version.c_str());
        return EXIT_SUCCESS;
    }

    if(input.empty())
    {
        fprintf(stderr, "gta3sc: error: no input file\n");
        return EXIT_FAILURE;
    }

    if(conf.config_name.empty())
    {
        fprintf(stderr, "gta3sc: error: no game config specified [--config=<name>]\n");
        return EXIT_FAILURE;
    }

    if(action == Action::None)
    {
        std::string extension = input.extension().string();
        if(iequal_to()(extension, ".sc"))
            action = Action::Compile;
        else if(iequal_to()(extension, ".scm"))
            action = Action::Decompile;
        else if(iequal_to()(extension, ".scc"))
            action = Action::Decompile;
        else if(iequal_to()(extension, ".cs"))
            action = Action::Decompile;
        else if(iequal_to()(extension, ".cm"))
            action = Action::Decompile;
        else
        {
            fprintf(stderr, "gta3sc: error: could not infer action from input extension (compile/decompile)\n");
            return EXIT_FAILURE;
        }
    }

    if(action != Action::QueryModels)
    {
        if(!options.guesser && options.fswitch)
        {
            fprintf(stderr, "gta3sc: error: use of -fswitch only available in guesser mode [--guesser]\n");
            return EXIT_FAILURE;
        }

        if(!options.guesser && options.farrays)
        {
            fprintf(stderr, "gta3sc: error: use of -farrays only available in guesser mode [--guesser]\n");
            return EXIT_FAILURE;
        }

        if(!options.guesser && options.fconst)
        {
            fprintf(stderr, "gta3sc: error: use of -fconst only available in guesser mode [--guesser]\n");
            return EXIT_FAILURE;
        }

        if(!options.guesser && options.streamed_scripts)
        {
            fprintf(stderr, "gta3sc: error: use of -fstreamed_scripts only available in guesser mode [--guesser]\n");
            return EXIT_FAILURE;
        }

        if(!options.guesser && options.skip_cutscene)
        {
            fprintf(stderr, "gta3sc: error: use of -fskip-cutscene only available in guesser mode [--guesser]\n");
            return EXIT_FAILURE;
        }
    }

    if(!data.datadir.empty())
    {
        if(data.levelfile.empty())
        {
            if(fs::exists(data.datadir / "gta.dat"))
                data.levelfile = "gta.dat";
            else if(fs::exists(data.datadir / "gta3.dat"))
                data.levelfile = "gta3.dat";
            else if(fs::exists(data.datadir / "gta_vc.dat"))
                data.levelfile = "gta_vc.dat";
            else
            {
                fprintf(stderr, "gta3sc: error: could not find level file (gta*.dat) in datadir '%s'\n",
                            data.datadir.generic_u8string().c_str());
                return EXIT_FAILURE;
            }
        }

        try
        {
            default_models = load_dat(data.datadir / "default.dat", true);
            level_models   = load_dat(data.datadir / data.levelfile, false);
        }
        catch(const ConfigError& e)
        {
            fprintf(stderr, "gta3sc: error: %s\n", e.what());
            return EXIT_FAILURE;
        }
    }

    try
    {
        std::vector<fs::path> config_files;
        config_files.reserve(6 + conf.add_config_files.size());

        config_files.emplace_back(config_path() / "gta3sc.xml");
        config_files.emplace_back("alternators.xml");
        config_files.emplace_back("commands.xml");
        config_files.emplace_back("constants.xml");
        if(data.datadir.empty()) config_files.emplace_back("default.xml");
        if(options.cleo) config_files.emplace_back("cleo.xml");
        std::move(conf.add_config_files.begin(), conf.add_config_files.end(), std::back_inserter(config_files));

        Commands commands = Commands::from_xml(conf.config_name, config_files);
        commands.add_default_models(default_models);

        program.emplace(std::move(options), std::move(commands));
        program->setup_models(std::move(default_models), std::move(level_models));
    }
    catch(const ConfigError& e)
    {
        fprintf(stderr, "gta3sc: error: %s\n", e.what());
        return EXIT_FAILURE;
    }

    fs::path conf_path = config_path();
    //fprintf(stderr, "gta3sc: using '%s' as configuration path\n", conf_path.generic_u8string().c_str());

    switch(action)
    {
        case Action::Compile:
            return compile(input, output, *program);
        case Action::Decompile:
            return decompile(input, output, *program);
        case Action::QueryModels:
        {
            if(input == "default" || input == "all")
            {
                fprintf(stdout, "=DEFAULT\n");
                for(auto& pair : program->commands.get_defaultmodel_enum()->values)
                {
                    fprintf(stdout, "%s %u\n", pair.first.c_str(), pair.second);
                }
            }
            if(input == "level" || input == "all")
            {
                fprintf(stdout, "=LEVEL\n");
                for(auto& pair : program->level_models)
                {
                    fprintf(stdout, "%s %u\n", pair.first.c_str(), pair.second);
                }
            }
            return EXIT_SUCCESS;
        }
        default:
            Unreachable();
    }
}
