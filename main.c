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
volatile int headward[MAX_X][MAX_Y];
volatile int tailward[MAX_X][MAX_Y];
volatile int head;
volatile int tail;
volatile int apple;
volatile int dir_x = 1, dir_y = 0;
volatile int length;
volatile int STAT;

//Variables to manage the speed of the game
volatile int WAIT = 500;
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
int  getHeadward(int coord)                      {return headward[getX(coord)][getY(coord)];}
void setHeadward(int x, int y, int coord)        {headward[x][y] = coord;}
void setHeadwardCoord(int atCoord, int toCoord)  {headward[getX(atCoord)][getY(atCoord)] = toCoord;}
int  getTailward(int coord)                      {return tailward[getX(coord)][getY(coord)];}
void setTailward(int x, int y, int coord)        {tailward[x][y] = coord;}
void setTailwardCoord(int atCoord, int toCoord)  {tailward[getX(atCoord)][getY(atCoord)] = toCoord;}

xSemaphoreHandle lcdLock;

void test(){
	clearScreen();
}
void test_left(){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
  GLCD_bitmap(100, 100, 16, 16, sprite_body);
	xSemaphoreGive(lcdLock);
}
void test_right(){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	GLCD_bitmap(50, 50, 16, 16, sprite_apple);
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


//X and Y refers to columns and rows, not pixel.
extern void printCharXY(int x, int y, char c){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	GLCD_displayChar(y * SPRITE_DIM_Y, SCREEN_WIDTH - (SPRITE_DIM_X * (x+1)), c);
	xSemaphoreGive(lcdLock);
}


//void printSpriteXY(int x, int y, unsigned short * sprite) {
//	xSemaphoreTake(lcdLock, portMAX_DELAY);

//	GLCD_bitmap(SCREEN_WIDTH - (SPRITE_DIM_X * (x+1)),
//							y * SPRITE_DIM_Y,
//							SPRITE_DIM_X,
//							SPRITE_DIM_Y,
//							sprite);
//	
//	xSemaphoreGive(lcdLock);
//}

void updateScreen(int coord){
		
//	switch (getContent(coord)) {
//		case BODY  : 
//				printSpriteXY(getX(coord), getY(coord), sprite_body);
//				break;
//		case TAIL  : 
//				printSpriteXY(getX(coord), getY(coord), sprite_body);
//				break;
//		case HEAD  :
//				printSpriteXY(getX(coord), getY(coord), sprite_head);
//				break;
//		case APPLE :
//				printSpriteXY(getX(coord), getY(coord), sprite_apple);
//				break;
//		default    : 
//				printSpriteXY(getX(coord), getY(coord), sprite_empty);
//	}
		printCharXY(getX(coord), getY(coord), getContent(coord));  

//	if (getContent(coord)==APPLE)
//		//printSpriteXY(getX(coord), getY(coord), sprite_apple);
//	if (getContent(coord)==HEAD)
//		printSpriteXY(getX(coord), getY(coord), sprite_head);

	LED_out(1<<3);
}

static void ledOn(int led) {
	LED_out(led);
}

void clearScreen() {
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	GLCD_clear(White);
	GLCD_setTextColor(Black);
	xSemaphoreGive(lcdLock);
}

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
		ledOn(1<<t);
//		t = (t+1)%4;
	  vTaskDelay(300 / portTICK_RATE_MS);
  }
}

