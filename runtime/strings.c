// RasCode String Manipulation Library
// PHP-inspired easy string handling adapted to RasCode

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>

// Split string by delimiter - returns pointer to allocated array of strings
// Returns pointer to malloc'd array of string pointers (NULL-terminated)
// Caller responsible for freeing: free() the returned pointer when done
// Usage: @split[str, delim] -> pointer to string array
long sc_split(const char *str, const char *delim) {
    if (!str || !delim) return 0;
    if (*delim == '\0') return 0;  // Empty delimiter not allowed
    
    // First pass: count delimiters to determine array size
    int count = 1;
    const char *p = str;
    while (*p) {
        if (*p == *delim) count++;
        p++;
    }
    
    // Make a copy of the input string since we'll be modifying it with strtok
    char *str_copy = (char *)malloc(strlen(str) + 1);
    if (!str_copy) return 0;
    strcpy(str_copy, str);
    
    // Allocate array of string pointers (count + 1 for NULL terminator)
    char **result = (char **)malloc((count + 1) * sizeof(char *));
    if (!result) {
        free(str_copy);
        return 0;
    }
    
    // Split using strtok and populate array
    int index = 0;
    char *token = strtok(str_copy, delim);
    
    while (token != NULL && index < count) {
        // Allocate memory for each token and copy it
        char *token_copy = (char *)malloc(strlen(token) + 1);
        if (!token_copy) {
            // Failed allocation - free what we have and return
            for (int i = 0; i < index; i++) {
                free(result[i]);
            }
            free(result);
            free(str_copy);
            return 0;
        }
        strcpy(token_copy, token);
        result[index++] = token_copy;
        token = strtok(NULL, delim);
    }
    
    // NULL-terminate the array
    result[index] = NULL;
    
    // Free the working copy
    free(str_copy);
    
    // Return as pointer to array (caller must free this and all strings)
    return (long)result;
}

// Join array into string with delimiter
// Input: arr_ptr = pointer to array of string pointers (see sc_split return format)
// Returns: pointer to newly allocated joined string
// Usage: @join[arr, delim] -> pointer to joined_string
long sc_join(long arr_ptr, const char *delim) {
    if (!arr_ptr || !delim) return 0;
    
    char **arr = (char **)arr_ptr;
    if (!arr[0]) return 0;  // Empty array
    
    // First pass: calculate total length needed
    size_t total_len = 0;
    size_t delim_len = strlen(delim);
    
    for (int i = 0; arr[i] != NULL; i++) {
        total_len += strlen(arr[i]);
        if (arr[i + 1] != NULL) {  // Not the last element
            total_len += delim_len;
        }
    }
    
    // Allocate result string
    char *result = (char *)malloc(total_len + 1);
    if (!result) return 0;
    
    // Second pass: concatenate strings with delimiters
    char *dst = result;
    for (int i = 0; arr[i] != NULL; i++) {
        char *src = arr[i];
        while (*src) {
            *dst++ = *src++;
        }
        
        if (arr[i + 1] != NULL) {  // Not the last element
            strcpy(dst, delim);
            dst += delim_len;
        }
    }
    
    *dst = '\0';  // Null terminate
    
    return (long)result;
}

// Trim whitespace from string
// Usage: @trim[str] -> trimmed_string
long sc_trim(const char *str) {
    if (!str) return 0;
    
    const char *start = str;
    const char *end = str + strlen(str) - 1;
    
    while (*start && isspace(*start)) start++;
    while (end >= start && isspace(*end)) end--;
    
    long len = end - start + 1;
    if (len <= 0) return 0;
    
    // Return pointer to trimmed region
    return (long)start;
}

// Convert string to uppercase
// Usage: @upper[str] -> uppercase_string
long sc_upper(const char *str) {
    if (!str) return 0;
    
    char *result = (char *)malloc(strlen(str) + 1);
    if (!result) return 0;
    
    for (int i = 0; str[i]; i++) {
        result[i] = toupper(str[i]);
    }
    result[strlen(str)] = '\0';
    
    return (long)result;
}

// Convert string to lowercase
// Usage: @lower[str] -> lowercase_string
long sc_lower(const char *str) {
    if (!str) return 0;
    
    char *result = (char *)malloc(strlen(str) + 1);
    if (!result) return 0;
    
    for (int i = 0; str[i]; i++) {
        result[i] = tolower(str[i]);
    }
    result[strlen(str)] = '\0';
    
    return (long)result;
}

// Find substring position
// Usage: @index[str, substr] -> position (0-based, -1 if not found)
long sc_index(const char *str, const char *substr) {
    if (!str || !substr) return -1;
    
    const char *pos = strstr(str, substr);
    if (!pos) return -1;
    
    return pos - str;
}

