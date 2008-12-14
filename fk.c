#include <glib.h>
#include <stdio.h>
#include "fk.h"

#define FK_NOT_DELETED(x) (!(x->flags & FK_FLAG_DELETED))

GHashTable *FK_HASH;

typedef struct {
	gconstpointer key;
	gint flags;
	GSList *rdeps;
} FKItem;

static void FK_DESTROY(const gchar *s)
{
	printf("FK_DESTROY(\"%s\")\n", s);
}

void fk_initialize()
{
	FK_HASH = g_hash_table_new(g_str_hash, g_str_equal);
}

/* Destroy an item in the list */
static void fk_item_destroy(FKItem *item)
{
	/* TODO: invoke callback */
	g_slist_free(item->rdeps);
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
	FKItem *item = g_hash_table_lookup(FK_HASH, name);
	if (item == NULL) {
		item = g_slice_alloc(sizeof(FKItem));
		item->key = name;
		item->flags = 0;
		item->rdeps = NULL;
		g_hash_table_insert(FK_HASH, (gpointer) name, item);
	}

	/* For each dependency */
	while (deps) {
		const gchar *rdep_name = deps->data;
		FKItem *dep_item = g_hash_table_lookup(FK_HASH, rdep_name);
		if (!dep_item) {
			dep_item = g_slice_alloc(sizeof(FKItem));
			dep_item->key = rdep_name;
			dep_item->flags = FK_FLAG_INACTIVE;
			dep_item->rdeps = NULL;
			g_hash_table_insert(FK_HASH, (gpointer) rdep_name, dep_item);
		}

		gboolean found_rdep = FALSE;
		GSList *list = dep_item->rdeps;
		while (list) {
			if (list->data == item) {
				found_rdep = TRUE;
				break;
			}
		}
		if (!found_rdep)
			dep_item->rdeps = g_slist_prepend(dep_item->rdeps, item);

		deps = deps->next;
	}
}

/* Mark an item as inactive, if it exists */
void fk_inactivate(const gchar *name)
{
	FKItem *item = g_hash_table_lookup(FK_HASH, name);
	if (item)
		item->flags |= FK_FLAG_INACTIVE;
}

/**********************
 * DELETION METHODS
 *********************/

static void fk_delete_ht(FKItem *data, gpointer user_data)
{
	g_assert(data);
	g_assert(!user_data);
	g_hash_table_remove(FK_HASH, data->key);
}

static void fk_delete_item(FKItem *item, GSList **list)
{
	g_assert(item);
	g_assert(item->key);
	g_assert(!(item->flags & FK_FLAG_DELETED));

	item->flags |= FK_FLAG_DELETED;
	*list = g_slist_prepend(*list, item);

	/* if the item is not inactive, the destroy callback needs to be
	 * called */
	if (!(item->flags & FK_FLAG_INACTIVE))
		FK_DESTROY(item->key);
	else
		printf("not deleting %s, it is inactive\n", item->key);

	/* call fk_delete on all reverse dependencies */
	while (item->rdeps) {
		FKItem *it = item->rdeps->data;
		g_assert(it);
		printf("Deleting %s\n", it->key);
		if (FK_NOT_DELETED(it))
			fk_delete_item(it, list);
		item->rdeps = item->rdeps->next;
	}
}

void fk_delete(const gchar *name)
{
	FKItem *item = g_hash_table_lookup(FK_HASH, name);
	if (item) {
		GSList *list = NULL;
		fk_delete_item(item, &list);

		/* Remove all of the nodes from the keyed data list */
		g_slist_foreach(list, (GFunc) fk_delete_ht, NULL);
		g_slist_free(list);
	} else {
		fprintf(stderr, "fk_delete: no item found for \"%s\"\n", name);
	}
}
