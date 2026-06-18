#ifndef DASHBOARD_UI_H
#define DASHBOARD_UI_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

gboolean aura_launch_dashboard(int *argc, char ***argv);
gboolean aura_dashboard_restart_requested(void);
void aura_dashboard_clear_restart_request(void);

G_END_DECLS

#endif // DASHBOARD_UI_H
