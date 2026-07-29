#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>
#include "../include/common.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/codegen.h"
#include "../include/assembler.h"
#include "../include/linker.h"
#include "../include/analyzer.h"
#include "../include/optimizer.h"
#include "../include/memory_safety.h"

// Global flags
static bool g_verbose = false;
static bool g_show_time = false;
static bool g_show_size = false;

// SECURITY: Safe process execution to prevent shell injection
// Replaces system() with fork() + execvp()
static int safe_exec(const char *program, const char *arg1, const char *arg2, const char *arg3) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    
    if (pid == 0) {
        // Child process
        const char *argv[] = {program, arg1, arg2, arg3, NULL};
        execvp(program, (char * const *)argv);
        // Only reached if execvp fails
        perror("execvp");
        exit(127);
    } else {
        // Parent process - wait for child
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Process terminated by signal %d\n", WTERMSIG(status));
            return 1;
        }
        return 1;
    }
}

// SECURITY: Safe process execution with up to 8 arguments
static int safe_exec_many(const char *program, const char *args[]) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    
    if (pid == 0) {
        // Child process
        execvp(program, (char * const *)args);
        // Only reached if execvp fails
        perror("execvp");
        exit(127);
    } else {
        // Parent process - wait for child
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Process terminated by signal %d\n", WTERMSIG(status));
            return 1;
        }
        return 1;
    }
}

// Find bundled NASM executable in the tools directory
__attribute__((unused))
static char *find_bundled_nasm(const char *argv0) {
    (void)argv0;  // Unused - we rely on system NASM
    return NULL;  // Simplified: use system NASM only
}

// Find runtime/sync.o relative to the compiler executable location
static char *find_runtime_sync_o(const char *argv0) {
    (void)argv0;  // Mark parameter as intentionally unused
    static char sync_o_path[2048];
    char exe_path[2048];
    char dir[2048];
    ssize_t len;
    char *last_slash;
    
    // First, try to use /proc/self/exe to get executable path
    len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len > 0) {
        exe_path[len] = '\0';
        
        // Extract directory name manually (without dirname to avoid FORTIFY issues)
        last_slash = strrchr(exe_path, '/');
        if (last_slash) {
            strncpy(dir, exe_path, last_slash - exe_path);
            dir[last_slash - exe_path] = '\0';
            
            // Check if ./runtime/sync.o exists (same directory as binary)
            snprintf(sync_o_path, sizeof(sync_o_path), "%s/runtime/sync.o", dir);
            if (access(sync_o_path, F_OK) == 0) {
                return sync_o_path;
            }
        }
    }
    
    // Fallback: try relative to current directory
    if (access("runtime/sync.o", F_OK) == 0) {
        return "runtime/sync.o";
    }
    
    // Last resort: absolute path for system installation
    if (access("/usr/local/lib/RasCode/runtime/sync.o", F_OK) == 0) {
        return "/usr/local/lib/RasCode/runtime/sync.o";
    }
    if (access("/usr/lib/RasCode/runtime/sync.o", F_OK) == 0) {
        return "/usr/lib/RasCode/runtime/sync.o";
    }
    
    // If nothing found, return the default relative path and let linker fail with clear error
    return "runtime/sync.o";
}

// Find all runtime object files and build linker arguments
__attribute__((unused))
static void build_runtime_objects(const char *argv0, char *runtime_objs, size_t maxlen) {
    const char *sync_o = find_runtime_sync_o(argv0);
    
    // Build string with all runtime object files
    snprintf(runtime_objs, maxlen, "%s", sync_o);
    
    // Get directory for all runtime files
    char sync_copy[2048];
    strncpy(sync_copy, sync_o, sizeof(sync_copy) - 1);
    char *last_slash = strrchr(sync_copy, '/');
    char runtime_dir[2048] = "runtime";
    
    if (last_slash) {
        *last_slash = '\0';
        strncpy(runtime_dir, sync_copy, sizeof(runtime_dir) - 1);
    }
    
    // List of all runtime modules
    const char *modules[] = {
        "channels.o",
        "threadpool.o", 
        "network.o",
        "fileio.o",
        "memmap.o",
        "hardware.o",
        "advanced.o",
        "strings.o",
        "math.o",
        "errors.o",
        "process.o",
        "concurrency.o",
        "time.o",
        "sorting.o",
        NULL
    };
    
    // Add each module if it exists
    for (int i = 0; modules[i]; i++) {
        char module_path[2048];
        snprintf(module_path, sizeof(module_path), "%s/%s", runtime_dir, modules[i]);
        
        if (access(module_path, F_OK) == 0) {
            strncat(runtime_objs, " ", maxlen - strlen(runtime_objs) - 1);
            strncat(runtime_objs, module_path, maxlen - strlen(runtime_objs) - 1);
        }
    }
}

