#include "fk.h"

int main(void)
{
	/* A -> B C D
	 * D -> E F
	 */
	fk_initialize();
	GSList *xs = NULL;
	xs = g_slist_prepend(xs, "B");
	xs = g_slist_prepend(xs, "C");
	xs = g_slist_prepend(xs, "D");

	fk_add_relation("A", xs);
	g_slist_free(xs);

	GSList *ys = NULL;
	ys = g_slist_prepend(ys, "E");
	ys = g_slist_prepend(ys, "F");

	fk_add_relation("D", ys);
	g_slist_free(ys);

	puts("calling fk_delete(\"F\")");
	fk_delete("F");
	puts("calling fk_delete(\"F\")");
	fk_delete("F");

	return 0;
}
