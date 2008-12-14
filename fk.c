#include <glib.h>
#include <stdio.h>

#define FK_FLAG_INACTIVE 0x01
#define FK_FLAG_DELETED  0x02

#define FK_NOT_DELETED(x) (!(x->flags & FK_FLAG_DELETED))

GData *FK_DATA;

void FK_DESTROY(const gchar *s)
{
	printf("FK_DESTROY(\"%s\")\n", s);
}

typedef struct {
	GQuark key;
	gint flags;
	GSList *rdeps;
} FKItem;

void fk_delete_item(FKItem *item, GSList **list);

void fk_initialize()
{
	g_datalist_init(&FK_DATA);
}

/* Destroy an item in the list */
static void fk_item_destroy(FKItem *item)
{
	/* TODO: invoke callback */
	g_slice_free(FKItem, item);
}

static gint fk_item_compare(const FKItem *a, const FKItem *b)
{
	if (a->key < b->key)
		return -1;
	else if (a->key == b->key)
		return 0;
	else
		return 1;
}

void fk_add_relation(const gchar *name, GSList *deps)
{
	GQuark quark = g_quark_from_string(name);
	FKItem *this = g_datalist_id_get_data(&FK_DATA, quark);
	if (this == NULL) {
		this = g_slice_alloc(sizeof(FKItem));
		this->key = quark;
		this->flags = 0;
		this->rdeps = NULL;
		g_datalist_id_set_data_full(&FK_DATA, quark, this, (GDestroyNotify) fk_item_destroy);
	}/* else {
		this->flags &= ~FK_FLAG_INACTIVE;
	}*/

	while (deps) {
		GQuark q = g_quark_from_string(deps->data);
		FKItem *item = g_datalist_id_get_data(&FK_DATA, q);

		/* possibly create the reverse dependency */
		if (item == NULL) {
			fprintf(stderr, "fk_add_relation: creating rdep \"%s\" for \"%s\"\n", deps->data, name);
			item = g_slice_alloc(sizeof(FKItem));
			item->key = q;
			item->flags = FK_FLAG_INACTIVE;
			item->rdeps = NULL;
			g_datalist_id_set_data_full(&FK_DATA, q, item, (GDestroyNotify) fk_item_destroy);
		}

		/* note the rdep, if it isn't already stored */
		if (!g_slist_find_custom(item->rdeps, this, (GCompareFunc) fk_item_compare))
			item->rdeps = g_slist_prepend(item->rdeps, this);

		deps = deps->next;
	}
}

/* Mark an item as inactive, if it exists */
void fk_inactivate(const gchar *name)
{
	GQuark quark = g_quark_try_string(name);
	if (quark) {
		FKItem *item = g_datalist_id_get_data(&FK_DATA, quark);
		if (item)
			item->flags |= FK_FLAG_INACTIVE;
	}
}

static void fk_delete_datalist(FKItem *data, gpointer user_data)
{
	g_assert(data);
	g_assert(!user_data);
	g_datalist_id_remove_data(&FK_DATA, data->key);
}

void fk_delete(const gchar *name)
{
	GQuark quark = g_quark_try_string(name);
	if (quark) {
		FKItem *item = g_datalist_id_get_data(&FK_DATA, quark);
		if (item) {
			GSList *list = NULL;
			fk_delete_item(item, &list);

			/* Remove all of the nodes from the keyed data list */
			g_slist_foreach(list, (GFunc) fk_delete_datalist, NULL);
			g_slist_free(list);
		}
	} else {
		fprintf(stderr, "fk_delete: no quark found for \"%s\"\n", name);
	}
}

void fk_delete_item(FKItem *item, GSList **list)
{
	g_assert(item);
	g_assert(!(item->flags & FK_FLAG_DELETED));

	item->flags |= FK_FLAG_DELETED;
	*list = g_slist_prepend(*list, item);

	/* if the item is not inactive, the destroy callback needs to be
	 * called */
	if (!(item->flags & FK_FLAG_INACTIVE)) {
		const gchar *s = g_quark_to_string(item->key);
		g_assert(s);
		FK_DESTROY(s);
	}

	/* call fk_delete on all reverse dependencies */
	while (item->rdeps) {
		FKItem *it = item->rdeps->data;
		g_assert(it);
		if (FK_NOT_DELETED(it))
			fk_delete_item(it, list);
		item->rdeps = item->rdeps->next;
	}
}

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

	GSList *ys = NULL;
	ys = g_slist_prepend(ys, "E");
	ys = g_slist_prepend(ys, "F");

	fk_add_relation("D", ys);

	puts("calling fk_delete(\"F\")");
	fk_delete("F");
	puts("calling fk_delete(\"F\")");
	fk_delete("F");

	g_slist_free(xs);
	g_slist_free(ys);
	g_datalist_clear(&FK_DATA);
	return 0;
}
