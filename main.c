#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>

#define MAX_PATH_LEN 1024
#define MAX_CMD_LEN 2048
#define MAX_LINE_LEN 512
#define MAX_SECTION_LEN 64
#define MAX_KEY_LEN 128
#define MAX_VALUE_LEN 512

// Configuration structure
typedef struct {
    // [compiler]
    int experimental;
    char flags[MAX_VALUE_LEN];
    char target[MAX_VALUE_LEN];
    char custom_path[MAX_PATH_LEN];

    // [env]
    char default_env[32];
    char dev_flags[MAX_VALUE_LEN];
    char prod_flags[MAX_VALUE_LEN];
    char test_flags[MAX_VALUE_LEN];

    // [custom]
    char pre_build[MAX_CMD_LEN];
    char post_build[MAX_CMD_LEN];
    char pre_test[MAX_CMD_LEN];
    char post_test[MAX_CMD_LEN];

    // [lint]
    int run_clippy;
    char clippy_flags[MAX_VALUE_LEN];

    // [fmt]
    int auto_format;
    char formatter[MAX_PATH_LEN];
    char formatter_flags[MAX_VALUE_LEN];

    // [binary]
    char output_dir[MAX_PATH_LEN];
    int overwrite;
    int skip_existing;
    int save_backup;

    // [project]
    char name[MAX_VALUE_LEN];
    char version[MAX_VALUE_LEN];
    char author[MAX_VALUE_LEN];
    char description[MAX_VALUE_LEN];

    // [features]
    int enable_experimental;
    int enable_logging;
    int run_on_save;
} Config;

// Global configuration
Config g_config = {0};

// Command line options
typedef struct {
    char file[MAX_PATH_LEN];
    int run_after;
    int release_mode;
    int skip_compilation;
    int save_binary;
    int auto_yes;
    int verbose;
    int very_verbose;
    int use_config;
    char config_path[MAX_PATH_LEN];
    int lint;
    int format;
    char env_mode[32];
    char command[64];
} Options;

// Forward declarations
void print_help(void);
void print_command_help(const char *command);
void print_version(void);
int parse_arguments(int argc, char *argv[], Options *opts);
int load_config(const char *path, Config *config);
void init_default_config(Config *config);
int create_default_config(const char *path);
int file_exists(const char *path);
int is_cargo_project(void);
int execute_command(const char *cmd, int verbose);
int run_pre_post_scripts(const char *script, const char *phase);
int compile_rust_file(const Options *opts);
int run_cargo_command(const char *cmd, const Options *opts);
int format_code(const Options *opts);
int run_clippy(const Options *opts);
int create_project(const char *name);
void trim_whitespace(char *str);
int parse_boolean(const char *value);

// Implementation of utility functions first
void trim_whitespace(char *str) {
    char *start = str;
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
}

int parse_boolean(const char *value) {
    return (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "yes") == 0);
}

int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

int is_cargo_project(void) {
    return file_exists("Cargo.toml");
}

int execute_command(const char *cmd, int verbose) {
    if (verbose) {
        printf("Executing: %s\n", cmd);
    }
    return system(cmd);
}

void print_version(void) {
    printf("rskid version 1.0.0\n");
    execute_command("rustc --version", 0);
    execute_command("cargo --version", 0);
}

