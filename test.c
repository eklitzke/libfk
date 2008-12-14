#include "fk.h"
#include <stdio.h>

static void destroy(const char *s)
{
	printf("DESTROY(\"%s\")\n", s);
}


int main(void)
{
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

	return 0;
}
