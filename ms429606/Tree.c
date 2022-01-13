/* Author Mikołaj Szkaradek */
#include <errno.h>
#include "path_utils.h"
#include "HashMap.h"
#include "Tree.h"
#include "readers-writers-template.h"
#include "err.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define NEW_ERROR -11

// Each Tree stores a pointer to its parent, pointer to
// struct readwrite and a Hashmap of subtrees.
// Keys in the map are folder names, values are whole subtrees.
typedef struct Tree {
    Tree* parent;
    struct readwrite* library; // Each node has its own library.
    HashMap* subTrees;
} Tree;

// Pair of tree* and bool returned by let_readers_and_writer_in function.
typedef struct PairTreeBool {
    Tree* tree;
    bool writing;
} PairTB;

// Removes writer from tree->library and changes pointer to parent.
static void release_writer(Tree** tree) {
    rw_writer_final_protocol((*tree)->library);
    *tree = (*tree)->parent;
}

// Function removes one reader from each library on the path.
// If writing is true function removes writer instead of reader
// from the first folder of the path.
// It starts in the given node and goes up until it reaches
// NULL which is the parent of root.
static void release_readers_and_writer(PairTB first_to_release) {
    if (!first_to_release.tree) return;
    Tree* t = first_to_release.tree;
    if (first_to_release.writing)
        release_writer(&t);
    while (t) {
        rw_reader_final_protocol(t->library);
        t = t->parent;
    }
}

// Function places one reader in each library on the path.
// If writing is true function places writer instead of reader
// in the last folder of the path.
// If path doesn't exist it will stop at the end of existing part.
// Return last tree on the path and bool which value depends on
// successful locking of the whole path.
// It's true when there is a writer in the returned tree library.
// We assume that the path is valid.
static PairTB let_readers_and_writer_in(Tree* tree, const char* path, bool writing) {
    PairTB result;
    result.tree = NULL;
    result.writing = false;
    if (!tree) return result;
    char component[MAX_FOLDER_NAME_LENGTH + 1];
    const char* subpath = path;

    Tree *current = tree;
    while ((subpath = split_path(subpath, component))) {
        result.tree = current;
        rw_reader_preliminary_protocol(current->library);
        current = (Tree*)hmap_get(current->subTrees, component);
        if (!current) {
            release_readers_and_writer(result);
            result.tree = NULL;
            return result;
        }
    }

    result.tree = current;
    result.writing = writing;
    if (writing) rw_writer_preliminary_protocol(current->library);
    else rw_reader_preliminary_protocol(current->library);
    return result;
}

// Return a pointer to tree which represents the folder that
// the path leads to.
// We assume that the path is valid.
static Tree* find_path_subtree(Tree* tree, const char* path) {
    if (!tree) return NULL;
    Tree* current = tree;
    char component[MAX_FOLDER_NAME_LENGTH + 1];
    const char* subpath = path;

    while ((subpath = split_path(subpath, component))) {
        current = (Tree*)hmap_get(current->subTrees, component);
        if (!current) return NULL;
    }

    return current;
}

Tree* tree_new() {
    Tree* tree = malloc(sizeof(Tree));
    CHECK_PTR(tree);
    tree->parent = NULL;
    tree->library = malloc(sizeof(struct readwrite));
    CHECK_PTR(tree->library);
    rw_init(tree->library);
    tree->subTrees = hmap_new();
    return tree;
}

void tree_free(Tree* tree) {
    const char* key;
    void* value;
    HashMapIterator it = hmap_iterator(tree->subTrees);

    while (hmap_next(tree->subTrees, &it, &key, &value)) {
        tree_free((Tree*)value);
    }
    rw_destroy(tree->library);
    free(tree->library);
    hmap_free(tree->subTrees);

    free(tree);
}

char* tree_list(Tree* tree, const char* path) {
    if (!is_path_valid(path)) return NULL;
    PairTB first_to_release = let_readers_and_writer_in(tree, path, false);
    if (!first_to_release.tree) return NULL;

    Tree* folder = find_path_subtree(tree, path);

    if (!folder) {
        release_readers_and_writer(first_to_release);
        return NULL;
    }
    char* contents_string = make_map_contents_string(folder->subTrees);

    release_readers_and_writer(first_to_release);
    return contents_string;
}

