#pragma once

/*
 * Opis rozwiązania:
 * W każdym wierzchołku drzewa trzymam sztuczną czytelnię.
 * Operacje tree_list mogą wykonywać się współbieżnie, ponieważ wykonują
 * jedynie operacje hmap_get, która może być wykonywana współbieżnie z
 * innymi operacjami hmap_get. Dlatego uznaję w rozwiązaniu, że tree_list wpuszcza,
 * do każdej biblioteki na scieżce po jednym czytelniku. Uznaję,
 * że hmap_get to operacja czytania. Kiedy skończy się list, wypuszczam, idąc w górę
 * drzewa wpuszczonych wcześniej czytelników. Każdą z operacji hmap_insert,
 * hmap_remove, hmap_free uznaję za pisanie.
 * Kiedy wykonywana jest jedna z operacji tree_move lub tree_remove musimy dostać
 * się do wierzchołka, który będziemy modyfikować, w tym celu wpuszczam po jednym
 * czytelniku do każdego elementu ścieżki, poza rodzicem tworzonego/usuwanego wierzchołka,
 * tam wpuszczam pisarza, ponieważ na tym wierzchołku, będzie wykonywane pisanie.
 * Oczywiście wpuszczanie pisarza lub czytelnika nie musi odbyć się natychmiast, ale
 * czekamy, aż będzie to możliwe. Po zakończeniu operacji wypuszczam pisarza, a potem
 * idąc w górę drzewa czytelników. Ostatnia operacja, czyli tree_move, naraz potrzebuje
 * dostępu do rodzica target i source. W tym celu postąpimy tak jak poprzednio, ale
 * pisarza wpuszczamy do najgłębszego wspólnego przodka target i source.
 * Zauważmy, że pisarz ten zaczeka na skończenie wszystkich operacji pisania w obu
 * poddrzewach, bo jeśli ktoś tam pisze, to w lca jest jakiś czytelnik.
 * Kiedy już wejdzie, nikt nie będzie mógł zmieniać tych poddrzew.
 * Analogicznie po operacji move wypuszczamy pisarza z lca i czytelników powyżej.
 */

typedef struct Tree Tree; // Let "Tree" mean the same as "struct Tree".

Tree* tree_new();

void tree_free(Tree*);

char* tree_list(Tree* tree, const char* path);

int tree_create(Tree* tree, const char* path);

int tree_remove(Tree* tree, const char* path);

int tree_move(Tree* tree, const char* source, const char* target);
