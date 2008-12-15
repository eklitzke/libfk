#include <glib.h>
#include <string.h>
#include <stdio.h>
#include "fk.h"

/***********
 * GLOBALS *
 **********/

GHashTable *FK_HASH;

FKDestroyCallback FK_DESTROY_CALLBACK;

/**********************
 * SETUP AND TEARDOWN *
 *********************/

/* Function to free a key. The key must be an ordinary null-terminated C
 * string. */
static void fk_free_key_func(char *key)
{
	g_assert(key);
	g_slice_free1(strlen(key) + 1, key);
}

/* Function to free a value. */
static void fk_free_val_func(FKItem *val)
{
	g_assert(val);

	/* N.B. We don't free the key. The key in the FKItem will be the same as
	 * the key that is used in hash table, so it is the responsibility of
	 * fk_free_key_func to free that data. */
	g_slice_free(FKItem, val);

	/* Just free the list; the actual elements in the list do not need to be
	 * freed. */
	g_slist_free(val->rdeps);
}

/* Initialize the library */
void fk_initialize(FKDestroyCallback cb)
{
	g_assert(!FK_HASH);
	g_assert(!FK_DESTROY_CALLBACK);
	g_assert(cb);

	FK_DESTROY_CALLBACK = cb;
	FK_HASH = g_hash_table_new_full(
			g_str_hash, g_str_equal, (GDestroyNotify) fk_free_key_func,
			(GDestroyNotify) fk_free_val_func);

	g_assert(FK_HASH);
	g_assert(FK_DESTROY_CALLBACK);
}

/* Finalize (tear down) the library. After this function is called, all memory
 * allocated by the library should be freed. */
void fk_finalize()
{
	g_assert(FK_HASH);
	g_assert(FK_DESTROY_CALLBACK);

	g_hash_table_destroy(FK_HASH);
	FK_HASH = NULL;
	FK_DESTROY_CALLBACK = NULL;
}

/**********************
 * RELATIONAL METHODS *
 *********************/

void fk_add_relation(const gchar *name, GSList *deps)
{
	g_assert(name);
	printf("Adding relation %s\n", name);

	FKItem *item = g_hash_table_lookup(FK_HASH, name);
	if (item == NULL) {
		item = g_slice_alloc(sizeof(FKItem));
		item->key = g_slice_copy(strlen(name) + 1, name);
		item->flags = 0;
		item->rdeps = NULL;
		g_hash_table_insert(FK_HASH, (gpointer) item->key, item);
	} else {
		item->flags &= ~FK_FLAG_INACTIVE;
	}

	/* For each dependency */
	while (deps) {
		const gchar *rdep_name = deps->data;
		g_assert(rdep_name);

		FKItem *dep_item = g_hash_table_lookup(FK_HASH, rdep_name);
		if (!dep_item) {
			dep_item = g_slice_alloc(sizeof(FKItem));
			dep_item->key = g_slice_copy(strlen(rdep_name) + 1, rdep_name);
			dep_item->flags = FK_FLAG_INACTIVE;
			dep_item->rdeps = NULL;
			g_hash_table_insert(FK_HASH, (gpointer) dep_item->key, dep_item);
		}

		gboolean found_rdep = FALSE;
		GSList *list = dep_item->rdeps;
		while (list) {
			if (list->data == item) {
				found_rdep = TRUE;
				break;
			}
			list = list->next;
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

static void fk_delete_ht(FKItem *data, gpointer null_user_data)
{
	g_assert(data);
	g_assert(!null_user_data);
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
		FK_DESTROY_CALLBACK((const char *) item->key);
	else
		printf("not deleting %s, it is inactive\n", item->key);

	/* call fk_delete on all reverse dependencies */
	while (item->rdeps) {
		FKItem *it = item->rdeps->data;
		g_assert(it);
		if (!(it->flags & FK_FLAG_DELETED))
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

#ifdef FK_UNIT_TEST
/*********************
 * UNIT TEST METHODS *
 ********************/

GHashTable* fk_get_hash_table()
{
	return FK_HASH;
}
#endif

/* vim: noet */
