#ifndef _FK_H_
#define _FK_H_

#include <glib.h>

typedef void (*FKDestroyCallback) (const char *key);

void fk_initialize(FKDestroyCallback cb);
void fk_finalize();

void fk_add_relation(const gchar *name, GSList *deps);
void fk_delete(const gchar *name);
void fk_inactivate(const gchar *name);

#endif
