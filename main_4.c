/**
 * Program skeleton for the course "Programming embedded systems"
 */

#include <stdio.h>
#include <stdlib.h>
//#include <time.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "setup.h"
#include "assert.h"

#include "GLCD.h"
#include "stm3210c_eval_ioe.h"
#include <defines.h>

/*----------------------------------------------------------*/

volatile int dir_y = 0;
volatile int dir_x = 0;
volatile int length = 3;
volatile int status = INIT;

volatile int WAIT = 30;
const int timeStep = 10;

xSemaphoreHandle lcdLock;

BoardPosition board[MAX_X][MAX_Y];
BoardPosition* head;
BoardPosition* tail;
BoardPosition* apple;



void up(){
  dir_y = 1;
  dir_x = 0;
}

void down(){
  dir_y = -1;
  dir_x = 0;
}

void left(){
  dir_y = 0;
  dir_x = -1;
}

void right(){
  dir_y = 0;
  dir_x = 1;
}


void clearScreen() {
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	GLCD_clear(White);
	GLCD_setTextColor(Black);
	xSemaphoreGive(lcdLock);
}

void exitGame(int stat) {
  
  /*printf("%03d: ", stat);
  switch (stat) {
    case ERROR :
      printf("Some fatal error occurred");
      break;
     
    case WIN :
      printf("Congratulations! You won!");
      break;
     
    case GAME_OVER :
      printf("Uh oh! Game Over!");
      break;
      
    case COMPLETE :
      printf("End of program.");
      break;
    }
  
  exit(stat);
  return;*/
}


//X and Y refers to columns and rows, not pixel.
void printCharXY(int x, int y, char c){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	GLCD_displayChar(y * 24, SCREEN_WIDTH - (16 * (x+1)), c);
	xSemaphoreGive(lcdLock);
}

void updateScreen(BoardPosition* pos){
	printCharXY(pos->x, pos->y, pos->content);  
}

void generateApple() {
  int x, y, complete = 0;

  if (length >= MAX_X * MAX_Y){
    exitGame(WIN);
  }

  while (!complete) {
    x = rand () % MAX_X;
    y = rand () % MAX_Y;

    if (board[x][y].content == EMPTY)
      complete = 1;
  }


  apple = &board[x][y];

  //exitGame(__LINE__);
  apple->content = APPLE;

  //printBoardPos(apple);
  updateScreen(apple);

  return;
}

static void ledOn(int led) {
	LED_out(led);
	/*switch (led) {
		case 1:
			LED_out(0x01);
			break;
		case 2:
			LED_out(0x02);
			break;
		case 3:
			LED_out(0x04);
			break;
		case 4:
			LED_out(0x08);
			break;
		default:
			LED_out(0x00);
			break;
	}*/
}


static void initDisplay () {
  /* LCD Module init */

  lcdLock = xSemaphoreCreateMutex();
	ledOn(1);
  GLCD_init();
  GLCD_clear(White);
  GLCD_setTextColor(Blue);
  GLCD_displayStringLn(Line2, " Programming");
  GLCD_displayStringLn(Line3, " Embedded");
  GLCD_displayStringLn(Line4, " Systems");
}


/*-----------------------------------------------------------*/



void printSnake() {
  BoardPosition* pos = head;

  printf("%d: ", length);
  while (pos != NULL) {
    updateScreen(pos);
    pos = pos->tailward;
  }

  //printf("\r\n");
  return;
}

void printBoard(){
  int i = 0,j = 0;
  //char out[MAX_Y*2];
  //char temp[4];

  //printSnake();
  clearScreen();
	
 // printf("-------------------\n");
  for (i=0;i<MAX_Y;i++){
    //printf("|");
    for (j=0;j<MAX_X;j++){
			printCharXY(j,i, board[j][i].content);
      //printf("%c", board[j][i].content);
    };
    //printf("|\n");
  };

  //printf("-----------------^%02d\n", length);
  return;
}

void test(){
	printCharXY(0,0, HEAD);
	printCharXY(0,1, BODY);
	printCharXY(1,0, TAIL);
	printCharXY(1,1, APPLE);
	vTaskDelay(2000 / portTICK_RATE_MS);
	
	vTaskDelay(2000 / portTICK_RATE_MS);
	
	GLCD_setTextColor(Black);
	
	printCharXY(0,0, HEAD);
	printCharXY(0,1, BODY);
	printCharXY(1,0, TAIL);
	printCharXY(1,1, APPLE);

	vTaskDelay(2000 / portTICK_RATE_MS);

	printCharXY(0,0, EMPTY);
	printCharXY(0,1, EMPTY);
	printCharXY(1,0, EMPTY);
	printCharXY(1,1, EMPTY);	
}

void initiate() {
  int i,j;
    BoardPosition* pos;

  //Clean board array
  for (i=0;i<MAX_Y;i++) {
    for (j=0;j<MAX_X;j++){
      board[j][i].x = j;
      board[j][i].y = i;
      board[j][i].content = EMPTY;
      board[j][i].headward = NULL;
      board[j][i].tailward = NULL;
    }
  }

  //Assign Snake of variable length;
  length = LENGTH_START;
  i = length -1;
  head = &board[i][0];
  tail = &board[0][0];
  pos = head;
  
  do {
    pos->content = BODY;
    pos->tailward = &board[i-1][0];
    pos = pos->tailward;
    i--;
  } while (pos != tail);
  pos = tail;

  do {
    pos->headward = &board[i+1][0];
    pos = pos->headward;
    i++;
  } while (pos != head);

  head->content = HEAD;
  tail->content = TAIL;

  //Generate Apple
  srand(5);
  generateApple();
  
  //Set starting direction
  right();
  
  //Clear screen and print board;
  printBoard();
}

void Joy_stick(void *params){
	
	volatile JOY_State_TypeDef state;
	
	for(;;){
	 state = JOY_NONE;
	//state =  IOE_JoyStickGetState();
	state =  IOE_JoyStickGetState();
	ledOn(7);
	switch (state) {
		case JOY_NONE:	
			break;
		case JOY_UP:
			ledOn(8);	
			up();
			break;
		case JOY_DOWN:
			ledOn(4);	
			down();
			break;
		case JOY_LEFT:
			ledOn(1);	
			left();
			break;
		case JOY_RIGHT:
			ledOn(2);	
			right();
			break;
		case JOY_CENTER:
			ledOn(15);	
			test();
			break;

		default:
			break;	
	}
	vTaskDelay(10 / portTICK_RATE_MS);
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

//	Joy_stick();
  initDisplay();
	//initiate();
  //xTaskCreate(lcdTask, "lcd", 100, NULL, 1, NULL);
  //xTaskCreate(printTask, "print", 100, NULL, 1, NULL);
  //xTaskCreate(Joy_stick, "led", 100, NULL, 1, NULL);
  //xTaskCreate(touchScreenTask, "touchScreen", 100, NULL, 1, NULL);
  //xTaskCreate(highlightButtonsTask, "highlighter", 100, NULL, 1, NULL);


  vTaskStartScheduler();

  assert(0);
  return 0;                 // not reachable
}

/*-----------------------------------------------------------*/
