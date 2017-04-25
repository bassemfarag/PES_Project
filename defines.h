//Values for Sprites
#define EMPTY           ' '
#define HEAD            'O'
#define BODY            'o'
#define TAIL             '.'
#define APPLE           '#'

//?
#define BORDER_V            11

//Screen Definitions
#define SCREEN_WIDTH        320
#define SCREEN_HEIGHT       240
#define SPRITE_DIM          24
#define VERT                SCREEN_WIDTH  / SPRITE_DIM
#define HOR                 SCREEN_HEIGHT / SPRITE_DIM
#define MAX_X               SCREEN_WIDTH  / 16
#define MAX_Y               (SCREEN_HEIGHT / 24)-1

//Snake start Length
#define LENGTH_START        2

//Directions
#define UP                  101
#define DOWN                102
#define LEFT                103
#define RIGHT               104

//Status
#define INIT                201
#define RUNNING             202
#define APPLE_CAUGHT        203
#define WALL                204
#define GAME_OVER           205
#define COMPLETE            206
#define ERROR               207
#define WIN                 208


//Sprites
//#include "Sprites/blob.h"


