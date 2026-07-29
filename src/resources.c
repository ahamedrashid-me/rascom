/*
 * resources.c - rascom Branding and Resource Management
 * 
 * Provides access to embedded resources (logos, version info, etc)
 */

#include <stdio.h>
#include <stdlib.h>

/* Embedded logo section - created by objcopy from binary */
extern const unsigned char _binary_src_RasCom_jpg_start[];
extern const unsigned char _binary_src_RasCom_jpg_end[];

/**
 * Get embedded logo data
 * Returns pointer to logo image and its size
 */
const unsigned char *get_rascom_logo(size_t *out_size) {
    if (!out_size) return NULL;
    
    /* Calculate size from section markers */
    *out_size = (size_t)(_binary_src_RasCom_jpg_end - _binary_src_RasCom_jpg_start);
    
    if (*out_size == 0) {
        /* Logo not embedded, return NULL */
        return NULL;
    }
    
    return _binary_src_RasCom_jpg_start;
}

/**
 * Display rascom branding information
 */
void display_rascom_branding(void) {
    fprintf(stderr, "\n");
    fprintf(stderr, "╔═══════════════════════════════════════════════╗\n");
    fprintf(stderr, "║             rascom (RasCode Compiler)          ║\n");
    fprintf(stderr, "║     Compiling RasCode into x86-64 Assembly    ║\n");
    fprintf(stderr, "╚═══════════════════════════════════════════════╝\n");
    fprintf(stderr, "\n");
}

/**
 * Extract embedded logo to file (for testing/display)
 */
int extract_logo_to_file(const char *output_path) {
    if (!output_path) return 0;
    
    size_t logo_size = 0;
    const unsigned char *logo_data = get_rascom_logo(&logo_size);
    
    if (!logo_data || logo_size == 0) {
        fprintf(stderr, "Warning: Logo not embedded in executable\n");
        return 0;
    }
    
    FILE *f = fopen(output_path, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot create file %s\n", output_path);
        return 0;
    }
    
    size_t written = fwrite(logo_data, 1, logo_size, f);
    fclose(f);
    
    return written == logo_size;
}
