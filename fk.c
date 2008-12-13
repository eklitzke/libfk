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

void fk_delete_item(FKItem *item);

void fk_initialize()
{
	g_datalist_init(&FK_DATA);
}

/* Destroy an item in the list */
void fk_item_destroy(FKItem *item)
{
	g_slice_free(FKItem, item);
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
		g_datalist_id_set_data_full(&FK_DATA, quark, this, fk_item_destroy);
	} else {
		this->flags &= ~FK_FLAG_INACTIVE;
	}

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
			g_datalist_id_set_data_full(&FK_DATA, q, item, fk_item_destroy);
		}

		/* note the rdep, if it isn't already stored */
		if (g_slist_index(item->rdeps, this) == -1)
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

void fk_delete(const gchar *name)
{
	GQuark quark = g_quark_try_string(name);
	if (quark) {
		FKItem *item = g_datalist_id_get_data(&FK_DATA, quark);
		fk_delete_item(item);
	} else {
		fprintf(stderr, "fk_delete: no quark found for \"%s\"\n", name);
	}
}

void fk_delete_item(FKItem *item)
{
	g_assert(item);
	g_assert(!(item->flags & FK_FLAG_DELETED));

	item->flags |= FK_FLAG_DELETED;

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
		if (FK_NOT_DELETED(it)) {
			fk_delete_item(it);
		}
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
	return 0;
}