int tree_create(Tree* tree, const char* path) {
    if (!is_path_valid(path)) return EINVAL;
    if (strlen(path) == 1) return EEXIST;

    char to_insert[MAX_FOLDER_NAME_LENGTH + 1];
    char* subpath = make_path_to_parent(path, to_insert);
    PairTB first_to_release = let_readers_and_writer_in(tree, subpath, true);
    if (!first_to_release.tree) {
        free(subpath);
        return ENOENT;
    }

    Tree* folder_parent = find_path_subtree(tree, subpath);
    free(subpath);

    if (!folder_parent) {
        release_readers_and_writer(first_to_release);
        return ENOENT;
    }
    if (hmap_get(folder_parent->subTrees, to_insert)) {
        release_readers_and_writer(first_to_release);
        return EEXIST;
    }

    Tree* new_tree = tree_new();
    new_tree->parent = folder_parent;
    hmap_insert(folder_parent->subTrees, to_insert, new_tree);
    release_readers_and_writer(first_to_release);

    return 0;
}

int tree_remove(Tree* tree, const char* path) {
    if (!is_path_valid(path)) return EINVAL;
    if (strlen(path) == 1) return EBUSY;

    char component[MAX_FOLDER_NAME_LENGTH + 1];
    char* subpath = make_path_to_parent(path, component);
    PairTB first_to_release = let_readers_and_writer_in(tree, subpath, true);
    if (!first_to_release.tree) {
        free(subpath);
        return ENOENT;
    }

    Tree* folder_parent = find_path_subtree(tree, subpath);
    free(subpath);

    if (!folder_parent || !hmap_get(folder_parent->subTrees, component)) {
        release_readers_and_writer(first_to_release);
        return ENOENT;
    }

    Tree* to_remove = (Tree*)hmap_get(folder_parent->subTrees, component);
    if (hmap_size(to_remove->subTrees) != 0) {
        release_readers_and_writer(first_to_release);
        return ENOTEMPTY;
    }
    tree_free(to_remove);
    hmap_remove(folder_parent->subTrees, component);
    release_readers_and_writer(first_to_release);

    return 0;
}

int tree_move(Tree* tree, const char* source, const char* target) {
    if (!is_path_valid(source) || !is_path_valid(target)) return EINVAL;
    if (strlen(source) == 1) return EBUSY;
    if (strlen(target) == 1) return EEXIST;
    if (moving_to_subtree(source, target)) return NEW_ERROR;

    char* lca = path_to_lca(source, target);
    PairTB first_to_release = let_readers_and_writer_in(tree, lca, true);
    free(lca);
    if (!first_to_release.tree) return ENOENT;

    char source_name[MAX_FOLDER_NAME_LENGTH + 1];
    char* source_parent = make_path_to_parent(source, source_name);
    Tree* source_parent_tree = find_path_subtree(tree, source_parent);
    free(source_parent);

    if (!source_parent_tree || !hmap_get(source_parent_tree->subTrees, source_name)) {
        release_readers_and_writer(first_to_release);
        return ENOENT;
    }

    Tree* to_move = (Tree*)hmap_get(source_parent_tree->subTrees, source_name);

    char target_name[MAX_FOLDER_NAME_LENGTH + 1];
    char* target_parent = make_path_to_parent(target, target_name);
    Tree* target_tree = find_path_subtree(tree, target_parent);
    free(target_parent);

    if (!target_tree) {
        release_readers_and_writer(first_to_release);
        return ENOENT;
    }
    if (strcmp(source, target) == 0) {
        release_readers_and_writer(first_to_release);
        return 0;
    }
    if (hmap_get(target_tree->subTrees, target_name)) {
        release_readers_and_writer(first_to_release);
        return EEXIST;
    }

    hmap_remove(source_parent_tree->subTrees, source_name);
    to_move->parent = target_tree;
    hmap_insert(target_tree->subTrees, target_name, to_move);
    release_readers_and_writer(first_to_release);

    return 0;
}
