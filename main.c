/**
 * Program skeleton for the course "Programming embedded systems"
 */

#include <stdio.h>
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "setup.h"
#include "assert.h"

#include "GLCD.h"
#include "stm3210c_eval_ioe.h"
#include "defines.h"
/*-----------------------------------------------------------*/

//Global variables from board.
volatile int content[MAX_X][MAX_Y];
//volatile int headward[MAX_X][MAX_Y];
volatile int tailward[MAX_X][MAX_Y];
volatile int head;
volatile int tail;
volatile int tail2;
volatile int apple;
volatile int dir_x = 1, dir_y = 0;
volatile int length;
volatile int STAT;

//Variables to manage the speed of the game
volatile int WAIT = 250;
const int timeStep = 10;

//Functions to set directions of the snake movement.
void up()   { if (dir_x) { dir_x =  0; dir_y =  1; }}
void down() { if (dir_x) { dir_x =  0; dir_y = -1; }}
void left() { if (dir_y) { dir_x = -1; dir_y =  0; }}
void right(){ if (dir_y) { dir_x =  1; dir_y =  0; }}

//Functions to manage board
int  getX(int coord)                             {return (coord / 100);}
int  getY(int coord)                             {return (coord % 100);}
int  setX(int coord, int x)                      {return x*100 + getY(coord);}
int  setY(int coord, int y)                      {return getX(coord)*100 + y;}
int  setXY(int x, int y)                         {return x*100+y;}
int  getContent(int coord)                       {return content[getX(coord)][getY(coord)];}
void setContent(int coord, char cont)            {content[getX(coord)][getY(coord)] = cont;}
//int  getHeadward(int coord)                      {return headward[getX(coord)][getY(coord)];}
//void setHeadward(int x, int y, int coord)        {headward[x][y] = coord;}
//void setHeadwardCoord(int atCoord, int toCoord)  {headward[getX(atCoord)][getY(atCoord)] = toCoord;}
int  getTailward(int coord)                      {return tailward[getX(coord)][getY(coord)];}
void setTailward(int x, int y, int coord)        {tailward[x][y] = coord;}
void setTailwardCoord(int atCoord, int toCoord)  {tailward[getX(atCoord)][getY(atCoord)] = toCoord;}
int  getTailnew(int head){
	int i;
	int pos = head;
	for (i = 0; i<length; i++){
		pos = getTailward(pos);
	}
	return pos;
}


xSemaphoreHandle lcdLock;

void clearScreen() {
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	GLCD_clear(White);
	GLCD_setTextColor(Black);
	xSemaphoreGive(lcdLock);
}

void test(){
	clearScreen();
}
/*
void test_left(){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	//GLCD_bitmap(100, 100, 16, 16, sprite_body);
	xSemaphoreGive(lcdLock);
}
void test_right(){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	//GLCD_bitmap(50, 50, 16, 16, sprite_apple);
	xSemaphoreGive(lcdLock);
}
void test_up(){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	//GLCD_bitmap(150, 150, 16, 16, sprite_apple);
	xSemaphoreGive(lcdLock);
}
void test_down(){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
//	GLCD_bitmap(200, 200, 16, 16, sprite_head);
	xSemaphoreGive(lcdLock);
}
*/

//X and Y refers to columns and rows, not pixel.
extern void printCharXY(int x, int y, char c){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	GLCD_displayChar(y * SPRITE_DIM_Y, SCREEN_WIDTH - (SPRITE_DIM_X * (x+1)), c);
	xSemaphoreGive(lcdLock);
}


void printSpriteXY(int x, int y, unsigned short * sprite) {
	int i, j, k = 0;
//	xSemaphoreTake(lcdLock, portMAX_DELAY);

	for (j=(y * SPRITE_DIM_Y); j<((y * SPRITE_DIM_Y) + SPRITE_DIM_Y);j++ ){
		for (i=(x * SPRITE_DIM_X); i<((x * SPRITE_DIM_X) + SPRITE_DIM_X);i++ ){
			GLCD_setTextColor(sprite[k++]);
			GLCD_putPixel(j, SCREEN_WIDTH - i-1);
			//vTaskDelay(5 / portTICK_RATE_MS);
		}
	}
	GLCD_setTextColor(Black);
	xSemaphoreGive(lcdLock);
}