void print_help(void) {
    printf("=============================================================\n");
    printf("                        rskid - Rust CLI Wrapper\n");
    printf("=============================================================\n");
    printf("DESCRIPTION:\n");
    printf("rskid is a unified command-line interface for Rust development.\n");
    printf("It wraps rustc, Cargo, and experimental compilers, while providing\n");
    printf("QoL features, interactive prompts, configuration support, and\n");
    printf("project initialization.\n\n");
    printf("COMMANDS:\n");
    printf("  run       : Compile & run a Rust file or Cargo project\n");
    printf("  build     : Build the project using Cargo or rustc\n");
    printf("  test      : Run all tests for the project\n");
    printf("  fmt       : Format Rust code using rustfmt\n");
    printf("  doc       : Generate documentation using cargo doc\n");
    printf("  create    : Alias for creating a new Cargo project\n");
    printf("  clean     : Clean build artifacts\n");
    printf("  list      : List available binaries in Cargo project\n");
    printf("  version   : Show rustc and cargo versions\n");
    printf("  init      : Create new Cargo project + base .rskid.toml config\n\n");
    printf("FLAGS:\n");
    printf("  -f, --file <path>        : Rust source file (optional for Cargo)\n");
    printf("  -R, --run                : Run binary after build\n");
    printf("  -r, --release            : Build in release mode\n");
    printf("  -s, --skip               : Skip compilation if binary exists\n");
    printf("  -S, --save               : Save binary even if it exists\n");
    printf("  -y, --yes                : Auto yes to all prompts\n");
    printf("  -v, --verbose            : Enable verbose logging\n");
    printf("  -V, --very-verbose       : Enable debug logging (extra verbose)\n");
    printf("  -G                       : Use default .rskid.toml configuration\n");
    printf("  --cfg <path>             : Specify custom config path\n");
    printf("  --lint                   : Run cargo clippy after build\n");
    printf("  --fmt                    : Format Rust code before build/run\n");
    printf("  --dev / --prod / --test  : Set environment mode for build/run\n\n");
    printf("EXAMPLES:\n");
    printf("# Create new project with config\n");
    printf("./rskid init my_project\n\n");
    printf("# Compile and run with config\n");
    printf("./rskid -f src/main.rs -R -G\n\n");
    printf("# Build in release mode with formatting and linting\n");
    printf("./rskid build --prod --fmt --lint -G\n\n");
    printf("# Run Cargo project in dev mode\n");
    printf("./rskid run --dev -v\n\n");
    printf("# Format all code\n");
    printf("./rskid fmt\n\n");
    printf("# Run tests with verbose output\n");
    printf("./rskid test -v -G\n\n");
    printf("# Get help for specific command\n");
    printf("./rskid <command> --help\n");
}