//Status codes to show when exiting game.
void exitGame(int stat) {
	STAT = stat;
  //gotoxy(0, MAX_Y+3);
  printf("%03d: ", stat);
  switch (stat) {
    case ERR       : printf("Some fatal error occurred"); break;
    case WIN       : printf("Congratulations! You won!"); break;
    case GAME_OVER : printf("Uh oh! Game Over!");         break;
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
  if (getTailward(pos) && getHeadward(pos))
    printf("h(%2d,%2d) '%c':(%2d, %2d) t(%2d,%2d)\n", 
	   getX(getHeadward(pos)), getY(getHeadward(pos)), getContent(pos), getX(pos), getY(pos), getX(getTailward(pos)), getY(getTailward(pos))); 
  else if (getHeadward(pos))
    printf("h(%2d,%2d) '%c':(%2d, %2d) t(--,--)\n", 
	   getX(getHeadward(pos)), getY(getHeadward(pos)), getContent(pos), getX(pos), getY(pos));
  else if (getTailward(pos))
    printf("h(--,--) '%c':(%2d, %2d) t(%2d,%2d)\n", 
	   getContent(pos), getX(pos), getY(pos),  getX(getTailward(pos)), getY(getTailward(pos)));
  else
    printf("h(--,--) '%c':(%2d, %2d) t(--,--)\n", 
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
  int x, y; 

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
  updateScreen(apple);
}

//Initialise the board and snake.
void initiate() {
  int i,j, pos = head;

  //Clean/Reset board array
  for (i=0;i<MAX_Y;i++)
    for (j=0;j<MAX_X;j++){
      setContent(setXY(j,i), EMPTY);
      setHeadward(j,i,0);
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
  do {
    setHeadwardCoord(pos, setXY(i+1,0));
    pos = getHeadward(pos);
    i++;
  } while (pos != head);
  
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

xQueueHandle printQueue;

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
      generateApple();     
      setHeadwardCoord(headOld, head);
      setContent(headOld, BODY);
      setContent(head, HEAD);
      setTailwardCoord(head, headOld);
      break;
    case BODY  : exitGame(GAME_OVER);
    case TAIL  :
    case EMPTY :
      tail = getHeadward(tail);
      setContent(tailOld, EMPTY);
      setHeadwardCoord(tailOld, 0);
      setContent(headOld, BODY);
      setHeadwardCoord(headOld, head);
      setContent(tail, TAIL);
      setTailwardCoord(tail, 0);
      setContent(head, HEAD);
      setTailwardCoord(head, headOld);
      break;
    default    :
			exitGame(__LINE__);	
			printBoardPos(head);
  }
  updateScreen(headOld);
  updateScreen(head);
	printBoardPos(head);
  updateScreen(tailOld);
  updateScreen(tail);
}


/* Retarget printing to the serial port 1 */
int fputc(int ch, FILE *f) {
  unsigned char c = ch;
  xQueueSend(printQueue, &c, 0);
  return ch;
}


/*-----------------------------------------------------------*/

void wait(int delayMS){
	vTaskDelay(delayMS / portTICK_RATE_MS);
}

void autoMoveTask(void *params){
	tick();
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
	while(STAT == RUNNING){
		tick();
		ledOn(1<<2);
		wait(WAIT);
	}
	
	while (1) {;};
}
void Joy_stick(void *params){
	
	volatile JOY_State_TypeDef state;
	
	for(;;){
	 state = JOY_NONE;
	state =  IOE_JoyStickGetState();
	ledOn(0);
	switch (state) {
		case JOY_NONE:	
			break;
		case JOY_UP:
			ledOn(8);	
			up();
			test_up();
			break;
		case JOY_DOWN:
			ledOn(4);	
			down();
			test_down();
			break;
		case JOY_LEFT:
			ledOn(1);	
			left();
			test_left();
			break;
		case JOY_RIGHT:
			ledOn(2);	
			right();
			test_right();
			break;
		case JOY_CENTER:
			ledOn(15);	
			test();
			tick();
			break;

		default:
			break;	
	}
	ledOn(1<<1);

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

  printQueue = xQueueCreate(128, 1);
//	Joy_stick();
  initDisplay();
  initiate();
  
	xTaskCreate(ledTask, "lcd", 100, NULL, 1, NULL);
  xTaskCreate(printTask, "print", 100, NULL, 1, NULL);
  xTaskCreate(Joy_stick, "joy", 100, NULL, 1, NULL);
  xTaskCreate(autoMoveTask, "tick", 100, NULL, 1, NULL);
  
	//xTaskCreate(touchScreenTask, "touchScreen", 100, NULL, 1, NULL);
  //xTaskCreate(highlightButtonsTask, "highlighter", 100, NULL, 1, NULL);

  printf("Setup complete ");  // this is redirected to the display

  vTaskStartScheduler();

  assert(0);
  return 0;                 // not reachable
}

/*-----------------------------------------------------------*/