void updateScreen(int coord){
	switch (getContent(coord)) {
		case BODY  : 
				//printSpriteXY(getX(coord), getY(coord), sprite_body);
				printCharXY(getX(coord), getY(coord), getContent(coord));  
				break;
		case TAIL  : 
				//printSpriteXY(getX(coord), getY(coord), sprite_body);
				printCharXY(getX(coord), getY(coord), getContent(coord));  
				break;
		case HEAD  :
				printCharXY(getX(coord), getY(coord), getContent(coord));  
				//printSpriteXY(getX(coord), getY(coord), sprite_head);
				break;
		case APPLE :
				//printCharXY(getX(coord), getY(coord), getContent(coord));  
				printSpriteXY(getX(coord), getY(coord), sprite_apple);
				//printf("(%2d, %2d)\n", getX(coord), getY(coord) );
				break;
		default    : 
				//printSpriteXY(getX(coord), getY(coord), sprite_empty);
				printCharXY(getX(coord), getY(coord), getContent(coord));  
	}
	

//		printCharXY(getX(coord), getY(coord), getContent(coord));  

//	if (getContent(coord)==APPLE)
//		//printSpriteXY(getX(coord), getY(coord), sprite_apple);
//	if (getContent(coord)==HEAD)
//		printSpriteXY(getX(coord), getY(coord), sprite_head);

//	LED_out(1<<3);
}

//static void ledOn(int led) {
//	LED_out(led);
//}



static void initDisplay () {
  /* LCD Module init */
  lcdLock = xSemaphoreCreateMutex();
  GLCD_init();
	clearScreen();
	xSemaphoreGive(lcdLock);
}

static void ledTask(void *params) {
  unsigned short t = 0;

  for (;;) {
		LED_out(1<<t);
		t = (t+1)%4;
	  vTaskDelay(100 / portTICK_RATE_MS);
  }
}

//Status codes to show when exiting game.
void exitGame(int stat) {
	STAT = stat;
  //gotoxy(0, MAX_Y+3);
  //printf("%03d: ", stat);
  switch (stat) {
    case ERR       : printf("Some fatal error occurred"); break;
    case WIN       : printf("Congratulations! You won!"); break;
    case GAME_OVER :
			vTaskDelay(50/portTICK_RATE_MS);
			clearScreen();
			GLCD_displayStringLn(Line4, "   Game Over!");       
			break;
		case COMPLETE  : printf("End of program.");           break;
    default        : printf("LINE");
  }
	dir_x = 0;
	dir_y = 0;
  //exit(stat);
}

//Print information about the board
void printBoardPos(int pos) {
  //gotoxy(0, MAX_Y+2);
	if (getTailward(pos))
    printf("'%c':(%2d, %2d) t(%2d,%2d)\n", 
	   getContent(pos), getX(pos), getY(pos),  getX(getTailward(pos)), getY(getTailward(pos)));
  else
    printf("'%c':(%2d, %2d) t(--,--)\n", 
	   getContent(pos), getX(pos), getY(pos));
}

//Print the snake on the screen
void printSnake() {
  int pos;
  for (pos = head; pos; pos = getTailward(pos))
    updateScreen(pos);   
}

//(Re-)print the contents of the full board (Full Update)
void printBoard(){
  int i, j;
  clearScreen();
  for (i=0;i<MAX_Y;i++)
    for (j=0;j<MAX_X;j++)
      updateScreen(setXY(j,i));
}

//Generate Apple and update the screen with new position
void generateApple() {
  uint16_t x, y; 

  //If the board is full, i.e. there is no place left to continue.
  if (length >= MAX_X * MAX_Y)
    exitGame(WIN);

  //Find an empty slot to put the apple
  while (1) {
    x = rand() % MAX_X;
    y = rand() % MAX_Y;
    if (getContent(setXY(x,y)) == EMPTY)
      break;
  }
	
  //Update the variable with the new position
  apple = setXY(x,y);
  setContent(apple, APPLE);
	
	WAIT = WAIT - WAIT*timeStep/100;
	
	//printf("(%2d, %2d)\n", x, y);
  updateScreen(apple);
}