void print_command_help(const char *command) {
    if (strcmp(command, "run") == 0) {
        printf("=============================================================\n");
        printf("                        rskid run\n");
        printf("=============================================================\n");
        printf("DESCRIPTION:\n");
        printf("  Compile and run a Rust file or Cargo project.\n");
        printf("  Automatically detects if you're in a Cargo project or\n");
        printf("  working with standalone Rust files.\n\n");
        printf("USAGE:\n");
        printf("  rskid run [OPTIONS]\n");
        printf("  rskid run -f <file> [OPTIONS]\n\n");
        printf("OPTIONS:\n");
        printf("  -f, --file <path>    : Rust source file to compile and run\n");
        printf("  -r, --release        : Build in release mode (optimized)\n");
        printf("  -v, --verbose        : Enable verbose output\n");
        printf("  -G                   : Use .rskid.toml configuration file\n");
        printf("  --cfg <path>         : Use custom configuration file\n");
        printf("  --fmt                : Format code before running\n");
        printf("  --lint               : Run clippy after build\n");
        printf("  --dev                : Use development build settings\n");
        printf("  --prod               : Use production build settings\n\n");
        printf("EXAMPLES:\n");
        printf("  rskid run                    # Run Cargo project\n");
        printf("  rskid run -f main.rs         # Run standalone Rust file\n");
        printf("  rskid run --prod --fmt -G    # Production run with formatting\n");
        printf("  rskid run --dev -v           # Development run with verbose output\n");
    } else if (strcmp(command, "build") == 0) {
        printf("=============================================================\n");
        printf("                        rskid build\n");
        printf("=============================================================\n");
        printf("DESCRIPTION:\n");
        printf("  Build a Rust project or standalone file without running it.\n");
        printf("  Supports both Cargo projects and individual Rust files.\n\n");
        printf("USAGE:\n");
        printf("  rskid build [OPTIONS]\n");
        printf("  rskid build -f <file> [OPTIONS]\n\n");
        printf("OPTIONS:\n");
        printf("  -f, --file <path>    : Rust source file to compile\n");
        printf("  -r, --release        : Build in release mode\n");
        printf("  -S, --save           : Save binary even if it exists\n");
        printf("  -s, --skip           : Skip compilation if binary exists\n");
        printf("  -v, --verbose        : Enable verbose output\n");
        printf("  -G                   : Use .rskid configuration file\n");
        printf("  --fmt                : Format code before building\n");
        printf("  --lint               : Run clippy after build\n");
        printf("  --dev/--prod         : Environment-specific build settings\n\n");
        printf("EXAMPLES:\n");
        printf("  rskid build                  # Build Cargo project\n");
        printf("  rskid build -f src/main.rs   # Build standalone file\n");
        printf("  rskid build --release -G     # Release build with config\n");
    } else if (strcmp(command, "test") == 0) {
        printf("=============================================================\n");
        printf("                        rskid test\n");
        printf("=============================================================\n");
        printf("DESCRIPTION:\n");
        printf("  Run all tests for the Rust project.\n");
        printf("  Executes pre-test and post-test scripts if configured.\n\n");
        printf("USAGE:\n");
        printf("  rskid test [OPTIONS]\n\n");
        printf("OPTIONS:\n");
        printf("  -v, --verbose        : Enable verbose test output\n");
        printf("  -G                   : Use .rskid configuration file\n");
        printf("  --cfg <path>         : Use custom configuration file\n");
        printf("  --test               : Use test-specific build settings\n\n");
        printf("EXAMPLES:\n");
        printf("  rskid test           # Run all tests\n");
        printf("  rskid test -v -G     # Verbose tests with config\n");
    } else if (strcmp(command, "fmt") == 0) {
        printf("=============================================================\n");
        printf("                        rskid fmt\n");
        printf("=============================================================\n");
        printf("DESCRIPTION:\n");
        printf("  Format Rust code using rustfmt.\n");
        printf("  Can format specific files or entire src/ directory.\n\n");
        printf("USAGE:\n");
        printf("  rskid fmt [OPTIONS]\n");
        printf("  rskid fmt -f <file> [OPTIONS]\n\n");
        printf("OPTIONS:\n");
        printf("  -f, --file <path>    : Specific Rust file to format\n");
        printf("  -v, --verbose        : Enable verbose output\n");
        printf("  -G                   : Use .rskid configuration file\n");
        printf("  --fmt                : Use custom formatter settings\n\n");
        printf("EXAMPLES:\n");
        printf("  rskid fmt            # Format all files in src/\n");
        printf("  rskid fmt -f main.rs # Format specific file\n");
        printf("  rskid fmt -G         # Format using config settings\n");
    } else if (strcmp(command, "init") == 0 || strcmp(command, "create") == 0) {
        printf("=============================================================\n");
        printf("                       rskid init/create\n");
        printf("=============================================================\n");
        printf("DESCRIPTION:\n");
        printf("  Create a new Cargo project with base .rskid configuration.\n");
        printf("  If no name is provided, initializes in current directory.\n\n");
        printf("USAGE:\n");
        printf("  rskid init [project_name]\n");
        printf("  rskid create [project_name]\n\n");
        printf("OPTIONS:\n");
        printf("  -v, --verbose        : Enable verbose output\n");
        printf("  -y, --yes            : Auto-confirm all prompts\n\n");
        printf("EXAMPLES:\n");
        printf("  rskid init           # Initialize in current directory\n");
        printf("  rskid init my_app    # Create new project 'my_app'\n");
        printf("  rskid create web_app # Create new project 'web_app'\n");
    } else if (strcmp(command, "clean") == 0) {
        printf("=============================================================\n");
        printf("                        rskid clean\n");
        printf("=============================================================\n");
        printf("DESCRIPTION:\n");
        printf("  Clean build artifacts and target directory.\n");
        printf("  Equivalent to 'cargo clean' for Cargo projects.\n\n");
        printf("USAGE:\n");
        printf("  rskid clean [OPTIONS]\n\n");
        printf("OPTIONS:\n");
        printf("  -v, --verbose        : Enable verbose output\n\n");
        printf("EXAMPLES:\n");
        printf("  rskid clean          # Clean build artifacts\n");
        printf("  rskid clean -v       # Clean with verbose output\n");
    } else if (strcmp(command, "doc") == 0) {
        printf("=============================================================\n");
        printf("                        rskid doc\n");
        printf("=============================================================\n");
        printf("DESCRIPTION:\n");
        printf("  Generate documentation for the Rust project.\n");
        printf("  Uses 'cargo doc' to build HTML documentation.\n\n");
        printf("USAGE:\n");
        printf("  rskid doc [OPTIONS]\n\n");
        printf("OPTIONS:\n");
        printf("  -v, --verbose        : Enable verbose output\n");
        printf("  --dev/--prod         : Environment-specific doc generation\n\n");
        printf("EXAMPLES:\n");
        printf("  rskid doc            # Generate documentation\n");
        printf("  rskid doc -v         # Generate with verbose output\n");
    } else if (strcmp(command, "list") == 0) {
        printf("=============================================================\n");
        printf("                        rskid list\n");
        printf("=============================================================\n");
        printf("DESCRIPTION:\n");
        printf("  List available binary targets in Cargo project.\n");
        printf("  Shows all binaries that can be run.\n\n");
        printf("USAGE:\n");
        printf("  rskid list [OPTIONS]\n\n");
        printf("OPTIONS:\n");
        printf("  -v, --verbose        : Enable verbose output\n\n");
        printf("EXAMPLES:\n");
        printf("  rskid list           # List all binaries\n");
    } else if (strcmp(command, "version") == 0) {
        printf("=============================================================\n");
        printf("                       rskid version\n");
        printf("=============================================================\n");
        printf("DESCRIPTION:\n");
        printf("  Show version information for rskid, rustc, and cargo.\n\n");
        printf("USAGE:\n");
        printf("  rskid version\n\n");
        printf("EXAMPLES:\n");
        printf("  rskid version        # Show all version info\n");
    } else {
        printf("Unknown command: %s\n", command);
        printf("Use 'rskid --help' to see available commands.\n");
    }
}

