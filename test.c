#include "fk.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void destroy(const char *s)
{
	printf("DESTROY \"%s\"\n", s);
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
		fk_finalize();
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
		GList *y = item->rdeps;
		while (y) {
			FKItem *i = y->data;
			fputc('\t', dot_file);
			fputs(i->key, dot_file);
			fputs(" -> ", dot_file);
			fputs(item->key, dot_file);
			fputc('\n', dot_file);
			y = y->next;
		}
		y = item->fdeps;
		while (y) {
			FKItem *i = y->data;
			fputc('\t', dot_file);
			fputs(i->key, dot_file);
			fputs(" -> ", dot_file);
			fputs(item->key, dot_file);
			fputs(" [ style = dashed ]\n", dot_file);
			y = y->next;
		}
		x = x->next;
	}
	g_list_free(list);
	fputs("}\n", dot_file);

	fclose(dot_file);
	wait(NULL);
}

static void chti_helper(char *key, FKItem *value, gpointer user_data)
{
	g_assert(key);
	g_assert(value);
	g_assert(!user_data);

	/* This should be an identity test, not a structural test */
	g_assert(key == value->key);

	FKItem *item;
	GList *xs;

	/* For each rdep, check that the rdep has this item as an fdep */
	GList *list = value->rdeps;
	while (list) {
		item = list->data;
		xs = item->fdeps;
		while (xs) {
			if (xs->data == value)
				break;
			xs = xs->next;
		}
		/* If this assertion fails, an rdep of value didn't have value as an
		 * fdep */
		g_assert(xs);
		list = list->next;
	}

	/* For each fdep, check that the fdep has this item as an rdep */
	list = value->fdeps;
	while (list) {
		item = list->data;
		xs = item->rdeps;
		while (xs) {
			if (xs->data == value)
				break;
			xs = xs->next;
		}
		/* If this assertion fails, an fdep of value didn't have value as an
		 * rdep */
		g_assert(xs);
		list = list->next;
	}
}


void check_hash_table_integrity()
{
	GHashTable *ht = fk_get_hash_table();
	g_hash_table_foreach(ht, (GHFunc) chti_helper, NULL);
}

void test_one()
{
	printf("Running test_one...");
	/* A and B are both active and depend on C, which is inactive. After
	 * deleting A, the graph should only contain B and C */

	fk_initialize(NULL);

	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "C");
	fk_add_relation("A", xs);
	check_hash_table_integrity();
	fk_add_relation("B", xs);
	check_hash_table_integrity();
	fk_delete("A");
	check_hash_table_integrity();
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
	puts(" passed!");
}

void test_two()
{
	printf("Running test_two...");
	/* This is the same as test_one, but B is deleted rather than A */

	fk_initialize(NULL);

	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "C");
	fk_add_relation("A", xs);
	check_hash_table_integrity();
	fk_add_relation("B", xs);
	check_hash_table_integrity();
	fk_delete("B");
	check_hash_table_integrity();
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
	puts(" passed!");
}

void test_three()
{
	printf("Running test_three...");

	fk_initialize(NULL);

	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "C");
	fk_add_relation("A", xs);
	check_hash_table_integrity();
	fk_add_relation("B", xs);
	check_hash_table_integrity();
	fk_delete("C");
	g_slist_free(xs);
	make_graph_png("three.png");

	GHashTable *ht = fk_get_hash_table();
	g_assert(g_hash_table_size(ht) == 0);
	fk_finalize();
	puts(" passed!");
}

void test_four()
{
	printf("Running test_four...");
	/* A -> B C D
	 * D -> E F
	 * G -> F
	 *
	 * Then delete F
	 */
	fk_initialize(NULL);
	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "B");
	xs = g_slist_prepend(xs, "C");
	xs = g_slist_prepend(xs, "D");
	fk_add_relation("A", xs);
	g_slist_free(xs);
	check_hash_table_integrity();

	xs = NULL;
	xs = g_slist_prepend(xs, "E");
	xs = g_slist_prepend(xs, "F");
	fk_add_relation("D", xs);
	g_slist_free(xs);
	check_hash_table_integrity();

	xs = NULL;
	xs = g_slist_prepend(xs, "F");
	fk_add_relation("G", xs);
	g_slist_free(xs);

	fk_delete("F");
	GHashTable *ht = fk_get_hash_table();
	g_assert(g_hash_table_size(ht) == 0);

	/* Try deleting again, just to make sure nothing crashes */
	fk_delete("F");

	fk_finalize();
	puts(" passed!");
}

