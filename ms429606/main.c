#include "HashMap.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "path_utils.h"
#include "Tree.h"

void print_map(HashMap* map) {
    const char* key = NULL;
    void* value = NULL;
    printf("Size=%zd\n", hmap_size(map));
    HashMapIterator it = hmap_iterator(map);
    while (hmap_next(map, &it, &key, &value)) {
        printf("Key=%s Value=%p\n", key, value);
    }
    printf("\n");
}

static char* path_to_lca(const char* source, const char* target) {
    size_t source_folders_count = 0;
    size_t target_folders_count = 0;
    char** source_folders = make_path_folders_array(source, &source_folders_count);
    char** target_folders = make_path_folders_array(target, &target_folders_count);

    size_t i = 0;
    char* result = malloc ((MAX_PATH_LENGTH + 1) * sizeof (char));
    strcpy(result, "/");
    while (source_folders && target_folders && source_folders[i] &&target_folders[i] &&
          (strcmp(source_folders[i], target_folders[i])) == 0) {
        strcat(result, source_folders[i]);
        strcat(result, "/");
        i++;
    }

    free_array_of_strings(source_folders, source_folders_count);
    free_array_of_strings(target_folders, target_folders_count);
    return result;
}

int main(void) {
    /*HashMap* map = hmap_new();
    hmap_insert(map, "a", hmap_new());
    print_map(map);

    HashMap* child = (HashMap*)hmap_get(map, "a");
    hmap_free(child);
    hmap_remove(map, "a");
    print_map(map);

    hmap_free(map);*/
    /*Tree *tree = tree_new();
    char *list_content = tree_list(tree, "/");
    assert(strcmp(list_content, "") == 0);
    free(list_content);
    assert(tree_list(tree, "/a/") == NULL);
    assert(tree_create(tree, "/a/") == 0);
    assert(tree_create(tree, "/a/b/") == 0);
    assert(tree_create(tree, "/a/b/") == EEXIST);
    assert(tree_create(tree, "/a/b/c/d/") == ENOENT);
    assert(tree_remove(tree, "/a/") == ENOTEMPTY);
    assert(tree_create(tree, "/b/") == 0);
    assert(tree_create(tree, "/a/c/") == 0);
    assert(tree_create(tree, "/a/c/d/") == 0);
    assert(tree_move(tree, "/a/c/", "/b/c/") == 0);
    assert(tree_remove(tree, "/b/c/d/") == 0);
    list_content = tree_list(tree, "/b/");
    assert(strcmp(list_content, "c") == 0);
    free(list_content);
    tree_free(tree);*/

//    size_t size;
    const char* so = "/a/b/c/";
    const char* ta = "/ffas/";
    char* a = path_to_lca(so, ta);
    printf("%s", a);
    free(a);

    return 0;
}