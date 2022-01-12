#include "path_utils.h"
#include "err.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_path_valid(const char* path)
{
    size_t len = strlen(path);
    if (len == 0 || len > MAX_PATH_LENGTH)
        return false;
    if (path[0] != '/' || path[len - 1] != '/')
        return false;
    const char* name_start = path + 1; // Start of current path component, just after '/'.
    while (name_start < path + len) {
        char* name_end = strchr(name_start, '/'); // End of current path component, at '/'.
        if (!name_end || name_end == name_start || name_end > name_start + MAX_FOLDER_NAME_LENGTH)
            return false;
        for (const char* p = name_start; p != name_end; ++p)
            if (*p < 'a' || *p > 'z')
                return false;
        name_start = name_end + 1;
    }
    return true;
}

const char* split_path(const char* path, char* component)
{
    const char* subpath = strchr(path + 1, '/'); // Pointer to second '/' character.
    if (!subpath) // Path is "/".
        return NULL;
    if (component) {
        int len = subpath - (path + 1);
        assert(len >= 1 && len <= MAX_FOLDER_NAME_LENGTH);
        strncpy(component, path + 1, len);
        component[len] = '\0';
    }
    return subpath;
}

char* make_path_to_parent(const char* path, char* component)
{
    size_t len = strlen(path);
    if (len == 1) // Path is "/".
        return NULL;
    const char* p = path + len - 2; // Point before final '/' character.
    // Move p to last-but-one '/' character.
    while (*p != '/')
        p--;

    size_t subpath_len = p - path + 1; // Include '/' at p.
    char* result = malloc(subpath_len + 1); // Include terminating null character.
    CHECK(result);
    strncpy(result, path, subpath_len);
    result[subpath_len] = '\0';

    if (component) {
        size_t component_len = len - subpath_len - 1; // Skip final '/' as well.
        assert(component_len >= 1 && component_len <= MAX_FOLDER_NAME_LENGTH);
        strncpy(component, p + 1, component_len);
        component[component_len] = '\0';
    }

    return result;
}

// A wrapper for using strcmp in qsort.
// The arguments here are actually pointers to (const char*).
static int compare_string_pointers(const void* p1, const void* p2)
{
    return strcmp(*(const char**)p1, *(const char**)p2);
}

const char** make_map_contents_array(HashMap* map)
{
    size_t n_keys = hmap_size(map);
    const char** result = calloc(n_keys + 1, sizeof(char*));
    HashMapIterator it = hmap_iterator(map);
    const char** key = result;
    void* value = NULL;
    while (hmap_next(map, &it, key, &value)) {
        key++;
    }
    *key = NULL; // Set last array element to NULL.
    qsort(result, n_keys, sizeof(char*), compare_string_pointers);
    return result;
}

char* make_map_contents_string(HashMap* map)
{
    const char** keys = make_map_contents_array(map);

    unsigned int result_size = 0; // Including ending null character.
    for (const char** key = keys; *key; ++key)
        result_size += strlen(*key) + 1;

    // Return empty string if map is empty.
    if (!result_size) {
        // Note we can't just return "", as it can't be free'd.
        char* result = malloc(1);
        CHECK(result);
        free(keys);
        *result = '\0';
        return result;
    }

    char* result = malloc(result_size);
    CHECK(result);
    char* position = result;
    for (const char** key = keys; *key; ++key) {
        size_t keylen = strlen(*key);
        assert(position + keylen <= result + result_size);
        strcpy(position, *key); // NOLINT: array size already checked.
        position += keylen;
        *position = ',';
        position++;
    }
    position--;
    *position = '\0';
    free(keys);
    return result;
}

/* Author of functions below: Mikołaj Szkaradek */

char** make_path_folders_array(const char* path, size_t *size) {
    size_t path_folders_count = 0;
    if (strlen(path) == 1) return NULL;
    const char* path_ptr = path + 1; // We skip first '/'.
    while (*path_ptr != '\0') {
        if (*path_ptr == '/') path_folders_count++;
        path_ptr++;
    }
    *size = path_folders_count;

    char component[MAX_FOLDER_NAME_LENGTH + 1];
    const char* subpath = path;
    char** path_folders = malloc((path_folders_count + 1) * sizeof(char*));
    CHECK(path_folders);

    int i = 0;
    while ((subpath = split_path(subpath, component))) {
        path_folders[i] = malloc((MAX_FOLDER_NAME_LENGTH + 1) * sizeof (char));
        CHECK(path_folders[i]);
        strcpy(path_folders[i], component);
        i++;
    }

    return path_folders;
}

void free_array_of_strings(char** arr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (arr[i]) free(arr[i]);
    }
    if (arr) free(arr);
}

bool moving_to_subtree(const char* source, const char* target) {
    if (strcmp(source, target) == 0) return 0;
    size_t s_len = strlen(source);
    char* target_helper = malloc(s_len + 1);
    CHECK(target_helper);
    strncpy(target_helper, target, s_len);
    target_helper[s_len] = '\0';
    bool retval = 0;
    if (strcmp(source, target_helper) == 0) retval = 1;
    free (target_helper);
    return retval;
}

static size_t min(size_t a, size_t b) {
    if (a <= b) return a;
    return b;
}

char* path_to_lca(const char* source, const char* target) {
    size_t source_folders_count = 0;
    size_t target_folders_count = 0;
    char** source_folders = make_path_folders_array(source, &source_folders_count);
    char** target_folders = make_path_folders_array(target, &target_folders_count);

    size_t i = 0;
    size_t size = min(source_folders_count, target_folders_count);
    char* result = malloc ((MAX_PATH_LENGTH + 1) * sizeof (char));
    CHECK(result);
    strcpy(result, "/");
    while (i < size && (strcmp(source_folders[i], target_folders[i])) == 0) {
        strcat(result, source_folders[i]);
        strcat(result, "/");
        i++;
    }

    free_array_of_strings(source_folders, source_folders_count);
    free_array_of_strings(target_folders, target_folders_count);
    return result;
}