//Initialise the board and snake.
void initiate() {
  int i,j, pos = head;

  //Clean/Reset board array
  for (i=0;i<MAX_Y;i++)
    for (j=0;j<MAX_X;j++){
      setContent(setXY(j,i), EMPTY);
      //setHeadward(j,i,0);
      setTailward(j,i,0);
    }
 
  //Assign Snake of variable length;
  length = LENGTH_START;
  i = length -1;
  head = setXY(i,0);
  tail = setXY(0,0);
  
  //Set the trailing pointers
  do {
    setContent(pos, BODY);
    setTailward(getX(pos), getY(pos), setXY(i-1,0));
    pos = getTailward(pos);
    i--;
  } while (pos != tail);
  pos = tail;
  
  //Set the preceding pointers
  /*do {
    setHeadwardCoord(pos, setXY(i+1,0));
    pos = getHeadward(pos);
    i++;
  } while (pos != head);*/
  
  //Enter content;
  setContent(head, HEAD);
  setContent(tail, TAIL);
 
  //Generate Apple
  srand(NULL);
  generateApple();
  
  //Set starting direction
  right();
  
  //Clear screen and print board;
  printBoard();
  //drawWalls(); //Only on windows
	
	STAT = RUNNING;
}



/*-----------------------------------------------------------*/

/**
 * Display stdout on the display
 */

//xQueueHandle printQueue;
/*
static void printTask(void *params) {
  unsigned char str[21] = "                    ";
  portTickType lastWakeTime = xTaskGetTickCount();
  int i;

  for (;;) {
    xSemaphoreTake(lcdLock, portMAX_DELAY);
    GLCD_setTextColor(Black);
    GLCD_displayStringLn(Line9, str);
    xSemaphoreGive(lcdLock);

    for (i = 0; i < 19; ++i)
	  str[i] = str[i+1];

    if (!xQueueReceive(printQueue, str + 19, 0))
	  str[19] = ' ';

	vTaskDelayUntil(&lastWakeTime, 100 / portTICK_RATE_MS);
  }
}
*/

//Actual game logic for each step forward taken.
void tick(){ 
  int headOld = head, tailOld = tail;
  int mx = MAX_X, my = MAX_Y;
	int new_x, new_y;
	new_x = (getX(head)+ dir_x)%mx;
	if(new_x < 0)
		new_x = mx + new_x;
	new_y = (getY(head)- dir_y)%my;
	if(new_y < 0)
		new_y = my + new_y;
  head = setXY(new_x, new_y);
	
	
  switch (getContent(head)){
    case APPLE :
      length++;
      //setHeadwardCoord(headOld, head);
      setContent(headOld, BODY);
      setContent(head, HEAD);
      setTailwardCoord(head, headOld);
      generateApple();     
			updateScreen(headOld);
			updateScreen(apple);
      break;
    case BODY  : exitGame(GAME_OVER);
    case TAIL  :
    case EMPTY :
      setContent(tailOld, EMPTY);
      setContent(headOld, BODY);
      //setHeadwardCoord(tailOld, 0);
      //setHeadwardCoord(headOld, head);
      setContent(head, HEAD);
      setTailwardCoord(head, headOld);
      tail = getTailnew(head);
      setContent(tail, TAIL);
      setTailwardCoord(tail, 0);
      break;
    default    :
			exitGame(__LINE__);	
//			printBoardPos(head);
  }
	//updateScreen(headOld);
  //updateScreen(head);
	//printBoardPos(head);
  //updateScreen(tail);
	
	printSnake();
	updateScreen(apple);
	updateScreen(tailOld);
 
}


/* Retarget printing to the serial port 1 */
int fputc(int ch, FILE *f) {
  unsigned char c = ch;
//  xQueueSend(printQueue, &c, 0);
  return ch;
}


/*-----------------------------------------------------------*/

void wait(int delayMS){
	vTaskDelay(delayMS / portTICK_RATE_MS);
}



typedef struct {
  u16 lower, upper, left, right;
  void *data;
  void (*callback)(u16 x, u16 y, u16 pressure, void *data);
} TSCallback;

static TSCallback callbacks[5];
static u8 callbackNum = 0;