int parse_arguments(int argc, char *argv[], Options *opts) {
    int command_found = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            if (command_found) {
                print_command_help(opts->command);
            } else {
                print_help();
            }
            exit(0);
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) {
            if (i + 1 < argc) {
                strcpy(opts->file, argv[++i]);
            }
        } else if (strcmp(argv[i], "-R") == 0 || strcmp(argv[i], "--run") == 0) {
            opts->run_after = 1;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--release") == 0) {
            opts->release_mode = 1;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--skip") == 0) {
            opts->skip_compilation = 1;
        } else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--save") == 0) {
            opts->save_binary = 1;
        } else if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--yes") == 0) {
            opts->auto_yes = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            opts->verbose = 1;
        } else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--very-verbose") == 0) {
            opts->very_verbose = 1;
        } else if (strcmp(argv[i], "-G") == 0) {
            opts->use_config = 1;
        } else if (strcmp(argv[i], "--cfg") == 0) {
            if (i + 1 < argc) {
                strcpy(opts->config_path, argv[++i]);
                opts->use_config = 1;
            }
        } else if (strcmp(argv[i], "--lint") == 0) {
            opts->lint = 1;
        } else if (strcmp(argv[i], "--fmt") == 0) {
            opts->format = 1;
        } else if (strcmp(argv[i], "--dev") == 0) {
            strcpy(opts->env_mode, "dev");
        } else if (strcmp(argv[i], "--prod") == 0) {
            strcpy(opts->env_mode, "prod");
        } else if (strcmp(argv[i], "--test") == 0) {
            strcpy(opts->env_mode, "test");
        } else if (argv[i][0] != '-' && !command_found) {
            strcpy(opts->command, argv[i]);
            command_found = 1;
        }
        // Skip other non-flag arguments after command is found
    }
    return 0;
}

void init_default_config(Config *config) {
    config->experimental = 0;
    strcpy(config->flags, "-C opt-level=3");
    strcpy(config->target, "x86_64-unknown-linux-gnu");
    strcpy(config->custom_path, "rustc");
    strcpy(config->default_env, "dev");
    strcpy(config->dev_flags, "");
    strcpy(config->prod_flags, "--release");
    strcpy(config->test_flags, "--all-targets");
    strcpy(config->pre_build, "echo \"Preparing build...\"");
    strcpy(config->post_build, "echo \"Build finished successfully!\"");
    strcpy(config->pre_test, "echo \"Running tests...\"");
    strcpy(config->post_test, "echo \"All tests done!\"");
    config->run_clippy = 1;
    config->auto_format = 1;
    strcpy(config->formatter, "rustfmt");
    strcpy(config->formatter_flags, "--edition 2021");
    strcpy(config->output_dir, "./bin");
    config->overwrite = 0;
    config->skip_existing = 0;
    config->save_backup = 1;
    strcpy(config->name, "MyRustApp");
    strcpy(config->version, "0.1.0");
    strcpy(config->author, "User <user@example.com>");
    strcpy(config->description, "A sample Rust project using rskid");
    config->enable_experimental = 0;
    config->enable_logging = 1;
    config->run_on_save = 0;
}

