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

//Global variables from board.
volatile int content[MAX_X][MAX_Y];
volatile int headward[MAX_X][MAX_Y];
volatile int tailward[MAX_X][MAX_Y];
volatile int head;
volatile int tail;
volatile int apple;
volatile int dir_x = 0, dir_y = 0;
volatile int length;

//Variables to manage the speed of the game
volatile int WAIT = 0;
const int timeStep = 10;

//Functions to set directions of the snake movement.
void up()   { dir_x =  0; dir_y =  1; }
void down() { dir_x =  0; dir_y = -1; }
void left() { dir_x = -1; dir_y =  0; }
void right(){ dir_x =  1; dir_y =  0; }

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

//X and Y refers to columns and rows, not pixel.
extern void printCharXY(int x, int y, char c){
	xSemaphoreTake(lcdLock, portMAX_DELAY);
	GLCD_displayChar(y * 24, SCREEN_WIDTH - (16 * (x+1)), c);
	xSemaphoreGive(lcdLock);
}

void updateScreen(BoardPosition* pos){
	printCharXY(pos->x, pos->y, pos->content);  
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

//Status codes to show when exiting game.
void exitGame(int stat) {
  gotoxy(0, MAX_Y+3);
  printf("%03d: ", stat);
  switch (stat) {
    case ERR       : printf("Some fatal error occurred"); break;
    case WIN       : printf("Congratulations! You won!"); break;
    case GAME_OVER : printf("Uh oh! Game Over!");         break;
    case COMPLETE  : printf("End of program.");           break;
    default        : printf("LINE");
  }
  exit(stat);
}

//Print information about the board
void printBoardPos(int pos) {
  gotoxy(0, MAX_Y+2);
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
  //If the board is full, i.e. there is no place left to continue.
  if (length >= MAX_X * MAX_Y)
    exitGame(WIN);

  //Find an empty slot to put the apple
  int x, y; 
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
  srand(time(NULL));
  generateApple();
  
  //Set starting direction
  right();
  
  //Clear screen and print board;
  printBoard();
  drawWalls(); //Only on windows
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
 
  head = setXY((getX(head)+dir_x) % mx, (getY(head)-dir_y) % my);
  
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
    default    : exitGame(__LINE__);
  }
  updateScreen(headOld);
  updateScreen(head);
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
  initiate();
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