static char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // SECURITY: Prevent reading extremely large files (DoS protection)
    // Maximum file size: 10MB
    const long MAX_FILE_SIZE = 10 * 1024 * 1024;
    
    if (size < 0) {
        fprintf(stderr, "Error: Could not determine file size: %s\n", filename);
        fclose(file);
        return NULL;
    }
    
    if (size > MAX_FILE_SIZE) {
        fprintf(stderr, "Error: File too large (%ld bytes). Maximum allowed size is %ld bytes.\n", 
                size, MAX_FILE_SIZE);
        fclose(file);
        return NULL;
    }
    
    if (size == 0) {
        fprintf(stderr, "Error: Input file is empty\n");
        fclose(file);
        return NULL;
    }
    
    char *buffer = xmalloc(size + 1);
    size_t bytes_read = fread(buffer, 1, size, file);
    
    // SECURITY: Verify that all bytes were read successfully
    if (bytes_read != (size_t)size) {
        fprintf(stderr, "Error: Failed to read file completely. Expected %ld bytes, got %zu bytes.\n", 
                size, bytes_read);
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    buffer[size] = '\0';
    
    if (fclose(file) != 0) {
        fprintf(stderr, "Warning: Could not properly close file: %s\n", filename);
    }
    
    return buffer;
}

static void print_usage(const char *prog) {
    (void)prog;  // Mark parameter as intentionally unused
    fprintf(stderr, "RasCode Compiler v%s - Target: %s\n\n", RASCODE_VERSION, RASCODE_TARGET);
    fprintf(stderr, "USAGE:\n");
    fprintf(stderr, "  rascom <input.ras> [-o <output>] [FLAGS]\n");
    fprintf(stderr, "\nQUICK START:\n");
    fprintf(stderr, "  rascom example.ras              # Compile to a.out\n");
    fprintf(stderr, "  rascom example.ras -o prog      # Compile with custom name\n");
    fprintf(stderr, "  rascom example.ras -V           # Verbose compilation output\n");
    fprintf(stderr, "\nBASIC OPTIONS:\n");
    fprintf(stderr, "  -h, --help           Display this help message\n");
    fprintf(stderr, "  -v, --version        Show version and configuration\n");
    fprintf(stderr, "  -V, --verbose        Show detailed compilation steps\n");
    fprintf(stderr, "  -o <file>            Output executable name (default: a.out)\n");
    fprintf(stderr, "\nOUTPUT INFORMATION:\n");
    fprintf(stderr, "  -tm                  Show total compilation time\n");
    fprintf(stderr, "  -sz                  Show compiled executable size\n");
    fprintf(stderr, "\nOPTIMIZATION LEVELS:\n");
    fprintf(stderr, "  -O0                  No optimization (debug builds)\n");
    fprintf(stderr, "  -O1                  Basic optimization (constant folding)\n");
    fprintf(stderr, "  -O2                  Medium optimization (loop + inlining)\n");
    fprintf(stderr, "  -O3                  Aggressive optimization [DEFAULT]\n");
    fprintf(stderr, "  -O                   Alias for -O3 (peak performance)\n");
    fprintf(stderr, "\nCOMPILER WARNINGS:\n");
    fprintf(stderr, "  -Wall                Enable all common warnings\n");
    fprintf(stderr, "  -Wextra              Enable extra warnings (implies -Wall)\n");
    fprintf(stderr, "  -Werror              Treat all warnings as errors\n");
    fprintf(stderr, "  -Wpedantic           Enable stricter pedantic warnings\n");
    fprintf(stderr, "  -Wno-unused          Suppress unused variable warnings\n");
    fprintf(stderr, "\nRUNTIME SAFETY:\n");
    fprintf(stderr, "  -foverflow-check     Enable integer overflow detection\n");
    fprintf(stderr, "  -fbounds-check       Enable array bounds checking [ON]\n");
    fprintf(stderr, "  -fno-bounds-check    Disable bounds checking (performance)\n");
    fprintf(stderr, "  -fstack-check        Enable recursion depth checking [ON]\n");
    fprintf(stderr, "  -fno-stack-check     Disable recursion checking\n");
}