static void touchScreenTask(void *params) {
  portTickType lastWakeTime = xTaskGetTickCount();
  TS_STATE *ts_state;
  u8 pressed = 0;
  u8 i;

  for (;;) {
    ts_state = IOE_TS_GetState();
  
	if (pressed) {
	  if (!ts_state->TouchDetected)
	    pressed = 0;
	} else if (ts_state->TouchDetected) {
	/*  for (i = 0; i < callbackNum; ++i) {
		if (callbacks[i].left  <= ts_state->X &&
		    callbacks[i].right >= ts_state->X &&
		    callbacks[i].lower >= ts_state->Y &&
		    callbacks[i].upper <= ts_state->Y)
		  callbacks[i].callback(ts_state->X, ts_state->Y, ts_state->Z,
		                        callbacks[i].data);
	  }		*/											
	  pressed = 1;
	}

    if (ts_state->TouchDetected) {
			//Move upwards
			if((ts_state->Y >= 0) && (ts_state->Y <= SCREEN_HEIGHT/2) && 
			(ts_state->X >= SCREEN_WIDTH/2 - MARGIN) && (ts_state->X <=SCREEN_WIDTH/2 + MARGIN) ){
			   up();
				LED_out(8);
			 vTaskDelay(100);
			}
		//Move downwards
		if((ts_state->Y > SCREEN_HEIGHT/2) && (ts_state->Y <= SCREEN_HEIGHT) && 
			(ts_state->X >= SCREEN_WIDTH/2 - MARGIN) && (ts_state->X <= SCREEN_WIDTH/2 + MARGIN) ){
				 down();	
			//	LED_out(4);
			//  vTaskDelay(100);
			}
				// Move Right
		if((ts_state->Y >= 0) && (ts_state->Y <= SCREEN_HEIGHT && 
			 (ts_state->X >= SCREEN_WIDTH/2 + MARGIN) && (ts_state->X <=SCREEN_WIDTH))){
			   right();	
			//	LED_out(2);
			//  vTaskDelay(100);
		}
		// Move Left
		if((ts_state->Y >= 0) && (ts_state->Y <= SCREEN_HEIGHT && (ts_state->X >=0) && (ts_state->X < SCREEN_WIDTH/2 - MARGIN) )){
			  left();
			//	LED_out(12);
			//  vTaskDelay(100);
		}
			LED_out(0);
			
			
	 // printf("x = %d, y= %d ", ts_state->X, ts_state->Y);
	}

	vTaskDelayUntil(&lastWakeTime, 100 / portTICK_RATE_MS);
  }
}
	
	
	
void autoMoveTask(void *params){
	int i;

	while(0){
		if(!getX(head)) {
			down();
			tick();
			wait(WAIT);
			right();
		}

		if(getX(head) == MAX_X-1) {
			down();
			tick();
		  wait(WAIT);
			left();
		}
		tick();
		wait(WAIT);
	}

	/*if (0){
		for (i = 3; i<12; i++) {tick(); wait(WAIT/3);}
		down();
		for (i = 1; i<10; i++) {tick(); wait(WAIT/3);}
		tick();
		left();
		tick();
		up();
		for (i = 0; i<6; i++) {tick(); wait(WAIT/1);}
		left();
		for (i = 0; i<5; i++) {tick(); wait(WAIT/1);}
		down();
		wait(WAIT*2);
	}*/
	
	while(STAT == RUNNING){
		tick();
		//ledOn(1<<2);
		wait(WAIT);
		//printCharXY(0,0,'#');
	}
	
	while (1) {;};
}

void Joy_stick(void *params){
	
	volatile JOY_State_TypeDef state;
	
	for(;;){
	state = JOY_NONE;
	state =  IOE_JoyStickGetState();
	LED_out(0);
	switch (state) {
		case JOY_NONE:	
			break;
		case JOY_UP:
			LED_out(8);	
			up();
			//test_up();
			break;
		case JOY_DOWN:
			LED_out(4);	
			down();
		//	test_down();
			break;
		case JOY_LEFT: 
			LED_out(1);	
			left();
		//	test_left();
			break;
		case JOY_RIGHT:
			LED_out(2);	
			right();
		//	test_right();
			break;
		case JOY_CENTER:
			LED_out(15);	
			test();
			//tick();
			break;

		default:
			break;	
	}
//	ledOn(1<<1);

	vTaskDelay(50 / portTICK_RATE_MS);
}
}


/*-----------------------------------------------------------*/

/*
 * Entry point of program execution
 */
int main( void )
{
  prvSetupHardware();
  IOE_Config();

  //printQueue = xQueueCreate(128, 1);
//	Joy_stick();
  initDisplay();
  initiate();
  
//	xTaskCreate(ledTask, "lcd", 100, NULL, 1, NULL);
  //xTaskCreate(printTask, "print", 100, NULL, 1, NULL);
    xTaskCreate(Joy_stick, "joy", 100, NULL, 1, NULL);
    xTaskCreate(autoMoveTask, "tick", 100, NULL, 1, NULL);
    xTaskCreate(touchScreenTask, "touchScreen", 100, NULL, 1, NULL);
  //xTaskCreate(highlightButtonsTask, "highlighter", 100, NULL, 1, NULL);

  //printf("%d/%d", MAX_X, MAX_Y);  // this is redirected to the display

  vTaskStartScheduler();

  assert(0);
  return 0;                 // not reachable
}

/*-----------------------------------------------------------*/
