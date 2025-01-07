#include <gtk/gtk.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "vectorwar.h"
#include "ggpo_perfmon.h"

// Callback for key press events
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    switch (event->keyval) {
    case GDK_KEY_P:
        ggpoutil_perfmon_toggle();
        break;
    case GDK_KEY_Escape:
        VectorWar_Exit();
        gtk_main_quit();
        break;
    case GDK_KEY_F1:
    case GDK_KEY_F2:
    case GDK_KEY_F3:
    case GDK_KEY_F4:
    case GDK_KEY_F5:
    case GDK_KEY_F6:
    case GDK_KEY_F7:
    case GDK_KEY_F8:
    case GDK_KEY_F9:
    case GDK_KEY_F10:
    case GDK_KEY_F11:
    case GDK_KEY_F12:
        VectorWar_DisconnectPlayer(event->keyval - GDK_KEY_F1);
        break;
    default:
        break;
    }
    return FALSE;
}

// Callback for draw events
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    VectorWar_DrawCurrentFrame();
    return FALSE;
}

// Create the main application window
GtkWidget *create_main_window() {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GGPO SDK Sample: Vector War");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(window, "draw", G_CALLBACK(on_draw), NULL);

    return window;
}

// Main loop equivalent to RunMainLoop
void run_main_loop(GtkWidget *window) {
    gint64 next_frame_time = g_get_real_time();

    g_timeout_add(1, [](gpointer data) -> gboolean {
        static gint64 next_frame_time = g_get_real_time();
        gint64 now = g_get_real_time();
        VectorWar_Idle(MAX(0, next_frame_time - now - 1));
        if (now >= next_frame_time) {
            VectorWar_RunFrame(GTK_WIDGET(data));
            next_frame_time = now + (1000000 / 60); // 60 FPS
        }
        return TRUE;
    }, window);

    gtk_main();
}

void syntax() {
    g_printerr("Syntax: vectorwar <local port> <num players> ('local' | <remote ip>:<remote port>)*\n");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    if (argc < 3) {
        syntax();
        return 1;
    }

    GtkWidget *window = create_main_window();
    gtk_widget_show_all(window);

    unsigned short local_port = static_cast<unsigned short>(std::atoi(argv[1]));
    int num_players = std::atoi(argv[2]);

    if (num_players <= 0 || argc < 3 + num_players) {
        syntax();
        return 1;
    }

    GGPOPlayer players[GGPO_MAX_PLAYERS] = {};
    int local_player = -1;

    for (int i = 0; i < num_players; ++i) {
        if (std::strcmp(argv[3 + i], "local") == 0) {
            players[i].type = GGPO_PLAYERTYPE_LOCAL;
            players[i].player_num = i + 1;
            local_player = i;
        } else {
            char *ip_port = argv[3 + i];
            char *colon = std::strchr(ip_port, ':');
            if (!colon) {
                syntax();
                return 1;
            }

            *colon = '\0';
            players[i].type = GGPO_PLAYERTYPE_REMOTE;
            players[i].player_num = i + 1;
            std::strncpy(players[i].u.remote.ip_address, ip_port, sizeof(players[i].u.remote.ip_address) - 1);
            players[i].u.remote.port = static_cast<unsigned short>(std::atoi(colon + 1));
        }
    }

    VectorWar_Init(window, local_port, num_players, players, 0);
    run_main_loop(window);
    VectorWar_Exit();

    return 0;
}
