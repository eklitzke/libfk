#ifndef _FK_H_
#define _FK_H_

#include <glib.h>

#define FK_FLAG_INACTIVE 0x01
#define FK_FLAG_DELETED  0x02

typedef void (*FKDestroyCallback) (const char *key);

typedef struct {
	gconstpointer key;
	gint flags;
	GSList *rdeps;
} FKItem;

void fk_initialize(FKDestroyCallback cb);
void fk_finalize();

void fk_add_relation(const gchar *name, GSList *deps);
void fk_delete(const gchar *name);
void fk_inactivate(const gchar *name);

#ifdef FK_UNIT_TEST
GHashTable* fk_get_hash_table();
#endif

#endif
