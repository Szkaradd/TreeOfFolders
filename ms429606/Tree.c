#include <errno.h>
#include "path_utils.h"
#include "HashMap.h"
#include "Tree.h"
#include "readers-writers-template.h"
#include "err.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h> //TODO usunac to xd. I zrobic checkPTR.

#define NEW_ERROR -1

typedef struct Tree {
    struct readwrite* library; // Each node has its own library.
    HashMap* subTrees;
} Tree;

static size_t min(size_t a, size_t b) {
    if (a <= b) return a;
    return b;
}

// Return string containing a path to last common ancestor of source and target.
// We assume that both paths are valid.
static char* path_to_lca(const char* source, const char* target) {
    size_t source_folders_count = 0;
    size_t target_folders_count = 0;
    char** source_folders = make_path_folders_array(source, &source_folders_count);
    char** target_folders = make_path_folders_array(target, &target_folders_count);

    size_t i = 0;
    size_t size = min(source_folders_count, target_folders_count);
    char* result = malloc ((MAX_PATH_LENGTH + 1) * sizeof (char));
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

// Function places one reader in each library on the path.
// If writing is true function places writer instead of reader
// in the last folder of the path.
// If path doesn't exist it will stop at the end of existing part.
static void lock_path(Tree* tree, const char* path, bool writing) {
    if (!tree) return;
    char component[MAX_FOLDER_NAME_LENGTH + 1];
    const char* subpath = path;

    Tree *curr_tree = tree;
    HashMap* next = tree->subTrees;
    while ((subpath = split_path(subpath, component))) {
        start_reading(curr_tree->library);
        curr_tree = (Tree*)hmap_get(next, component);
        if (!curr_tree) return;
        next = curr_tree->subTrees;
    }

    if (writing) start_writing(curr_tree->library);
    else start_reading(curr_tree->library);
}

// Function removes one reader in each library on the path.
// If writing is true function removes writer instead of reader
// from the last folder of the path.
// If path doesn't exist it will stop at the end of existing part.
static void unlock_path(Tree* tree, const char* path, bool writing) {
    if (!tree) return;
    char component[MAX_FOLDER_NAME_LENGTH + 1];
    const char* subpath = path;

    Tree* curr_tree = tree;
    //Tree* prev_tree = tree;
    HashMap* next = tree->subTrees;
    while ((subpath = split_path(subpath, component))) {
        finish_reading(curr_tree->library);
        curr_tree = (Tree*)hmap_get(next, component);
        //prev_tree = curr_tree;
        if (!curr_tree) return;
        next = curr_tree->subTrees;
    }

    //finish_reading(prev_tree->library);

    if (writing) finish_writing(curr_tree->library);
    else finish_reading(curr_tree->library);
}

// We assume that the path is valid and exists.
static HashMap* get_folder_contents(Tree* tree, const char* path) {
    if (!tree) return NULL;
    char component[MAX_FOLDER_NAME_LENGTH + 1];
    const char* subpath = path;
    Tree* curr_tree;
    HashMap* next = tree->subTrees;
    while ((subpath = split_path(subpath, component))) {
        curr_tree = (Tree*)hmap_get(next, component);
        if (!curr_tree) return NULL;
        next = curr_tree->subTrees;
    }

    return next;
}

// Function checks if source is a prefix of target.
// Returns false if paths are equal.
static bool moving_to_subtree(const char* source, const char* target) {
    if (strcmp(source, target) == 0) return 0;
    size_t s_len = strlen(source);
    char* target_helper = malloc(s_len + 1);
    strncpy(target_helper, target, s_len);
    target_helper[s_len] = '\0';
    bool retval = 0;
    if (strcmp(source, target_helper) == 0) retval = 1;
    free (target_helper);
    return retval;
}

Tree* tree_new() {
    Tree* tree = malloc(sizeof(Tree));
    assert(tree);
    tree->subTrees = hmap_new();
    tree->library = malloc(sizeof(struct readwrite));
    init(tree->library);
    return tree;
}

void tree_free(Tree* tree) {
    const char* key;
    void* value;
    HashMapIterator it = hmap_iterator(tree->subTrees);

    while (hmap_next(tree->subTrees, &it, &key, &value)) {
        tree_free((Tree *) value);
    }
    destroy(tree->library);
    free(tree->library);
    hmap_free(tree->subTrees);

    free(tree);
}

char* tree_list(Tree* tree, const char* path) {
    if (!is_path_valid(path)) return NULL;
    lock_path(tree, path, false);

    HashMap* folder = get_folder_contents(tree, path);
    if (!folder) {
        unlock_path(tree, path, false);
        return NULL;
    }
    char* res = make_map_contents_string(folder);

    unlock_path(tree, path, false);
    return res;
}

int tree_create(Tree* tree, const char* path) {
    if (!is_path_valid(path)) return EINVAL;
    if (strlen(path) == 1) return EEXIST;

    char to_insert[MAX_FOLDER_NAME_LENGTH + 1];
    char* subpath = make_path_to_parent(path, to_insert);
    lock_path(tree, subpath, true);

    HashMap* folder_parent = get_folder_contents(tree, subpath);

    if (!folder_parent) {
        unlock_path(tree, subpath, true);
        free(subpath);
        return ENOENT;
    }
    if (hmap_get(folder_parent, to_insert)) {
        unlock_path(tree, subpath, true);
        free(subpath);
        return EEXIST;
    }

    Tree* new_tree = tree_new();
    hmap_insert(folder_parent, to_insert, new_tree);
    unlock_path(tree, subpath, true);
    free(subpath);

    return 0;
}

int tree_remove(Tree* tree, const char* path) {
    if (!is_path_valid(path)) return EINVAL;
    if (strlen(path) == 1) return EBUSY;
    //if (!path_exists(tree, path)) return ENOENT;
    //if (!is_folder_empty(tree, path)) return ENOTEMPTY;
    
    char component[MAX_FOLDER_NAME_LENGTH + 1];
    char* subpath = make_path_to_parent(path, component);
    lock_path(tree, subpath, true);
    HashMap* folder_parent = get_folder_contents(tree, subpath);

    if (!folder_parent || !hmap_get(folder_parent, component)) {
        unlock_path(tree, subpath, true);
        free(subpath);
        return ENOENT;
    }

    Tree* to_remove = (Tree*)hmap_get(folder_parent, component);
    if (hmap_size(to_remove->subTrees) != 0) {
        unlock_path(tree, subpath, true);
        free(subpath);
        return ENOTEMPTY;
    }
    tree_free(to_remove);
    hmap_remove(folder_parent, component);
    unlock_path(tree, subpath, true);
    free(subpath);

    return 0;
}

int tree_move(Tree* tree, const char* source, const char* target) {
    if (!is_path_valid(source) || !is_path_valid(target)) return EINVAL;
    if (strlen(source) == 1) return EBUSY;
    if (strlen(target) == 1) return EEXIST;
    if (moving_to_subtree(source, target)) return NEW_ERROR;

    char* lca = path_to_lca(source, target);
    lock_path(tree, lca, true);

    //if (!path_exists(tree, source) || !path_parent_exists(tree, target)) return ENOENT;
    //if (path_exists(tree, target)) return EEXIST;

    char source_name[MAX_FOLDER_NAME_LENGTH + 1];
    char* source_parent = make_path_to_parent(source, source_name);
    HashMap* source_parent_map = get_folder_contents(tree, source_parent);
    free(source_parent);

    if (!source_parent_map || !hmap_get(source_parent_map, source_name)) {
        unlock_path(tree, lca, true);
        free(lca);
        return ENOENT;
    }

    Tree* to_move = (Tree*)hmap_get(source_parent_map, source_name);

    char target_name[MAX_FOLDER_NAME_LENGTH + 1];
    char* target_parent = make_path_to_parent(target, target_name);
    HashMap* target_map = get_folder_contents(tree, target_parent);
    free(target_parent);

    if (!target_map) {
        unlock_path(tree, lca, true);
        free(lca);
        return ENOENT;
    }
    if (strcmp(source, target) == 0) {
        unlock_path(tree, lca, true);
        free(lca);
        return 0;
    }
    if (hmap_get(target_map, target_name)) {
        unlock_path(tree, lca, true);
        free(lca);
        return EEXIST;
    }

    hmap_remove(source_parent_map, source_name);
    hmap_insert(target_map, target_name, to_move);
    unlock_path(tree, lca, true);
    free(lca);

    return 0;
}