void test_five()
{
	/* Just tests fk_inactivate */
	printf("Running test_five...");
	fk_initialize(NULL);
	fk_add_relation("A", NULL);
	check_hash_table_integrity();
	fk_inactivate("A");

	GHashTable *ht = fk_get_hash_table();
	g_assert(!g_hash_table_size(ht));

	fk_inactivate("A");
	g_assert(!g_hash_table_size(ht));

	fk_add_relation("C", NULL);
	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "C");
	fk_add_relation("B", xs);
	g_slist_free(xs);

	g_assert(((FKItem *) g_hash_table_lookup(ht, "C"))->flags == 0);
	g_assert(((FKItem *) g_hash_table_lookup(ht, "B"))->flags == 0);
	fk_inactivate("C");
	g_assert(((FKItem *) g_hash_table_lookup(ht, "B"))->flags == 0);
	g_assert(((FKItem *) g_hash_table_lookup(ht, "C"))->flags == FK_FLAG_INACTIVE);

	fk_finalize();
	puts(" passed!");
}

void test_six()
{
	/* Test fk_inactivate does delete_forward */
	printf("Running test_six...");
	fk_initialize(NULL);

	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "B");
	fk_add_relation("A", xs);
	g_slist_free(xs);
	check_hash_table_integrity();
	fk_inactivate("A");
	GHashTable *ht = fk_get_hash_table();
	g_assert(!g_hash_table_size(ht));

	fk_finalize();
	puts(" passed!");
}

void test_seven()
{
	printf("Running test_seven...");

	/* This is just a more complicated inactivation test */
	fk_initialize(NULL);

	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "E");
	fk_add_relation("D", xs);
	check_hash_table_integrity();

	xs->data = "D";
	fk_add_relation("C", xs);
	check_hash_table_integrity();

	xs->data = "C";
	fk_add_relation("B", xs);
	check_hash_table_integrity();

	xs->data = "B";
	fk_add_relation("A", xs);
	check_hash_table_integrity();
	g_slist_free(xs);

	GHashTable *ht = fk_get_hash_table();
	g_assert(g_hash_table_size(ht) == 5);

	/* Now we have a chain A -> B -> C -> D -> E */
	fk_inactivate("B");
	fk_inactivate("C");
	fk_inactivate("E");
	check_hash_table_integrity();
	g_assert(g_hash_table_size(ht) == 5);
	make_graph_png("seven.png");

	/* If we inactivate A, the new chain should just be D -> E */
	fk_inactivate("A");
	check_hash_table_integrity();
	g_assert(g_hash_table_size(ht) == 2);

	FKItem *item = g_hash_table_lookup(ht, "D");
	g_assert(item);
	g_assert(item->flags == 0);
	item = g_hash_table_lookup(ht, "E");
	g_assert(item);
	g_assert(item->flags == FK_FLAG_INACTIVE);

	fk_finalize();
	puts(" passed!");
}

void test_eight()
{
	printf("Running test_eight...");

	/* A -> B. Set A inactive, then B */
	fk_initialize(NULL);

	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "B");
	fk_add_relation("A", xs);
	g_slist_free(xs);
	fk_add_relation("B", NULL);
	check_hash_table_integrity();

	GHashTable *ht = fk_get_hash_table();
	g_assert(g_hash_table_size(ht) == 2);
	FKItem *item = g_hash_table_lookup(ht, "A");
	g_assert(item->flags == 0);
	g_assert(item->fdeps);
	g_assert(!item->rdeps);
	item = g_hash_table_lookup(ht, "B");
	g_assert(item->flags == 0);

	fk_inactivate("A");

	g_assert(g_hash_table_size(ht) == 1);
	check_hash_table_integrity();
	item = g_hash_table_lookup(ht, "A");
	g_assert(!item);
	item = g_hash_table_lookup(ht, "B");
	g_assert(item->flags == 0);
	fk_inactivate("B");

	g_assert(g_hash_table_size(ht) == 0);

	fk_finalize();
	puts(" passed!");
}

int main(void)
{
	test_one();
	test_two();
	test_three();
	test_four();
	test_five();
	test_six();
	test_seven();
	test_eight();
	return 0;
}
