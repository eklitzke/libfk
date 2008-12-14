#ifndef _FK_H_
#define _FK_H_

#include <glib.h>

#define FK_FLAG_INACTIVE 0x01
#define FK_FLAG_DELETED  0x02

void fk_add_relation(const gchar *name, GSList *deps);
void fk_delete(const gchar *name);
void fk_initialize();

#endif