int create_default_config(const char *path) {
    FILE *file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "Error creating config file: %s\n", strerror(errno));
        return -1;
    }

    fprintf(file, "=============================================================\n");
    fprintf(file, "                      rskid Configuration\n");
    fprintf(file, "=============================================================\n");
    fprintf(file, "[compiler]\n");
    fprintf(file, "# Use experimental compiler (rustcc) if true\n");
    fprintf(file, "experimental=false\n");
    fprintf(file, "# Additional flags for rustc/rustcc\n");
    fprintf(file, "flags=-C opt-level=3\n");
    fprintf(file, "# Compilation target (optional)\n");
    fprintf(file, "target=x86_64-unknown-linux-gnu\n");
    fprintf(file, "# Custom path to rustc/rustcc (optional)\n");
    fprintf(file, "custom_path=rustc\n\n");

    fprintf(file, "[env]\n");
    fprintf(file, "# Default environment mode\n");
    fprintf(file, "default_env=dev\n");
    fprintf(file, "# Flags for dev build\n");
    fprintf(file, "dev_flags=\n");
    fprintf(file, "# Flags for production build\n");
    fprintf(file, "prod_flags=--release\n");
    fprintf(file, "# Flags for tests\n");
    fprintf(file, "test_flags=--all-targets\n\n");

    fprintf(file, "[custom]\n");
    fprintf(file, "# Commands executed before build\n");
    fprintf(file, "pre_build=echo \"Preparing build...\"\n");
    fprintf(file, "# Commands executed after build\n");
    fprintf(file, "post_build=echo \"Build finished successfully!\"\n");
    fprintf(file, "# Commands before tests (optional)\n");
    fprintf(file, "pre_test=echo \"Running tests...\"\n");
    fprintf(file, "# Commands after tests (optional)\n");
    fprintf(file, "post_test=echo \"All tests done!\"\n\n");

    fprintf(file, "[lint]\n");
    fprintf(file, "# Enable clippy\n");
    fprintf(file, "run_clippy=true\n");
    fprintf(file, "# Custom clippy flags\n");
    fprintf(file, "clippy_flags=--deny warnings\n\n");

    fprintf(file, "[fmt]\n");
    fprintf(file, "# Automatically format code\n");
    fprintf(file, "auto_format=true\n");
    fprintf(file, "# Formatter executable\n");
    fprintf(file, "formatter=rustfmt\n");
    fprintf(file, "# Formatter flags\n");
    fprintf(file, "formatter_flags=--edition 2021\n\n");

    fprintf(file, "[binary]\n");
    fprintf(file, "# Directory to save compiled binaries\n");
    fprintf(file, "output_dir=./bin\n");
    fprintf(file, "# Automatically overwrite binaries\n");
    fprintf(file, "overwrite=false\n");
    fprintf(file, "# Skip compilation if binary exists\n");
    fprintf(file, "skip_existing=false\n");
    fprintf(file, "# Save old binary as backup\n");
    fprintf(file, "save_backup=true\n\n");

    fprintf(file, "[project]\n");
    fprintf(file, "# Project metadata\n");
    fprintf(file, "name=MyRustApp\n");
    fprintf(file, "version=0.1.0\n");
    fprintf(file, "author=User <user@example.com>\n");
    fprintf(file, "description=A sample Rust project using rskid\n\n");

    fprintf(file, "[features]\n");
    fprintf(file, "# Enable experimental compiler features at runtime\n");
    fprintf(file, "enable_experimental=false\n");
    fprintf(file, "# Enable verbose logging\n");
    fprintf(file, "enable_logging=true\n");
    fprintf(file, "# Automatically run binary after build/save\n");
    fprintf(file, "run_on_save=false\n");

    fclose(file);
    printf("Created default config file: %s\n", path);
    return 0;
}

