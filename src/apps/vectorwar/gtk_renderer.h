#ifndef _GTK_RENDERER_H_
#define _GTK_RENDERER_H_

#include "renderer.h"
#include <gtk/gtk.h>
#include <cairo.h>

/*
 * gtk_renderer.h --
 *
 * A simple C++ renderer that uses GTK and Cairo to render the game state.
 *
 */

class GTKRenderer : public Renderer {
public:
   GTKRenderer(GtkWidget *widget);
   ~GTKRenderer();

   virtual void Draw(GameState &gs, NonGameState &ngs);
   virtual void SetStatusText(const char *text);

protected:
   void RenderChecksum(cairo_t *cr, int y, NonGameState::ChecksumInfo &info);
   void DrawShip(cairo_t *cr, int which, GameState &gamestate);
   void DrawConnectState(cairo_t *cr, Ship &ship, PlayerConnectionInfo &info);

   GtkWidget     *_widget;
   char           _status[1024];
   GdkRGBA        _shipColors[4];
};

#endif