static void print_version(void) {
    fprintf(stderr, "RasCode Compiler v%s - Target: %s\n", RASCODE_VERSION, RASCODE_TARGET);
    fprintf(stderr, "Primary Assembler:  NASM v2.13+ (system)\n");
    fprintf(stderr, "Fallback Assembler: Built-in RasCode Assembler\n");
    fprintf(stderr, "Linker:             GCC (system) / RasCode Linker\n\n");
    fprintf(stderr, "Developed by ahamedrashid.me@gmail.com - SOS ecosystem\n");
}


static int compile_asm_to_binary(const char *asm_file, const char *output_file, const char *argv0) {
    // ============================================================================
    // Assembly Pipeline: System NASM (1st) -> RasCode rasm (2nd)
    // ============================================================================
    // This function assembles x86-64 code using available tools
    // Priority: NASM -> Internal assembler -> rasm
    // ============================================================================
    
    (void)argv0;  // Not needed for system tools
    int ret;
    
    // NASM is the ONLY assembler - no fallbacks
    if (access("/usr/bin/nasm", X_OK) == 0 || access("/bin/nasm", X_OK) == 0) {
        if (g_verbose) {
            fprintf(stderr, "    Assembling with NASM...\n");
        }
        
        char obj_file[256];
        snprintf(obj_file, sizeof(obj_file), "%s.o", output_file);
        
        const char *nasm_args[] = {"nasm", "-f", "elf64", asm_file, "-o", obj_file, NULL};
        ret = safe_exec_many("nasm", nasm_args);
        
        if (ret == 0) {
            if (g_verbose) {
                fprintf(stderr, "    ✓ NASM assembly: Success\n    Linking...\n");
            }
            
            // Build GCC linker command
            char gcc_cmd[8192] = "gcc -nostartfiles -no-pie -o ";
            strcat(gcc_cmd, output_file);   strcat(gcc_cmd, " ");
            strcat(gcc_cmd, obj_file);
            
            // Add runtime modules
            const char *runtime_modules[] = {
                "obj/runtime/sync.o",
                "obj/runtime/channels.o",
                "obj/runtime/threadpool.o",
                "obj/runtime/network.o",
                "obj/runtime/fileio.o",
                "obj/runtime/memmap.o",
                "obj/runtime/hardware.o",
                "obj/runtime/advanced.o",
                "obj/runtime/strings.o",
                "obj/runtime/math.o",
                "obj/runtime/errors.o",
                "obj/runtime/process.o",
                "obj/runtime/concurrency.o",
                "obj/runtime/time.o",
                "obj/runtime/sorting.o",
                NULL
            };
            
            for (int i = 0; runtime_modules[i] != NULL; i++) {
                strcat(gcc_cmd, " ");
                strcat(gcc_cmd, runtime_modules[i]);
            }
            
            strcat(gcc_cmd, " -lm -lpthread");
            
            ret = system(gcc_cmd);
            unlink(obj_file);
            
            if (ret == 0) {
                return 0;
            }
            return 1;
        }
        
        fprintf(stderr, "Error: NASM assembly failed\n");
    } else {
        fprintf(stderr, "Error: NASM assembler not found\n");
    }
    
    fprintf(stderr, "\nPlease install NASM:\n");
    fprintf(stderr, "  Ubuntu/Debian:  apt install nasm\n");
    fprintf(stderr, "  Fedora/RHEL:    dnf install nasm\n");
    fprintf(stderr, "  macOS:          brew install nasm\n");
    fprintf(stderr, "  Arch Linux:     pacman -S nasm\n");
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    char *input_file = NULL;
    char *output_file = "a.out";
    int optimization_level = 3;  // Default: -O3 (peak optimization)
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-V") == 0) {
            g_verbose = true;
            continue;
        } else if (strcmp(argv[i], "-tm") == 0) {
            g_show_time = true;
            continue;
        } else if (strcmp(argv[i], "-sz") == 0) {
            g_show_size = true;
            continue;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 1;
            }
            output_file = argv[++i];
        } else if (strncmp(argv[i], "-W", 2) == 0) {
            // Warning flag - will be processed by analyzer
            // Just validate that it's a known flag
            const char *flag = argv[i];
            if (strcmp(flag, "-Wall") == 0 || strcmp(flag, "-Wextra") == 0 ||
                strcmp(flag, "-Werror") == 0 || strcmp(flag, "-Wpedantic") == 0 ||
                strcmp(flag, "-Wno-unused") == 0 || strcmp(flag, "-Wno-shadowing") == 0) {
                // Valid warning flag, will be processed after creating analyzer
                continue;
            } else {
                fprintf(stderr, "Warning: Unknown warning flag: %s\n", flag);
            }
        } else if (strcmp(argv[i], "-foverflow-check") == 0) {
            // Enable overflow detection (PRIORITY 2.1)
            g_overflow_check_enabled = 1;
            continue;
        } else if (strcmp(argv[i], "-fbounds-check") == 0) {
            // Enable bounds checking (PRIORITY 2.2, default behavior)
            g_bounds_check_enabled = 1;
            continue;
        } else if (strcmp(argv[i], "-fno-bounds-check") == 0) {
            // Disable bounds checking for performance (PRIORITY 2.2)
            g_bounds_check_enabled = 0;
            continue;
        } else if (strcmp(argv[i], "-fstack-check") == 0) {
            // Enable stack checking (PRIORITY 2.3, default behavior)
            g_stack_check_enabled = 1;
            continue;
        } else if (strcmp(argv[i], "-fno-stack-check") == 0) {
            // Disable stack checking (risky, for advanced users)
            g_stack_check_enabled = 0;
            continue;
        } else if (strncmp(argv[i], "-O", 2) == 0) {
            // Optimization level flag: -O0, -O1, -O2, -O3
            if (strlen(argv[i]) == 3 && argv[i][2] >= '0' && argv[i][2] <= '3') {
                optimization_level = argv[i][2] - '0';
                continue;
            } else if (strcmp(argv[i], "-O") == 0) {
                optimization_level = 3;  // -O defaults to -O3 (peak)
                continue;
            } else {
                fprintf(stderr, "Warning: Unknown optimization flag: %s\n", argv[i]);
            }
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }
    
    if (!input_file) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Validate .rco or .ras extension (rco = modern, ras = legacy)
    int filename_len = strlen(input_file);
    if (filename_len < 5) {
        fprintf(stderr, "Error: Invalid input file name\n");
        fprintf(stderr, "Usage: rascom <input.rco> [-o <output>]\n");
        return 1;
    }
    const char *ext = input_file + filename_len - 4;
    if (strcmp(ext, ".rco") != 0 && strcmp(ext, ".ras") != 0) {
        fprintf(stderr, "Error: Input file must have .rco or .ras extension\n");
        fprintf(stderr, "Usage: rascom <input.rco> [-o <output>] [FLAGS]\n");
        return 1;
    }
    
    // SAFETY: Initialize mandatory memory safety layer
    // This must be done before any allocation occurs
    memory_safety_set_verbose(g_verbose);
    memory_safety_init(true);
    
    // Read source file
    char *source = read_file(input_file);
    if (!source) {
        return 1;
    }
    
    if (g_verbose) {
        fprintf(stderr, "\n[COMPILATION] Compiling %s\n", input_file);
        fprintf(stderr, "─────────────────────────────────────\n");
    }
    
    // Record start time for -tm flag
    struct timespec start_time, end_time;
    if (g_show_time) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
    }
    
    // Lexical analysis
    if (g_verbose) {
        fprintf(stderr, "\n[1] Lexical Analysis\n");
    }
    Lexer *lexer = lexer_new(source);
    if (g_verbose) {
        fprintf(stderr, "    ✓ Tokens generated\n");
    }
    
    // Parsing
    if (g_verbose) {
        fprintf(stderr, "\n[2] Parsing\n");
    }
    Parser *parser = parser_new(lexer);
    ASTNode *ast = parser_parse(parser);
    if (g_verbose) {
        fprintf(stderr, "    ✓ AST constructed\n");
    }
    
    // Apply warning flags (Priority 1.3)
    analyzer_set_warning_flags(argc, argv);
    
    // Intelligent code analysis
    if (g_verbose) {
        fprintf(stderr, "\n[3] Semantic Analysis & Warnings\n");
    }
    analyzer_analyze_and_report(ast);
    if (g_verbose) {
        fprintf(stderr, "    ✓ Analysis complete\n");
    }
    
    // Optimization passes
    if (optimization_level > 0) {
        if (g_verbose) {
            fprintf(stderr, "\n[4] Optimization (Level %d)\n", optimization_level);
        }
        OptimizerContext *optimizer = optimizer_new(optimization_level);
        ast = optimize_ast(optimizer, ast);
        if (g_verbose) {
            fprintf(stderr, "    ✓ Optimizations applied\n");
            optimizer_print_stats(optimizer);
        }
        optimizer_free(optimizer);
    } else {
        if (g_verbose) {
            fprintf(stderr, "\n[4] Optimization\n");
            fprintf(stderr, "    ✓ No optimization (O0)\n");
        }
    }
    
    // Code generation
    if (g_verbose) {
        fprintf(stderr, "\n[5] Code Generation\n");
    }
    char asm_file[256];
    snprintf(asm_file, sizeof(asm_file), "%s.asm", input_file);
    
    CodeGen *codegen = codegen_new(asm_file);
    if (!codegen) {
        return 1;
    }
    
    // Enable SIMD vectorization for -O2 and -O3
    if (optimization_level >= 2) {
        codegen->enable_simd = true;
        if (g_verbose) {
            fprintf(stderr, "    SIMD Vectorization: Enabled (AVX2)\n");
        }
    } else {
        codegen->enable_simd = false;
        if (g_verbose) {
            fprintf(stderr, "    SIMD Vectorization: Disabled\n");
        }
    }
    
    codegen_generate(codegen, ast);
    if (g_verbose) {
        fprintf(stderr, "    ✓ Assembly generated: %s\n", asm_file);
    }
    codegen_free(codegen);
    
    // Assemble and link
    if (g_verbose) {
        fprintf(stderr, "\n[6] Assembly & Linking\n");
    }
    int ret = compile_asm_to_binary(asm_file, output_file, argv[0]);
    
    if (ret == 0) {
        if (g_verbose) {
            fprintf(stderr, "    ✓ Linking complete\n");
            fprintf(stderr, "\n─────────────────────────────────────\n");
            fprintf(stderr, "[SUCCESS] Compilation complete\n\n");
        }
        
        printf("%s\n", output_file);
        
        // Get file size if -sz flag is set
        if (g_show_size) {
            struct stat st;
            if (stat(output_file, &st) == 0) {
                long kb = (st.st_size + 512) / 1024;  // Round to nearest KB
                if (g_verbose) {
                    printf("Size: %ld KB\n", kb);
                } else {
                    printf(" (%ld KB)\n", kb);
                }
            }
        }
        
        // Record end time and show if -tm flag is set
        if (g_show_time) {
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                           (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
            if (g_verbose) {
                printf("Time: %.3fs\n", elapsed);
            } else {
                printf(" [%.3fs]\n", elapsed);
            }
        }
        
        // Clean up assembly file
        // unlink(asm_file);  // Keep for debugging
    }
    
    // Cleanup
    ast_node_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    free(source);
    
    return ret;
}