int load_config(const char *path, Config *config) {
    FILE *file = fopen(path, "r");
    if (!file) {
        return -1;
    }

    char line[MAX_LINE_LEN];
    char current_section[MAX_SECTION_LEN] = "";

    while (fgets(line, sizeof(line), file)) {
        trim_whitespace(line);

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        // Parse section headers
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (end) {
                *end = '\0';
                strcpy(current_section, line + 1);
            }
            continue;
        }

        // Parse key-value pairs
        char *equals = strchr(line, '=');
        if (equals) {
            *equals = '\0';
            char *key = line;
            char *value = equals + 1;
            trim_whitespace(key);
            trim_whitespace(value);

            // Parse based on current section
            if (strcmp(current_section, "compiler") == 0) {
                if (strcmp(key, "experimental") == 0) {
                    config->experimental = parse_boolean(value);
                } else if (strcmp(key, "flags") == 0) {
                    strcpy(config->flags, value);
                } else if (strcmp(key, "target") == 0) {
                    strcpy(config->target, value);
                } else if (strcmp(key, "custom_path") == 0) {
                    strcpy(config->custom_path, value);
                }
            } else if (strcmp(current_section, "env") == 0) {
                if (strcmp(key, "default_env") == 0) {
                    strcpy(config->default_env, value);
                } else if (strcmp(key, "dev_flags") == 0) {
                    strcpy(config->dev_flags, value);
                } else if (strcmp(key, "prod_flags") == 0) {
                    strcpy(config->prod_flags, value);
                } else if (strcmp(key, "test_flags") == 0) {
                    strcpy(config->test_flags, value);
                }
            } else if (strcmp(current_section, "custom") == 0) {
                if (strcmp(key, "pre_build") == 0) {
                    strcpy(config->pre_build, value);
                } else if (strcmp(key, "post_build") == 0) {
                    strcpy(config->post_build, value);
                } else if (strcmp(key, "pre_test") == 0) {
                    strcpy(config->pre_test, value);
                } else if (strcmp(key, "post_test") == 0) {
                    strcpy(config->post_test, value);
                }
            } else if (strcmp(current_section, "lint") == 0) {
                if (strcmp(key, "run_clippy") == 0) {
                    config->run_clippy = parse_boolean(value);
                } else if (strcmp(key, "clippy_flags") == 0) {
                    strcpy(config->clippy_flags, value);
                }
            } else if (strcmp(current_section, "fmt") == 0) {
                if (strcmp(key, "auto_format") == 0) {
                    config->auto_format = parse_boolean(value);
                } else if (strcmp(key, "formatter") == 0) {
                    strcpy(config->formatter, value);
                } else if (strcmp(key, "formatter_flags") == 0) {
                    strcpy(config->formatter_flags, value);
                }
            } else if (strcmp(current_section, "binary") == 0) {
                if (strcmp(key, "output_dir") == 0) {
                    strcpy(config->output_dir, value);
                } else if (strcmp(key, "overwrite") == 0) {
                    config->overwrite = parse_boolean(value);
                } else if (strcmp(key, "skip_existing") == 0) {
                    config->skip_existing = parse_boolean(value);
                } else if (strcmp(key, "save_backup") == 0) {
                    config->save_backup = parse_boolean(value);
                }
            } else if (strcmp(current_section, "project") == 0) {
                if (strcmp(key, "name") == 0) {
                    strcpy(config->name, value);
                } else if (strcmp(key, "version") == 0) {
                    strcpy(config->version, value);
                } else if (strcmp(key, "author") == 0) {
                    strcpy(config->author, value);
                } else if (strcmp(key, "description") == 0) {
                    strcpy(config->description, value);
                }
            } else if (strcmp(current_section, "features") == 0) {
                if (strcmp(key, "enable_experimental") == 0) {
                    config->enable_experimental = parse_boolean(value);
                } else if (strcmp(key, "enable_logging") == 0) {
                    config->enable_logging = parse_boolean(value);
                } else if (strcmp(key, "run_on_save") == 0) {
                    config->run_on_save = parse_boolean(value);
                }
            }
        }
    }

    fclose(file);
    return 0;
}

int run_pre_post_scripts(const char *script, const char *phase) {
    if (strlen(script) > 0) {
        printf("Running %s script...\n", phase);
        return execute_command(script, 1);
    }
    return 0;
}

int compile_rust_file(const Options *opts) {
    char cmd[MAX_CMD_LEN];
    char *compiler = g_config.experimental ? "rustcc" :
                    (strlen(g_config.custom_path) > 0 ? g_config.custom_path : "rustc");

    // Get filename without extension for output
    char file_copy[MAX_PATH_LEN];
    strncpy(file_copy, opts->file, sizeof(file_copy) - 1);
    file_copy[sizeof(file_copy) - 1] = '\0';
    char *filename = basename(file_copy);
    char *dot = strrchr(filename, '.');
    if (dot) *dot = '\0';

    // Create output directory if needed
    if (strlen(g_config.output_dir) > 0) {
        char mkdir_cmd[MAX_CMD_LEN];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", g_config.output_dir);
        execute_command(mkdir_cmd, opts->verbose);
    }

    // Build command with bounds checking
    int result = snprintf(cmd, sizeof(cmd), "%s %s", compiler, g_config.flags);
    if ((size_t)result >= sizeof(cmd)) {
        fprintf(stderr, "Error: Initial command too long\n");
        return -1;
    }

    // Add environment-specific flags
    if (opts->release_mode || strcmp(opts->env_mode, "prod") == 0) {
        if (strlen(g_config.prod_flags) > 0) {
            size_t cmd_len = strlen(cmd);
            size_t remaining = sizeof(cmd) - cmd_len;
            result = snprintf(cmd + cmd_len, remaining, " %s", g_config.prod_flags);
            if ((size_t)result >= remaining) {
                fprintf(stderr, "Error: Command with prod flags too long\n");
                return -1;
            }
        }
    } else if (strcmp(opts->env_mode, "dev") == 0) {
        if (strlen(g_config.dev_flags) > 0) {
            size_t cmd_len = strlen(cmd);
            size_t remaining = sizeof(cmd) - cmd_len;
            result = snprintf(cmd + cmd_len, remaining, " %s", g_config.dev_flags);
            if ((size_t)result >= remaining) {
                fprintf(stderr, "Error: Command with dev flags too long\n");
                return -1;
            }
        }
    }

    // Add target if specified
    if (strlen(g_config.target) > 0) {
        size_t cmd_len = strlen(cmd);
        size_t remaining = sizeof(cmd) - cmd_len;
        result = snprintf(cmd + cmd_len, remaining, " --target %s", g_config.target);
        if ((size_t)result >= remaining) {
            fprintf(stderr, "Error: Command with target too long\n");
            return -1;
        }
    }

    char output_path[MAX_PATH_LEN];
    const char *output_dir = strlen(g_config.output_dir) > 0 ? g_config.output_dir : ".";
    int path_result = snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, filename);
    if ((size_t)path_result >= sizeof(output_path)) {
        fprintf(stderr, "Error: Output path too long\n");
        return -1;
    }

    // Build the final command with bounds checking
    size_t cmd_len = strlen(cmd);
    size_t remaining = sizeof(cmd) - cmd_len;

    // Check if we have enough space before trying to append
    size_t needed = strlen(" -o ") + strlen(output_path) + strlen(" ") + strlen(opts->file) + 1;
    if (needed > remaining) {
        fprintf(stderr, "Error: Final command would be too long\n");
        return -1;
    }

    result = snprintf(cmd + cmd_len, remaining, " -o %s %s", output_path, opts->file);
    if ((size_t)result >= remaining) {
        fprintf(stderr, "Error: Command construction failed\n");
        return -1;
    }

    result = execute_command(cmd, opts->verbose || opts->very_verbose);

    // Run the binary if requested
    if (result == 0 && (opts->run_after || g_config.run_on_save)) {
        char run_cmd[MAX_CMD_LEN];
        snprintf(run_cmd, sizeof(run_cmd), "./%s", output_path);
        execute_command(run_cmd, opts->verbose);
    }

    return result;
}

