/**
 * Program skeleton for the course "Programming embedded systems"
 */

#include <stdio.h>

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

/*-----------------------------------------------------------*/

volatile int dir_y = 0;
volatile int dir_x = 0;
volatile int length = 3;
volatile int status = INIT;

volatile int WAIT = 30;
const int timeStep = 10;

//Board Definition
typedef struct BoardPositionType{
  int x, y;
  char content;
  struct BoardPositionType* headward; //Towards Head;
  struct BoardPositionType* tailward; //Towards Tail;
} BoardPosition;

char content[MAX_X][MAX_Y];
int X[MAX_X][MAX_Y];
int prevY[MAX_X][MAX_Y];
int nextX[MAX_X][MAX_Y];
int nextY[MAX_X][MAX_Y];

BoardPosition board[MAX_X][MAX_Y];
BoardPosition* head;
BoardPosition* tail;
BoardPosition* apple;

xSemaphoreHandle lcdLock;

//X and Y refers to columns and rows, not pixel.
extern void printCharXY(int x, int y, char c){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	GLCD_displayChar(y * 24, SCREEN_WIDTH - (16 * (x+1)), c);
	xSemaphoreGive(lcdLock);
}


void updateScreen(BoardPosition* pos){
	printCharXY(pos->x, pos->y, pos->content);  
}


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

void exitGame(int stat) {
  
  printf("%03d: ", stat);
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
  
  //exit(stat);
  return;
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
}

static void ledTask(void *params) {
  unsigned short t = 0;

  for (;;) {
		ledOn(1<<t);
		t = (t+1)%4;
	  vTaskDelay(300 / portTICK_RATE_MS);
  }
}


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
	exitGame(__LINE__);
	
  for (i=0;i<MAX_Y;i++){
    for (j=0;j<MAX_X;j++){
			//printCharXY(j,i, board[j][i].content);
      printCharXY(j,i, TAIL);
    };
 };
}

void initiate() {
  int i,j;
	BoardPosition* pos;
  for (i=0;i<MAX_Y;i++) {
    for (j=0;j<MAX_X;j++){
			pos = &board[j][i];
			//pos->x = 5;
			//pos->y = i;*/
      /*board[j][i].x = j;
      board[j][i].y = i;
      board[j][i].content = EMPTY;
      board[j][i].headward = NULL;
      board[j][i].tailward = NULL;*/
    }
  }
	
	exitGame(__LINE__);
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

/* Retarget printing to the serial port 1 */
int fputc(int ch, FILE *f) {
  unsigned char c = ch;
  xQueueSend(printQueue, &c, 0);
  return ch;
}


/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

void test(){
	printCharXY(0,0,'#');
	printf("%d", __LINE__);
	exitGame(COMPLETE);
}


void Joy_stick(void *params){
	
	volatile JOY_State_TypeDef state;
	
	for(;;){
	 state = JOY_NONE;
	//state =  IOE_JoyStickGetState();
	state =  IOE_JoyStickGetState();
	ledOn(0);
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

  printQueue = xQueueCreate(128, 1);
//	Joy_stick();
  initDisplay();
  //setupButtons();
  xTaskCreate(ledTask, "lcd", 100, NULL, 1, NULL);
  xTaskCreate(printTask, "print", 100, NULL, 1, NULL);
  xTaskCreate(Joy_stick, "led", 100, NULL, 1, NULL);
  //xTaskCreate(touchScreenTask, "touchScreen", 100, NULL, 1, NULL);
  //xTaskCreate(highlightButtonsTask, "highlighter", 100, NULL, 1, NULL);

  printf("Setup complete ");  // this is redirected to the display

  vTaskStartScheduler();

  assert(0);
  return 0;                 // not reachable
}

/*-----------------------------------------------------------*/
