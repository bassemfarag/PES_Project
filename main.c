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
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "defines.h"

/*-----------------------------------------------------------*/

#define WIDTH 320
int pos[VERT][HOR];

xSemaphoreHandle lcdLock;


static void print_sprite(unsigned int pos_x, unsigned int pos_y, unsigned int sprite) {
	//char* sprite_path;
	//sprintf(sprite_path, "SPRITE_", sprite);
	GLCD_bitmap(SCREEN_WIDTH - pos_x*SPRITE_DIM, 
							pos_y*SPRITE_DIM, 
							SPRITE_DIM, SPRITE_DIM, 
							(unsigned char*) blob);
}


static void initDisplay () {
  /* LCD Module init */

  lcdLock = xSemaphoreCreateMutex();

  GLCD_init();
  GLCD_clear(White);
  GLCD_setTextColor(Blue);
  GLCD_displayStringLn(Line2, " Programming");
  GLCD_displayStringLn(Line3, " Embedded");
  GLCD_displayStringLn(Line4, " Systems");
}

static void lcdTask(void *params) {
  unsigned short col1 = Blue, col2 = Red, col3 = Green;
  unsigned short t;

  for (;;) {
    xSemaphoreTake(lcdLock, portMAX_DELAY);
    GLCD_setTextColor(col1);
    GLCD_displayChar(Line7, WIDTH - 40, '1');
    GLCD_setTextColor(col2);
    GLCD_displayChar(Line7, WIDTH - 60, '2');
    GLCD_setTextColor(col3);
    GLCD_displayChar(Line7, WIDTH - 80, '3');
    xSemaphoreGive(lcdLock);
	t = col1; col1 = col2; col2 = col3; col3 = t;
    vTaskDelay(300 / portTICK_RATE_MS);
  }
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

/**
 * Blink the LEDs to show that we are alive
 */

static void ledTask(void *params) {
/*  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( GPIOB, &GPIO_InitStructure );

  for (;;) {
    GPIO_SetBits(GPIOB, GPIO_Pin_11);
    vTaskDelay(1000 / portTICK_RATE_MS);
    GPIO_ResetBits(GPIOB, GPIO_Pin_11);
    vTaskDelay(1000 / portTICK_RATE_MS);
  } */

  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef timInit;
  TIM_OCInitTypeDef TIM_OCInitStruct;

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	        // IMPORTANT to use AF!
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( GPIOB, &GPIO_InitStructure );

  /* Setup timer TIM3 for pulse-width modulation:
     100kHz, periodically counting from 0 to 9999 */
  RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );

  GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);	        // Remap channel 3 to PB10

  TIM_DeInit( TIM2 );
  TIM_TimeBaseStructInit( &timInit );

  timInit.TIM_Period = (unsigned portSHORT)9999;
  timInit.TIM_Prescaler = 20000;
  timInit.TIM_CounterMode = TIM_CounterMode_Up;
    
  TIM_TimeBaseInit( TIM2, &timInit );
  TIM_ARRPreloadConfig( TIM2, ENABLE );

  TIM_OCStructInit(&TIM_OCInitStruct);
  TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStruct.TIM_OutputNState = TIM_OutputState_Disable;
  TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStruct.TIM_Pulse = 3000;
  TIM_OC3Init(TIM2, &TIM_OCInitStruct);			 // PB10

  TIM_Cmd(TIM2, ENABLE);

  for (;;) {
    printf("%d ", TIM_GetCounter(TIM2));

    vTaskDelay(500 / portTICK_RATE_MS);
  }
}

/*-----------------------------------------------------------*/

/**
 * Register a callback that will be invoked when a touch screen
 * event occurs within a given rectangle
 *
 * NB: the callback function should have a short execution time,
 * since long-running callbacks will prevent further events from
 * being generated
 */

typedef struct {
  u16 lower, upper, left, right;
  void *data;
  void (*callback)(u16 x, u16 y, u16 pressure, void *data);
} TSCallback;

static TSCallback callbacks[64];
static u8 callbackNum = 0;

void registerTSCallback(u16 left, u16 right, u16 lower, u16 upper,
                        void (*callback)(u16 x, u16 y, u16 pressure, void *data),
						void *data) {
  callbacks[callbackNum].lower    = lower;
  callbacks[callbackNum].upper    = upper;
  callbacks[callbackNum].left     = left;
  callbacks[callbackNum].right    = right;
  callbacks[callbackNum].callback = callback;
  callbacks[callbackNum].data     = data;
  callbackNum++;
}

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
	  for (i = 0; i < callbackNum; ++i) {
		if (callbacks[i].left  <= ts_state->X &&
		    callbacks[i].right >= ts_state->X &&
		    callbacks[i].lower >= ts_state->Y &&
		    callbacks[i].upper <= ts_state->Y)
		  callbacks[i].callback(ts_state->X, ts_state->Y, ts_state->Z,
		                        callbacks[i].data);
	  }													
	  pressed = 1;
	}

    if (ts_state->TouchDetected) {
	  printf("%d,%d,%d ", ts_state->X, ts_state->Y, ts_state->Z);
	}

	vTaskDelayUntil(&lastWakeTime, 100 / portTICK_RATE_MS);
  }
}

/*-----------------------------------------------------------*/

xQueueHandle buttonQueue;

static void highlightButton(u16 x, u16 y, u16 pressure, void *data) {
  u16 d = (int)data;
  xQueueSend(buttonQueue, &d, 0);
}

static void setupButtons(void) {
  u16 i;
  buttonQueue = xQueueCreate(4, sizeof(u16));
  
  for (i = 0; i < 3; ++i) {
    GLCD_drawRect(30 + 60*i, 30, 40, 40);
	registerTSCallback(WIDTH - 30 - 40, WIDTH - 30, 30 + 60*i + 40, 30 + 60*i,
	                   &highlightButton, (void*)i);
  }
}

static void highlightButtonsTask(void *params) {
  u16 d;

  for (;;) {
    xQueueReceive(buttonQueue, &d, portMAX_DELAY);

    xSemaphoreTake(lcdLock, portMAX_DELAY);
    GLCD_setTextColor(Red);
    GLCD_fillRect(31 + 60*d, 31, 38, 38);
    GLCD_setTextColor(Blue);
    xSemaphoreGive(lcdLock);

	vTaskDelay(500 / portTICK_RATE_MS);

    xSemaphoreTake(lcdLock, portMAX_DELAY);
    GLCD_setTextColor(White);
    GLCD_fillRect(31 + 60*d, 31, 38, 38);
    GLCD_setTextColor(Blue);
    xSemaphoreGive(lcdLock);
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

  initDisplay();
  setupButtons();

  xTaskCreate(lcdTask, "lcd", 100, NULL, 1, NULL);
  xTaskCreate(printTask, "print", 100, NULL, 1, NULL);
  xTaskCreate(ledTask, "led", 100, NULL, 1, NULL);
  xTaskCreate(touchScreenTask, "touchScreen", 100, NULL, 1, NULL);
  xTaskCreate(highlightButtonsTask, "highlighter", 100, NULL, 1, NULL);
  
  printf("Setup complete ");  // this is redirected to the display
  print_sprite(10,10,0);
  vTaskStartScheduler();

  assert(0);
  return 0;                 // not reachable
}

/*-----------------------------------------------------------*/
