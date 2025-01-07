#include <gtk/gtk.h>
#include <cairo.h>
#include <cmath>
#include <cstring>
#include "ggpo_perfmon.h"

#define MAX_GRAPH_SIZE      4096
#define MAX_FAIRNESS          20
#define MAX_PLAYERS            4

static GtkWidget *_window = NULL;
static GtkWidget *_dialog = NULL;
static gboolean _shown = FALSE;
static int _last_text_update_time = 0;

int _num_players;
int _first_graph_index = 0;
int _graph_size = 0;
int _ping_graph[MAX_PLAYERS][MAX_GRAPH_SIZE];
int _local_fairness_graph[MAX_PLAYERS][MAX_GRAPH_SIZE];
int _remote_fairness_graph[MAX_PLAYERS][MAX_GRAPH_SIZE];
int _fairness_graph[MAX_GRAPH_SIZE];
int _predict_queue_graph[MAX_GRAPH_SIZE];
int _remote_queue_graph[MAX_GRAPH_SIZE];
int _send_queue_graph[MAX_GRAPH_SIZE];

gboolean draw_graph(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    int *graph = (int *)data;
    int count = _graph_size;
    int range = 500; // Adjust range as needed.

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black background
    cairo_paint(cr);

    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0); // Green line
    cairo_set_line_width(cr, 1.0);

    int offset = 0;
    if (count > width) {
        offset = count - width;
        count = width;
    }

    for (int i = 0; i < count; ++i) {
        int value = graph[(_first_graph_index + offset + i) % MAX_GRAPH_SIZE];
        double x = i;
        double y = height - (value * height / range);

        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_stroke(cr);
    return FALSE;
}

void ggpoutil_perfmon_init(GtkWidget *window) {
    _window = window;

    if (!_dialog) {
        _dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(_dialog), "GGPO Performance Monitor");
        gtk_window_set_default_size(GTK_WINDOW(_dialog), 800, 600);

        GtkWidget *grid = gtk_grid_new();
        gtk_container_add(GTK_CONTAINER(_dialog), grid);

        GtkWidget *ping_graph = gtk_drawing_area_new();
        gtk_widget_set_size_request(ping_graph, 800, 300);
        gtk_grid_attach(GTK_GRID(grid), ping_graph, 0, 0, 1, 1);

        g_signal_connect(G_OBJECT(ping_graph), "draw", G_CALLBACK(draw_graph), _ping_graph[0]);
    }
}

void ggpoutil_perfmon_exit() {
    if (_dialog) {
        gtk_widget_destroy(_dialog);
        _dialog = NULL;
    }
}

void ggpoutil_perfmon_toggle() {
    if (!_dialog) {
        ggpoutil_perfmon_init(_window);
    }
    _shown = !_shown;
    if (_shown) {
        gtk_widget_show_all(_dialog);
    } else {
        gtk_widget_hide(_dialog);
    }
}

void ggpoutil_perfmon_update(GGPOSession *ggpo, GGPOPlayerHandle players[], int num_players) {
    GGPONetworkStats stats = {0};
    _num_players = num_players;

    int i = (_graph_size < MAX_GRAPH_SIZE) ? _graph_size++ : _first_graph_index;
    if (_graph_size >= MAX_GRAPH_SIZE) {
        _first_graph_index = (_first_graph_index + 1) % MAX_GRAPH_SIZE;
    }

    for (int j = 0; j < num_players; ++j) {
        ggpo_get_network_stats(ggpo, players[j], &stats);

        _ping_graph[j][i] = stats.network.ping;
        _local_fairness_graph[j][i] = stats.timesync.local_frames_behind;
        _remote_fairness_graph[j][i] = stats.timesync.remote_frames_behind;

        if (stats.timesync.local_frames_behind < 0 && stats.timesync.remote_frames_behind < 0) {
            _fairness_graph[i] = abs(abs(stats.timesync.local_frames_behind) - abs(stats.timesync.remote_frames_behind));
        } else if (stats.timesync.local_frames_behind > 0 && stats.timesync.remote_frames_behind > 0) {
            _fairness_graph[i] = 0;
        } else {
            _fairness_graph[i] = abs(stats.timesync.local_frames_behind) + abs(stats.timesync.remote_frames_behind);
        }
    }

    if (_dialog) {
        gtk_widget_queue_draw(GTK_WIDGET(_dialog));
    }
}
