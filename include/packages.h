#ifndef PACKAGES_H
#define PACKAGES_H

#include "common.h"
#include "ast.h"

// Package search paths
#define PKG_MAX_PATHS 10

// Package information
typedef struct {
    char *name;           // Package name
    char *file_path;      // Full path to package file
    bool is_compiled;     // Is it a .raslib or .ras file?
    ASTNode *ast;         // Parsed AST (if .ras file)
} Package;

// Package manager
typedef struct {
    char *search_paths[PKG_MAX_PATHS];
    int num_paths;
    Package **loaded_packages;
    int num_loaded;
    int capacity;
} PackageManager;

// Package manager functions
PackageManager *pkg_manager_new(void);
void pkg_manager_free(PackageManager *mgr);
void pkg_manager_add_search_path(PackageManager *mgr, const char *path);
Package *pkg_manager_load(PackageManager *mgr, const char *package_name);
Package *pkg_manager_find_loaded(PackageManager *mgr, const char *package_name);
char *pkg_find_file(PackageManager *mgr, const char *package_name);

// Package functions
Package *package_new(const char *name, const char *file_path, bool is_compiled);
void package_free(Package *pkg);

#endif // PACKAGES_H
