#ifndef _VECTORWAR_H
#define _VECTORWAR_H

#include "../../include/ggponet.h"
#include <gtk/gtk.h>

/*
 * vectorwar.h --
 *
 * Interface to the vector war application.
 *
 */

enum VectorWarInputs {
   INPUT_THRUST            = (1 << 0),
   INPUT_BREAK             = (1 << 1),
   INPUT_ROTATE_LEFT       = (1 << 2),
   INPUT_ROTATE_RIGHT      = (1 << 3),
   INPUT_FIRE              = (1 << 4),
   INPUT_BOMB              = (1 << 5),
};
// Structure to track the current state of key inputs
typedef struct {
    bool up_pressed;
    bool down_pressed;
    bool left_pressed;
    bool right_pressed;
    bool fire_pressed;
    bool bomb_pressed;
} InputState;

// Global variable to store the current input state
extern InputState input_state;

void VectorWar_Init(GtkWidget *widget, unsigned short localport, int num_players, GGPOPlayer *players, int num_spectators);
void VectorWar_InitSpectator(GtkWidget *widget, unsigned short localport, int num_players, char *host_ip, unsigned short host_port);
void VectorWar_DrawCurrentFrame();
void VectorWar_AdvanceFrame(int inputs[], int disconnect_flags);
void VectorWar_RunFrame();
void VectorWar_Idle(int time);
void VectorWar_DisconnectPlayer(int player);
void VectorWar_Exit();

#define ARRAY_SIZE(n)      (sizeof(n) / sizeof(n[0]))
#define FRAME_DELAY        2

#endif
