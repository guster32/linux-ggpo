#include <gtk/gtk.h>
#include <cairo.h>
#include <cmath>
#include <cstring>
#include "gtk_renderer.h"
#include "vectorwar.h"
#include "ggpo_perfmon.h"

//#define SYNC_TEST // Uncomment for sync test
#define MAX_PLAYERS 64

GameState gs = { 0 };
NonGameState ngs = { 0 };
Renderer *renderer = nullptr;
GGPOSession *ggpo = nullptr;
InputState input_state = {false, false, false, false, false, false};
/*
 * Simple checksum function stolen from Wikipedia:
 *   http://en.wikipedia.org/wiki/Fletcher%27s_checksum
 */
int fletcher32_checksum(short *data, size_t len) {
   int sum1 = 0xffff, sum2 = 0xffff;

   while (len) {
      size_t tlen = len > 360 ? 360 : len;
      len -= tlen;
      do {
         sum1 += *data++;
         sum2 += sum1;
      } while (--tlen);
      sum1 = (sum1 & 0xffff) + (sum1 >> 16);
      sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   return sum2 << 16 | sum1;
}

bool vw_begin_game_callback(const char *) {
   return true;
}

bool vw_on_event_callback(GGPOEvent *info) {
   int progress;
   switch (info->code) {
      case GGPO_EVENTCODE_CONNECTED_TO_PEER:
         ngs.SetConnectState(info->u.connected.player, Synchronizing);
         break;
      case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
         progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
         ngs.UpdateConnectProgress(info->u.synchronizing.player, progress);
         break;
      case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
         ngs.UpdateConnectProgress(info->u.synchronized.player, 100);
         break;
      case GGPO_EVENTCODE_RUNNING:
         ngs.SetConnectState(Running);
         renderer->SetStatusText("");
         break;
      case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
         ngs.SetDisconnectTimeout(info->u.connection_interrupted.player,
                                  g_get_real_time(),
                                  info->u.connection_interrupted.disconnect_timeout);
         break;
      case GGPO_EVENTCODE_CONNECTION_RESUMED:
         ngs.SetConnectState(info->u.connection_resumed.player, Running);
         break;
      case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
         ngs.SetConnectState(info->u.disconnected.player, Disconnected);
         break;
      case GGPO_EVENTCODE_TIMESYNC:
         g_usleep(1000 * info->u.timesync.frames_ahead / 60);
         break;
   }
   return true;
}

bool vw_advance_frame_callback(int) {
   int inputs[MAX_SHIPS] = { 0 };
   int disconnect_flags;
   ggpo_synchronize_input(ggpo, (void *)inputs, sizeof(int) * MAX_SHIPS, &disconnect_flags);
   VectorWar_AdvanceFrame(inputs, disconnect_flags);
   return true;
}

bool vw_load_game_state_callback(unsigned char *buffer, int len) {
   memcpy(&gs, buffer, len);
   return true;
}

bool vw_save_game_state_callback(unsigned char **buffer, int *len, int *checksum, int) {
   *len = sizeof(gs);
   *buffer = (unsigned char *)malloc(*len);
   if (!*buffer) {
      return false;
   }
   memcpy(*buffer, &gs, *len);
   *checksum = fletcher32_checksum((short *)*buffer, *len / 2);
   return true;
}

bool vw_log_game_state(char *filename, unsigned char *buffer, int) {
   FILE *fp = fopen(filename, "w");
   if (fp) {
      GameState *gamestate = (GameState *)buffer;
      fprintf(fp, "GameState object.\n");
      fprintf(fp, "  bounds: %d,%d x %d,%d.\n", gamestate->_bounds.x, gamestate->_bounds.y,
              gamestate->_bounds.width, gamestate->_bounds.height);
      fprintf(fp, "  num_ships: %d.\n", gamestate->_num_ships);
      for (int i = 0; i < gamestate->_num_ships; i++) {
         Ship *ship = gamestate->_ships + i;
         fprintf(fp, "  ship %d position:  %.4f, %.4f\n", i, ship->position.x, ship->position.y);
         fprintf(fp, "  ship %d velocity:  %.4f, %.4f\n", i, ship->velocity.dx, ship->velocity.dy);
         fprintf(fp, "  ship %d radius:    %d.\n", i, ship->radius);
         fprintf(fp, "  ship %d heading:   %d.\n", i, ship->heading);
         fprintf(fp, "  ship %d health:    %d.\n", i, ship->health);
         fprintf(fp, "  ship %d speed:     %d.\n", i, ship->speed);
         fprintf(fp, "  ship %d cooldown:  %d.\n", i, ship->cooldown);
         fprintf(fp, "  ship %d score:     %d.\n", i, ship->score);
         for (int j = 0; j < MAX_BULLETS; j++) {
            Bullet *bullet = ship->bullets + j;
            fprintf(fp, "  ship %d bullet %d: %.2f %.2f -> %.2f %.2f.\n", i, j,
                    bullet->position.x, bullet->position.y,
                    bullet->velocity.dx, bullet->velocity.dy);
         }
      }
      fclose(fp);
   }
   return true;
}

void vw_free_buffer(void *buffer) {
   free(buffer);
}

void VectorWar_Init(GtkWidget *widget, unsigned short localport, int num_players, GGPOPlayer *players, int num_spectators) {
   renderer = new GTKRenderer(widget);

   gs.Init(widget, num_players);
   ngs.num_players = num_players;

   GGPOSessionCallbacks cb = { 0 };
   cb.begin_game      = vw_begin_game_callback;
   cb.advance_frame   = vw_advance_frame_callback;
   cb.load_game_state = vw_load_game_state_callback;
   cb.save_game_state = vw_save_game_state_callback;
   cb.free_buffer     = vw_free_buffer;
   cb.on_event        = vw_on_event_callback;
   cb.log_game_state  = vw_log_game_state;

#if defined(SYNC_TEST)
   ggpo_start_synctest(&ggpo, &cb, "vectorwar", num_players, sizeof(int), 1);
#else
   ggpo_start_session(&ggpo, &cb, "vectorwar", num_players, sizeof(int), localport);
#endif

   ggpo_set_disconnect_timeout(ggpo, 30000);
   ggpo_set_disconnect_notify_start(ggpo, 10000);

   for (int i = 0; i < num_players + num_spectators; i++) {
      GGPOPlayerHandle handle;
      ggpo_add_player(ggpo, players + i, &handle);
      ngs.players[i].handle = handle;
      ngs.players[i].type = players[i].type;
      if (players[i].type == GGPO_PLAYERTYPE_LOCAL) {
         ngs.players[i].connect_progress = 100;
         ngs.local_player_handle = handle;
         ngs.SetConnectState(handle, Connecting);
         ggpo_set_frame_delay(ggpo, handle, FRAME_DELAY);
      } else {
         ngs.players[i].connect_progress = 0;
      }
   }

   ggpoutil_perfmon_init(widget);
   renderer->SetStatusText("Connecting to peers.");
}

void VectorWar_DisconnectPlayer(int player) {
   if (player < ngs.num_players) {
      GGPOErrorCode result = ggpo_disconnect_player(ggpo, ngs.players[player].handle);
      if (GGPO_SUCCEEDED(result)) {
         renderer->SetStatusText("Disconnected player.");
      } else {
         renderer->SetStatusText("Error while disconnecting player.");
      }
   }
}

void VectorWar_DrawCurrentFrame() {
   if (renderer) {
      renderer->Draw(gs, ngs);
   }
}

void VectorWar_AdvanceFrame(int inputs[], int disconnect_flags) {
   gs.Update(inputs, disconnect_flags);
   ngs.now.framenumber = gs._framenumber;
   ngs.now.checksum = fletcher32_checksum((short *)&gs, sizeof(gs) / 2);
   if ((gs._framenumber % 90) == 0) {
      ngs.periodic = ngs.now;
   }
   ggpo_advance_frame(ggpo);
   ggpoutil_perfmon_update(ggpo, nullptr, 0);
}

/*
 * ReadInputs --
 *
 * Read the inputs for player 1 from the keyboard.  We never have to
 * worry about player 2.  GGPO will handle remapping his inputs 
 * transparently.
 */
int ReadInputs() {
    int inputs = 0;

   if (input_state.up_pressed) {
        inputs |= VectorWarInputs::INPUT_THRUST;
    }
    if (input_state.down_pressed) {
        inputs |= VectorWarInputs::INPUT_BREAK;
    }
    if (input_state.left_pressed) {
        inputs |= VectorWarInputs::INPUT_ROTATE_LEFT;
    }
    if (input_state.right_pressed) {
        inputs |= VectorWarInputs::INPUT_ROTATE_RIGHT;
    }
    if (input_state.fire_pressed) {
        inputs |= VectorWarInputs::INPUT_FIRE;
    }
    if (input_state.bomb_pressed) {
        inputs |= VectorWarInputs::INPUT_BOMB;
    }

    return inputs;
}

void VectorWar_RunFrame() {
   GGPOErrorCode result = GGPO_OK;
   int disconnect_flags;
   int inputs[MAX_SHIPS] = { 0 };

   if (ngs.local_player_handle != GGPO_INVALID_HANDLE) {
      int input = ReadInputs();
#if defined(SYNC_TEST)
      input = rand(); // test: use random inputs to demonstrate sync testing
#endif
      result = ggpo_add_local_input(ggpo, ngs.local_player_handle, &input, sizeof(input));
   }

  if (GGPO_SUCCEEDED(result)) {
     result = ggpo_synchronize_input(ggpo, (void *)inputs, sizeof(int) * MAX_SHIPS, &disconnect_flags);
     if (GGPO_SUCCEEDED(result)) {
         // inputs[0] and inputs[1] contain the inputs for p1 and p2.  Advance
         // the game by 1 frame using those inputs.
         VectorWar_AdvanceFrame(inputs, disconnect_flags);
     }
  }

   VectorWar_DrawCurrentFrame();
}

void VectorWar_Idle(int time) {
   ggpo_idle(ggpo, time);
}

void VectorWar_Exit() {
   memset(&gs, 0, sizeof(gs));
   memset(&ngs, 0, sizeof(ngs));

   if (ggpo) {
      ggpo_close_session(ggpo);
      ggpo = nullptr;
   }

   delete renderer;
   renderer = nullptr;
}