int run_cargo_command(const char *cmd, const Options *opts) {
    char full_cmd[MAX_CMD_LEN];
    int result = snprintf(full_cmd, sizeof(full_cmd), "cargo %s", cmd);
    if ((size_t)result >= sizeof(full_cmd)) {
        fprintf(stderr, "Error: Cargo command too long\n");
        return -1;
    }

    if (opts->release_mode || strcmp(opts->env_mode, "prod") == 0) {
        if (strlen(g_config.prod_flags) > 0) {
            size_t cmd_len = strlen(full_cmd);
            size_t remaining = sizeof(full_cmd) - cmd_len;
            result = snprintf(full_cmd + cmd_len, remaining, " %s", g_config.prod_flags);
            if ((size_t)result >= remaining) {
                fprintf(stderr, "Error: Cargo command with prod flags too long\n");
                return -1;
            }
        }
    } else if (strcmp(opts->env_mode, "test") == 0) {
        if (strlen(g_config.test_flags) > 0) {
            size_t cmd_len = strlen(full_cmd);
            size_t remaining = sizeof(full_cmd) - cmd_len;
            result = snprintf(full_cmd + cmd_len, remaining, " %s", g_config.test_flags);
            if ((size_t)result >= remaining) {
                fprintf(stderr, "Error: Cargo command with test flags too long\n");
                return -1;
            }
        }
    }

    if (opts->verbose) {
        size_t cmd_len = strlen(full_cmd);
        size_t remaining = sizeof(full_cmd) - cmd_len;
        result = snprintf(full_cmd + cmd_len, remaining, " --verbose");
        if ((size_t)result >= remaining) {
            fprintf(stderr, "Error: Cargo command with verbose flag too long\n");
            return -1;
        }
    }

    return execute_command(full_cmd, opts->verbose || opts->very_verbose);
}

int format_code(const Options *opts) {
    char cmd[MAX_CMD_LEN];
    int result = snprintf(cmd, sizeof(cmd), "%s %s", g_config.formatter, g_config.formatter_flags);
    if ((size_t)result >= sizeof(cmd)) {
        fprintf(stderr, "Error: Format command too long\n");
        return -1;
    }

    const char *target = (strlen(opts->file) > 0) ? opts->file : "src/";
    size_t cmd_len = strlen(cmd);
    size_t remaining = sizeof(cmd) - cmd_len;
    result = snprintf(cmd + cmd_len, remaining, " %s", target);
    if ((size_t)result >= remaining) {
        fprintf(stderr, "Error: Format command with target too long\n");
        return -1;
    }

    return execute_command(cmd, opts->verbose);
}

int run_clippy(const Options *opts) {
    char cmd[MAX_CMD_LEN];
    int result = snprintf(cmd, sizeof(cmd), "cargo clippy %s", g_config.clippy_flags);
    if ((size_t)result >= sizeof(cmd)) {
        fprintf(stderr, "Error: Clippy command too long\n");
        return -1;
    }

    return execute_command(cmd, opts->verbose);
}