// Replace all occurrences in string
// Usage: @replace[str, old, new] -> new_string
long sc_replace(const char *str, const char *old, const char *new) {
    if (!str || !old || !new) return 0;
    
    int count = 0;
    const char *p = str;
    while ((p = strstr(p, old))) {
        count++;
        p += strlen(old);
    }
    
    if (count == 0) return (long)str;
    
    int old_len = strlen(old);
    int new_len = strlen(new);
    
    // SECURITY: Check for integer overflow in size calculation
    // Prevent: result_len = strlen(str) + count * (new_len - old_len) + 1
    int str_len = strlen(str);
    int size_diff = new_len - old_len;
    
    // Check if multiplication would overflow
    if (count > 0 && size_diff > 0 && count > (INT_MAX - str_len - 1) / size_diff) {
        return 0;  // Overflow detected
    }
    
    int result_len = str_len + count * size_diff + 1;
    
    // Additional sanity check (max 100MB result)
    if (result_len < 0 || result_len > 100 * 1024 * 1024) {
        return 0;
    }
    
    char *result = (char *)malloc(result_len);
    if (!result) return 0;
    
    char *dst = result;
    p = str;
    const char *next;
    
    while ((next = strstr(p, old))) {
        int len = next - p;
        // SECURITY: Bounds check before memcpy
        if (dst + len > result + result_len) {
            free(result);
            return 0;
        }
        memcpy(dst, p, len);
        dst += len;
        
        if (dst + new_len > result + result_len) {
            free(result);
            return 0;
        }
        memcpy(dst, new, new_len);
        dst += new_len;
        p = next + old_len;
    }
    
    // SECURITY: Use strncpy instead of unsafe strcpy
    size_t remaining = result_len - (dst - result);
    if (remaining > 0) {
        strncpy(dst, p, remaining - 1);
        dst[remaining - 1] = '\0';
    }
    
    return (long)result;
}

// Check if string starts with prefix
// Usage: @startswith[str, prefix] -> 1 (true) or 0 (false)
long sc_startswith(const char *str, const char *prefix) {
    if (!str || !prefix) return 0;
    
    int prefix_len = strlen(prefix);
    int str_len = strlen(str);
    
    if (prefix_len > str_len) return 0;
    return strncmp(str, prefix, prefix_len) == 0 ? 1 : 0;
}

// Check if string ends with suffix
// Usage: @endswith[str, suffix] -> 1 (true) or 0 (false)
long sc_endswith(const char *str, const char *suffix) {
    if (!str || !suffix) return 0;
    
    int suffix_len = strlen(suffix);
    int str_len = strlen(str);
    
    if (suffix_len > str_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0 ? 1 : 0;
}

// Reverse string in place
// Usage: @reverse[str] -> reversed_string
long sc_reverse(const char *str) {
    if (!str) return 0;
    
    int len = strlen(str);
    char *result = (char *)malloc(len + 1);
    if (!result) return 0;
    
    for (int i = 0; i < len; i++) {
        result[i] = str[len - 1 - i];
    }
    result[len] = '\0';
    
    return (long)result;
}

// Repeat string N times
// Usage: @repeat[str, count] -> repeated_string
long sc_repeat(const char *str, long count) {
    if (!str || count <= 0) return 0;
    
    int str_len = strlen(str);
    int result_len = str_len * count + 1;
    
    char *result = (char *)malloc(result_len);
    if (!result) return 0;
    
    char *dst = result;
    for (long i = 0; i < count; i++) {
        memcpy(dst, str, str_len);
        dst += str_len;
    }
    *dst = '\0';
    
    return (long)result;
}

// Pad string to length with character
// Usage: @pad[str, length, pad_char] -> padded_string
long sc_pad(const char *str, long length, char pad_char) {
    if (!str || length <= 0) return 0;
    
    // SECURITY: Validate length to prevent integer overflow
    if (length < 0 || length > 100 * 1024 * 1024) return 0;  // Max 100MB
    
    int str_len = strlen(str);
    if (str_len >= length) return (long)str;
    
    char *result = (char *)malloc(length + 1);
    if (!result) return 0;
    
    // SECURITY: Use strncpy instead of strcpy
    strncpy(result, str, length);
    for (long i = str_len; i < length; i++) {
        result[i] = pad_char;
    }
    result[length] = '\0';
    
    return (long)result;
}

/**
 * @type[value] - Get the type of a value as a string
 * Note: Due to static typing in RasCode, this returns a generic type
 * In a fully dynamic language, we'd use runtime type tagging
 */
long sc_type(long value) {
    // Since RasCode is statically typed, we can't determine the true runtime type
    // This is a placeholder that returns "int" for most values
    // In production, this would use runtime type metadata from the value
    
    static const char *type_str = "int";
    return (long)type_str;
}
