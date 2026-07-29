#include "../include/packages.h"
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

/* SECURITY: Maximum path length */
#define MAX_PACKAGE_PATH 4096

static bool is_valid_package_name(const char *name) {
    if (!name || *name == '\0') return false;

    /* Allow any file with extension or path */
    if (strchr(name, '.') != NULL || strchr(name, '/') != NULL) {
        return true;
    }

    /* Standard package name validation */
    if (strstr(name, "..") != NULL || name[0] == '/') return false;

    for (int i = 0; name[i]; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_' && name[i] != '-') {
            return false;
        }
    }
    return true;
}

bool file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

PackageManager *pkg_manager_new(void) {
    PackageManager *mgr = xmalloc(sizeof(PackageManager));
    mgr->num_paths = 0;
    mgr->loaded_packages = NULL;
    mgr->num_loaded = 0;
    mgr->capacity = 0;

    /* Project-local first */
    pkg_manager_add_search_path(mgr, ".");
    pkg_manager_add_search_path(mgr, "./lib");
    pkg_manager_add_search_path(mgr, "./packages");

    char *home = getenv("HOME");
    if (home) {
        char path[512];
        snprintf(path, sizeof(path), "%s/.rascom/packages", home);
        pkg_manager_add_search_path(mgr, path);
        snprintf(path, sizeof(path), "%s/.rascode/packages", home);
        pkg_manager_add_search_path(mgr, path);
    }

    /* System package roots — RasBox installs .rcp packages here */
    pkg_manager_add_search_path(mgr, "/usr/lib/rascom/packages");
    pkg_manager_add_search_path(mgr, "/usr/local/lib/rascom/packages");

    return mgr;
}

void pkg_manager_free(PackageManager *mgr) {
    if (!mgr) return;
    for (int i = 0; i < mgr->num_paths; i++) {
        free(mgr->search_paths[i]);
    }
    for (int i = 0; i < mgr->num_loaded; i++) {
        package_free(mgr->loaded_packages[i]);
    }
    free(mgr->loaded_packages);
    free(mgr);
}

void pkg_manager_add_search_path(PackageManager *mgr, const char *path) {
    if (mgr->num_paths >= PKG_MAX_PATHS) {
        fprintf(stderr, "Warning: Maximum package search paths reached\n");
        return;
    }
    mgr->search_paths[mgr->num_paths++] = xstrdup(path);
}

char *pkg_find_file(PackageManager *mgr, const char *package_name) {
    if (!is_valid_package_name(package_name)) {
        fprintf(stderr, "Error: Invalid package name '%s'.\n", package_name);
        return NULL;
    }

    /* Direct file path support */
    if (strstr(package_name, ".rco") != NULL || strstr(package_name, ".ras") != NULL ||
        strstr(package_name, ".rcp") != NULL || strstr(package_name, ".rclib") != NULL ||
        strchr(package_name, '/') != NULL) {
        if (file_exists(package_name)) {
            return xstrdup(package_name);
        }
    }

    char *path = xmalloc(MAX_PACKAGE_PATH);

    for (int i = 0; i < mgr->num_paths; i++) {
        /* .rcp — RasCode Package (distributable) */
        snprintf(path, MAX_PACKAGE_PATH, "%s/%s.rcp", mgr->search_paths[i], package_name);
        if (file_exists(path)) {
            char *result = xstrdup(path);
            free(path);
            return result;
        }

        /* Package directory forms */
        snprintf(path, MAX_PACKAGE_PATH, "%s/%s/package.rcp", mgr->search_paths[i], package_name);
        if (file_exists(path)) {
            char *result = xstrdup(path);
            free(path);
            return result;
        }
        snprintf(path, MAX_PACKAGE_PATH, "%s/%s/main.rco", mgr->search_paths[i], package_name);
        if (file_exists(path)) {
            char *result = xstrdup(path);
            free(path);
            return result;
        }

        /* Source */
        snprintf(path, MAX_PACKAGE_PATH, "%s/%s.rco", mgr->search_paths[i], package_name);
        if (file_exists(path)) {
            char *result = xstrdup(path);
            free(path);
            return result;
        }

        /* Legacy */
        snprintf(path, MAX_PACKAGE_PATH, "%s/%s.ras", mgr->search_paths[i], package_name);
        if (file_exists(path)) {
            char *result = xstrdup(path);
            free(path);
            return result;
        }
        snprintf(path, MAX_PACKAGE_PATH, "%s/%s.rclib", mgr->search_paths[i], package_name);
        if (file_exists(path)) {
            char *result = xstrdup(path);
            free(path);
            return result;
        }
    }

    free(path);
    return NULL;
}

Package *pkg_manager_find_loaded(PackageManager *mgr, const char *package_name) {
    for (int i = 0; i < mgr->num_loaded; i++) {
        if (strcmp(mgr->loaded_packages[i]->name, package_name) == 0) {
            return mgr->loaded_packages[i];
        }
    }
    return NULL;
}

Package *pkg_manager_load(PackageManager *mgr, const char *package_name) {
    Package *existing = pkg_manager_find_loaded(mgr, package_name);
    if (existing) return existing;

    char *file_path = pkg_find_file(mgr, package_name);
    if (!file_path) {
        fprintf(stderr, "Error: Package '%s' not found\n", package_name);
        return NULL;
    }

    /* Prevent accidental self-import of the main translation unit */
    if (strstr(file_path, "tyr.rco") != NULL || strstr(package_name, "tyr") != NULL) {
        fprintf(stderr, "Warning: Skipping self-import of main file '%s'\n", package_name);
        free(file_path);
        return NULL;
    }

    bool is_compiled = (strstr(file_path, ".rcp") != NULL ||
                        strstr(file_path, ".rclib") != NULL);
    Package *pkg = package_new(package_name, file_path, is_compiled);
    free(file_path);

    if (mgr->num_loaded >= mgr->capacity) {
        mgr->capacity = mgr->capacity == 0 ? 8 : mgr->capacity * 2;
        mgr->loaded_packages = xrealloc(mgr->loaded_packages, sizeof(Package *) * mgr->capacity);
    }
    mgr->loaded_packages[mgr->num_loaded++] = pkg;
    return pkg;
}

Package *package_new(const char *name, const char *file_path, bool is_compiled) {
    Package *pkg = xmalloc(sizeof(Package));
    pkg->name = xstrdup(name);
    pkg->file_path = xstrdup(file_path);
    pkg->is_compiled = is_compiled;
    pkg->ast = NULL;
    return pkg;
}

void package_free(Package *pkg) {
    if (!pkg) return;
    free(pkg->name);
    free(pkg->file_path);
    ast_node_free(pkg->ast);
    free(pkg);
}