int create_project(const char *name) {
    if (strcmp(name, ".") == 0) {
        // Initialize in current directory
        printf("Initializing rskid project in current directory...\n");

        // Check if Cargo.toml already exists
        if (!file_exists("Cargo.toml")) {
            int result = execute_command("cargo init", 1);
            if (result != 0) {
                fprintf(stderr, "Failed to initialize Cargo project\n");
                return result;
            }
        }

        // Create .rskid config
        if (!file_exists(".rskid")) {
            create_default_config(".rskid");
        } else {
            printf("Configuration file .rskid already exists\n");
        }

        return 0;
    } else {
        // Create new project directory
        char cmd[MAX_CMD_LEN];
        int result = snprintf(cmd, sizeof(cmd), "cargo new %s", name);
        if ((size_t)result >= sizeof(cmd)) {
            fprintf(stderr, "Project name too long\n");
            return -1;
        }

        result = execute_command(cmd, 1);

        if (result == 0) {
            // Create .rskid config in the new project
            char config_path[MAX_PATH_LEN];
            result = snprintf(config_path, sizeof(config_path), "%s/.rskid", name);
            if ((size_t)result >= sizeof(config_path)) {
                fprintf(stderr, "Config path too long\n");
                return -1;
            }
            create_default_config(config_path);
        }

        return result;
    }
}

int main(int argc, char *argv[]) {
    Options opts = {0};
    strcpy(opts.env_mode, "dev");
    strcpy(opts.command, "run");

    if (argc < 2) {
        print_help();
        return 1;
    }

    if (parse_arguments(argc, argv, &opts) != 0) {
        return 1;
    }

    // Load configuration if specified
    if (opts.use_config) {
        const char *config_path = strlen(opts.config_path) > 0 ?
            opts.config_path : ".rskid.toml";

        if (!file_exists(config_path)) {
            if (!opts.auto_yes) {
                printf("Config file '%s' not found. Create default? (y/n): ", config_path);
                char response;
                scanf(" %c", &response);
                if (response == 'y' || response == 'Y') {
                    create_default_config(config_path);
                }
            } else {
                create_default_config(config_path);
            }
        }

        if (file_exists(config_path)) {
            load_config(config_path, &g_config);
        } else {
            init_default_config(&g_config);
        }
    } else {
        init_default_config(&g_config);
    }

    // Handle different commands
    if (strcmp(opts.command, "version") == 0) {
        print_version();
        return 0;
    } else if (strcmp(opts.command, "init") == 0 || strcmp(opts.command, "create") == 0) {
        const char *project_name = ".";

        // Look for project name in remaining arguments
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], opts.command) == 0 && i + 1 < argc) {
                // Check if next argument is not a flag
                if (argv[i + 1][0] != '-') {
                    project_name = argv[i + 1];
                }
                break;
            }
        }

        return create_project(project_name);
    } else if (strcmp(opts.command, "clean") == 0) {
        return run_cargo_command("clean", &opts);
    } else if (strcmp(opts.command, "test") == 0) {
        if (strlen(g_config.pre_test) > 0) {
            run_pre_post_scripts(g_config.pre_test, "pre-test");
        }
        int result = run_cargo_command("test", &opts);
        if (strlen(g_config.post_test) > 0) {
            run_pre_post_scripts(g_config.post_test, "post-test");
        }
        return result;
    } else if (strcmp(opts.command, "fmt") == 0) {
        return format_code(&opts);
    } else if (strcmp(opts.command, "doc") == 0) {
        return run_cargo_command("doc", &opts);
    } else if (strcmp(opts.command, "list") == 0) {
        return run_cargo_command("run --bin", &opts);
    } else if (strcmp(opts.command, "build") == 0 || strcmp(opts.command, "run") == 0) {
        // Format code if requested
        if (opts.format || g_config.auto_format) {
            format_code(&opts);
        }

        // Run pre-build scripts
        if (strlen(g_config.pre_build) > 0) {
            run_pre_post_scripts(g_config.pre_build, "pre-build");
        }

        int result;
        if (is_cargo_project()) {
            result = run_cargo_command(strcmp(opts.command, "build") == 0 ? "build" : "run", &opts);
        } else {
            result = compile_rust_file(&opts);
        }

        // Run clippy if requested
        if ((opts.lint || g_config.run_clippy) && result == 0) {
            run_clippy(&opts);
        }

        // Run post-build scripts
        if (strlen(g_config.post_build) > 0) {
            run_pre_post_scripts(g_config.post_build, "post-build");
        }

        return result;
    }

    fprintf(stderr, "Unknown command: %s\n", opts.command);
    return 1;
}
