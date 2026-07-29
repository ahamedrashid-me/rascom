/*
 * resources.h - rascom Branding and Resource Management
 */

#ifndef RASCOM_RESOURCES_H
#define RASCOM_RESOURCES_H

#include <stddef.h>

/**
 * Get embedded logo data (RasCom.jpg)
 * @param out_size Pointer to receive logo size in bytes
 * @return Pointer to logo image data, or NULL if not embedded
 */
const unsigned char *get_rascom_logo(size_t *out_size);

/**
 * Display rascom branding and compiler information
 */
void display_rascom_branding(void);

/**
 * Extract embedded logo to file for external display
 * @param output_path File path to write logo to
 * @return 1 if successful, 0 if failed
 */
int extract_logo_to_file(const char *output_path);

#endif /* RASCOM_RESOURCES_H */
