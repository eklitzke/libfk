#include <glib.h>
#include <string.h>
#include <stdio.h>
#include "fk.h"

#define FK_IS_INACTIVE(x) (x & FK_FLAG_INACTIVE)

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
	printf("fk_free_key_func(%s)\n", key);
	g_slice_free1(strlen(key) + 1, key);
}

/* Function to free a value. */
static void fk_free_val_func(FKItem *val)
{
	g_assert(val);
	printf("fk_free_val_func(%p)\n", val);

	/* N.B. We don't free the key. The key in the FKItem will be the same as
	 * the key that is used in hash table, so it is the responsibility of
	 * fk_free_key_func to free that data. */
	g_slice_free(FKItem, val);

	/* Just free the list; the actual elements in the list do not need to be
	 * freed. */
	g_list_free(val->rdeps);
	g_list_free(val->fdeps);
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

	g_hash_table_unref(FK_HASH);
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

	gboolean is_new = FALSE;
	FKItem *item = g_hash_table_lookup(FK_HASH, name);
	if (item == NULL) {
		is_new = TRUE;
		item = g_slice_alloc(sizeof(FKItem));
		item->key = g_slice_copy(strlen(name) + 1, name);
		item->flags = 0;
		item->fdeps = NULL;
		item->rdeps = NULL;
		//printf("inserting to ht %s\n", item->key);
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
			dep_item->fdeps = NULL;
			dep_item->rdeps = NULL;
			//printf("inserting to ht %s\n", dep_item->key);
			g_hash_table_insert(FK_HASH, (gpointer) dep_item->key, dep_item);
		}

		GList *list;

		/* The case of a new item is optimized specially */
		if (is_new)
			item->fdeps = g_list_prepend(item->fdeps, dep_item);
		else {
			list = item->fdeps;
			while (list) {
				if (list->data == dep_item)
					break;
				list = list->next;
			}
			if (!list)
				item->fdeps = g_list_prepend(item->fdeps, dep_item);
		}

		list = dep_item->rdeps;
		while (list) {
			if (list->data == item)
				break;
			list = list->next;
		}
		if (!list)
			dep_item->rdeps = g_list_prepend(dep_item->rdeps, item);

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

static void fk_delete_forward(FKItem *item)
{
	g_assert(item);
	GList *fdeps = item->fdeps;
	printf("fk_delete_forward(\"%s\")\n", item->key);
	while (fdeps) {

		FKItem *it = fdeps->data;
		printf("Trying forward delete %s -> %s\n", item->key, it->key);

		GList *x = g_list_find(it->rdeps, (gconstpointer) item);
		g_assert(x);
		it->rdeps = g_list_delete_link(it->rdeps, x);
		if (it->rdeps) {
			FKItem *i = it->rdeps->data;
			printf("it->rdeps head = %s\n", i->key);
		}

		if (it->flags & FK_FLAG_DELETED) {
			printf("Skipping forward delete of %s (from %s); already deleted\n", it->key, item->key);
			goto next_delete_forward;
		}
		printf("Not skipping forward delete of %s (from %s)\n", it->key, item->key);


		if (!it->rdeps && FK_IS_INACTIVE(it->flags)) {
			it->flags &= FK_FLAG_DELETED;
			fk_delete_forward(it);
			g_hash_table_remove(FK_HASH, it->key);
		}

next_delete_forward:
		fdeps = fdeps->next;
	}
}

static void fk_delete_item(FKItem *item)
{
	printf("fk_delete_item(\"%s\")\n", item->key);
	g_assert(item);
	g_assert(item->key);
	g_assert(!(item->flags & FK_FLAG_DELETED));

	item->flags |= FK_FLAG_DELETED;

	/* call fk_delete on all reverse dependencies */

	FKItem *it;


	GList *rdep = item->rdeps;
	while (rdep) {
		it = rdep->data;
		g_assert(it);
		printf("Moving up in fk_delete_item to %s (rdep of %s)\n", it->key, item->key);
		rdep = rdep->next;
		if (!(it->flags & FK_FLAG_DELETED))
			fk_delete_item(it);
	}

	/* Try deleting forward now */
	if (item->rdeps == NULL)
		fk_delete_forward(item);

	if (!(item->flags & FK_FLAG_INACTIVE)) {
		FK_DESTROY_CALLBACK((const char *) item->key);
		g_hash_table_remove(FK_HASH, item->key);
	} else if (!item->rdeps)
		g_hash_table_remove(FK_HASH, item->key);
	else
		g_assert_not_reached();
}

void fk_delete(const gchar *name)
{
	FKItem *item = g_hash_table_lookup(FK_HASH, name);
	if (item) {
		fk_delete_item(item);
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
