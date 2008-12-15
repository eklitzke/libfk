#include "fk.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void destroy(const char *s)
{
	printf("DESTROY(\"%s\")\n", s);
}

void make_graph_png(const char *fname)
{

	int pipefd[2];
	if (pipe(pipefd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	pid_t cpid = fork();
	if (cpid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (cpid == 0) {
		close(pipefd[1]);
		dup2(pipefd[0], 0);
		execlp("dot", "dot", "-Tpng", "-o", fname, (char *) NULL);
		perror("execlp");
		exit(EXIT_FAILURE);
	} else
		close(pipefd[0]);

	FILE *dot_file = fdopen(pipefd[1], "w");
	if (!dot_file) {
		perror("fdopen");
		exit(EXIT_FAILURE);
	}

	fputs("digraph G {\n", dot_file);

	GHashTable *ht = fk_get_hash_table();
	GList *list = g_hash_table_get_values(ht);
	GList *x = list;
	while (x) {
		FKItem *item = x->data;
		if (item->flags & FK_FLAG_INACTIVE) {
			fputc('\t', dot_file);
			fputs(item->key, dot_file);
			fputs(" [style=filled]\n", dot_file);
		}
		GList *y = item->fdeps;
		while (y) {
			FKItem *i = y->data;
			fputc('\t', dot_file);
			fputs(item->key, dot_file);
			fputs(" -> ", dot_file);
			fputs(i->key, dot_file);
			fputc('\n', dot_file);
			y = y->next;
		}
		x = x->next;
	}
	g_list_free(list);
	fputs("}\n", dot_file);

	fclose(dot_file);
	//close(pipefd[0]);
	printf("waiting\n");
	wait(NULL);
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
	make_graph_png("one.png");
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
	make_graph_png("two.png");
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
