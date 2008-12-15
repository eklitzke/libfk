#include "fk.h"
#include <stdio.h>

void destroy(const char *s)
{
	printf("DESTROY(\"%s\")\n", s);
}

void test_one()
{
	puts("Running test_one...");
	/* A and B are both active and depend on C, which is inactive. After
	 * deleting A, the graph should only contain B and C */

	fk_initialize((FKDestroyCallback) destroy);

	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "C");
	fk_add_relation("A", xs);
	fk_add_relation("B", xs);
	fk_delete("A");
	g_slist_free(xs);

	GHashTable *ht = fk_get_hash_table();
	g_assert(g_hash_table_size(ht) == 2);
	GList *ys = g_hash_table_get_keys(ht);
	g_assert(g_list_length(ys) == 2);

	/* The list ys should just contain B and C. If the first item is not B,
	 * reverse the list */
	if (strcmp(g_list_first(ys)->data, "B"))
		ys = g_list_reverse(ys);

	GList *fst = g_list_first(ys);
	g_assert(strcmp(fst->data, "B") == 0);
	FKItem *item = g_hash_table_lookup(ht, fst->data);
	g_assert(item);
	g_assert(strcmp(item->key, "B") == 0);
	g_assert(item->rdeps == NULL);
	g_assert(item->flags == 0);

	GList *snd = g_list_last(ys);
	g_assert(strcmp(snd->data, "C") == 0);
	item = g_hash_table_lookup(ht, snd->data);
	g_assert(item);
	g_assert(strcmp(item->key, "C") == 0);
	g_assert(item->rdeps);
	g_assert(item->flags == FK_FLAG_INACTIVE);

	g_list_free(ys);
	fk_finalize();
}

void test_two()
{
	puts("Running test_two...");
	/* This is the same as test_one, but B is deleted rather than A */

	fk_initialize((FKDestroyCallback) destroy);

	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "C");
	fk_add_relation("A", xs);
	fk_add_relation("B", xs);
	fk_delete("B");
	g_slist_free(xs);

	GHashTable *ht = fk_get_hash_table();
	g_assert(g_hash_table_size(ht) == 2);
	GList *ys = g_hash_table_get_keys(ht);
	g_assert(g_list_length(ys) == 2);

	/* The list ys should just contain A and C. If the first item is not A,
	 * reverse the list */
	if (strcmp(g_list_first(ys)->data, "A"))
		ys = g_list_reverse(ys);

	GList *fst = g_list_first(ys);
	g_assert(strcmp(fst->data, "A") == 0);
	FKItem *item = g_hash_table_lookup(ht, fst->data);
	g_assert(item);
	g_assert(strcmp(item->key, "A") == 0);
	g_assert(item->rdeps == NULL);
	g_assert(item->flags == 0);

	GList *snd = g_list_last(ys);
	g_assert(strcmp(snd->data, "C") == 0);
	item = g_hash_table_lookup(ht, snd->data);
	g_assert(item);
	g_assert(strcmp(item->key, "C") == 0);
	g_assert(item->rdeps);
	g_assert(item->flags == FK_FLAG_INACTIVE);

	g_list_free(ys);
	fk_finalize();
}

void test_three()
{
	puts("Running test_three...");

	fk_initialize((FKDestroyCallback) destroy);

	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "C");
	fk_add_relation("A", xs);
	fk_add_relation("B", xs);
	fk_delete("C");
	g_slist_free(xs);

	GHashTable *ht = fk_get_hash_table();
	printf("ht size is %d\n", g_hash_table_size(ht));
	g_assert(g_hash_table_size(ht) == 0);
	fk_finalize();
}

int main(void)
{
	test_one();
	test_two();
	test_three();
#if 0
	/* A -> B C D
	 * D -> E F
	 */
	fk_initialize((FKDestroyCallback) destroy);
	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "B");
	xs = g_slist_prepend(xs, "C");
	xs = g_slist_prepend(xs, "D");

	puts("Adding A -> {B, C, D}");
	fk_add_relation("A", xs);
	g_slist_free(xs);

	GSList *ys = NULL;
	ys = g_slist_prepend(ys, "E");
	ys = g_slist_prepend(ys, "F");

	puts("Adding D -> {E, F}");
	fk_add_relation("D", ys);
	g_slist_free(ys);

	puts("calling fk_delete(\"F\")");
	fk_delete("F");
	puts("calling fk_delete(\"F\")");
	fk_delete("F");

	fk_finalize();
#endif

	return 0;
}
