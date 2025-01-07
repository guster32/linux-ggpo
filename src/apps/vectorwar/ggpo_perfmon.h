#ifndef _GGPOUTIL_PERFMON_H
#define _GGPOUTIL_PERFMON_H

#include "../../include/ggponet.h"
#include <gtk/gtk.h>

void ggpoutil_perfmon_init(GtkWidget *window);
void ggpoutil_perfmon_update(GGPOSession *ggpo, GGPOPlayerHandle players[], int num_players);
void ggpoutil_perfmon_toggle();

#endif
