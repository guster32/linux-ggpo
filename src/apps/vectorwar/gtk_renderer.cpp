#include <gtk/gtk.h>
#include <cairo.h>
#include <cmath>
#include <cstring>
#include "vectorwar.h"
#include "gtk_renderer.h"

#define PROGRESS_BAR_WIDTH        100
#define PROGRESS_BAR_TOP_OFFSET    22
#define PROGRESS_BAR_HEIGHT         8
#define PROGRESS_TEXT_OFFSET       (PROGRESS_BAR_TOP_OFFSET + PROGRESS_BAR_HEIGHT + 4)

GTKRenderer::GTKRenderer(GtkWidget *widget) :
   _widget(widget)
{
   memset(_status, 0, sizeof(_status));

   // Ship colors
   _shipColors[0] = {1.0, 0.0, 0.0, 1.0}; // Red
   _shipColors[1] = {0.0, 1.0, 0.0, 1.0}; // Green
   _shipColors[2] = {0.0, 0.0, 1.0, 1.0}; // Blue
   _shipColors[3] = {0.5, 0.5, 0.5, 1.0}; // Gray
}

GTKRenderer::~GTKRenderer() {}

void GTKRenderer::Draw(GameState &gs, NonGameState &ngs) {
   cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(_widget));

   cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black background
   cairo_paint(cr);

   cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White border
   cairo_set_line_width(cr, 2.0);
   cairo_rectangle(cr, gs._bounds.x, gs._bounds.y, gs._bounds.width, gs._bounds.height);
   cairo_stroke(cr);

   for (int i = 0; i < gs._num_ships; i++) {
      cairo_set_source_rgba(cr, _shipColors[i].red, _shipColors[i].green, _shipColors[i].blue, _shipColors[i].alpha);
      DrawShip(cr, i, gs);
      DrawConnectState(cr, gs._ships[i], ngs.players[i]);
   }

   cairo_destroy(cr);
}

void GTKRenderer::RenderChecksum(cairo_t *cr, int y, NonGameState::ChecksumInfo &info) {
   char checksum[128];
   snprintf(checksum, sizeof(checksum), "Frame: %04d  Checksum: %08x", info.framenumber, info.checksum);

   cairo_set_source_rgb(cr, 0.8, 0.8, 0.8); // Gray text
   cairo_move_to(cr, 10, y);
   cairo_show_text(cr, checksum);
}

void GTKRenderer::SetStatusText(const char *text) {
   strncpy(_status, text, sizeof(_status) - 1);
   _status[sizeof(_status) - 1] = '\0';
}

void GTKRenderer::DrawShip(cairo_t *cr, int which, GameState &gs) {
   Ship *ship = &gs._ships[which];
   cairo_save(cr);

   cairo_translate(cr, ship->position.x, ship->position.y);
   cairo_rotate(cr, ship->heading * M_PI / 180.0);

   cairo_move_to(cr, SHIP_RADIUS, 0);
   cairo_line_to(cr, -SHIP_RADIUS, SHIP_WIDTH);
   cairo_line_to(cr, -SHIP_RADIUS + SHIP_TUCK, 0);
   cairo_line_to(cr, -SHIP_RADIUS, -SHIP_WIDTH);
   cairo_close_path(cr);

   cairo_fill(cr);
   cairo_restore(cr);

   // Draw bullets
   for (int i = 0; i < MAX_BULLETS; i++) {
      if (ship->bullets[i].active) {
         cairo_arc(cr, ship->bullets[i].position.x, ship->bullets[i].position.y, 2, 0, 2 * M_PI);
         cairo_fill(cr);
      }
   }

   // Draw score
   char buf[32];
   snprintf(buf, sizeof(buf), "Hits: %d", ship->score);
   cairo_move_to(cr, gs._bounds.x + 10, gs._bounds.y + 20 * (which + 1));
   cairo_show_text(cr, buf);
}

void GTKRenderer::DrawConnectState(cairo_t *cr, Ship &ship, PlayerConnectionInfo &info) {
   char status[64] = {0};
   int progress = -1;

   switch (info.state) {
      case Connecting:
         snprintf(status, sizeof(status), "Connecting...");
         break;
      case Synchronizing:
         progress = info.connect_progress;
         snprintf(status, sizeof(status), "Synchronizing...");
         break;
      case Disconnected:
         snprintf(status, sizeof(status), "Disconnected");
         break;
      case Disconnecting:
         snprintf(status, sizeof(status), "Waiting for player...");
         progress = (g_get_monotonic_time() - info.disconnect_start) * 100 / info.disconnect_timeout;
         break;
   }

   if (status[0]) {
      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White text
      cairo_move_to(cr, ship.position.x, ship.position.y + PROGRESS_TEXT_OFFSET);
      cairo_show_text(cr, status);
   }

   if (progress >= 0) {
      double bar_width = PROGRESS_BAR_WIDTH * progress / 100.0;
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); // Red bar
      cairo_rectangle(cr, ship.position.x - PROGRESS_BAR_WIDTH / 2, ship.position.y + PROGRESS_BAR_TOP_OFFSET, bar_width, PROGRESS_BAR_HEIGHT);
      cairo_fill(cr);
   }
}
