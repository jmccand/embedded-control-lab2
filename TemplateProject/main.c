////////////////////////////////////////////////////////////////////////
//** ENGR-2350 Template Project 
//** NAME: Joel McCandless and Jeremy Goldberger
//** RIN: 662056597 and 662060308
//** This is the base project for several activities and labs throughout
//** the course.  The outline provided below isn't necessarily *required*
//** by a C program; however, this format is required within ENGR-2350
//** to ease debugging/grading by the staff.
////////////////////////////////////////////////////////////////////////

// We'll always add this include statement. This basically takes the
// code contained within the "engr_2350_msp432.h" file and adds it here.
#include "engr2350_msp432.h"
#include <stdlib.h>

#define BMP_LEN 6

// Add function prototypes here as needed.
void GPIO_init(void);
void Timer_init(void);
void Timer_ISR(void);
int8_t bumper_pressed(void);
void set_LED_color(uint8_t color);
void LED_off(void);
bool PB_pressed(void);
void main_game(void);
void BiLED_red(void);
void BiLED_green(void);
void BiLED_off(void);
void wait(uint8_t periods);

// Add global variables here as needed.
Timer_A_UpModeConfig config;
const uint_fast16_t BMP_PINS[] = {GPIO_PIN0, GPIO_PIN2, GPIO_PIN3, GPIO_PIN5, GPIO_PIN6, GPIO_PIN7};
const uint8_t COLORS[6][3] = {{0,0,1}, {0,1,0}, {0,1,1}, {1,0,0}, {1,0,1}, {1,1,0}};
uint8_t sequence[10];
volatile uint8_t counter = 0;

int main( void ) {    /** Main Function ****/
  
    // Add local variables here as needed.

    // We always call the SysInit function first to set up the 
    // microcontroller for how we are going to use it.
    SysInit();
    GPIO_init();
    Timer_init();

    // Place initialization code (or run-once) code here
    printf("Instructions\n\rPress the push button to start. Press the bumpers to show corresponding colors. Match the pattern. If you want to delete, press the push button while releasing the bumper.\n\r");
    while( 1 ) {  
        // Place code that runs continuously in here
        // start
        int8_t bmp = bumper_pressed();
        printf("bmp: %d\n\r", bmp);
        if (bmp != -1) {
            set_LED_color((uint8_t)bmp);
        } else {
            LED_off();
            if (PB_pressed()) {
                while(1) {
                    main_game();
                }
            }
        }
    }   
}    /** End Main Function ****/   

// Add function declarations here as needed

void GPIO_init(void) {
    // BiLED
    GPIO_setAsOutputPin(GPIO_PORT_P6,GPIO_PIN0 | GPIO_PIN1);
    // bumpers
    uint8_t i;
    for (i = 0; i < BMP_LEN; ++i) {
        GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, BMP_PINS[i]);
    }
    // LED
    GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
    // PB
    GPIO_setAsInputPinWithPullDownResistor(GPIO_PORT_P5,GPIO_PIN6);
}

void Timer_init(){
    config.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    config.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_64;
    config.timerPeriod = 18750;
    config.timerClear = TIMER_A_DO_CLEAR;
    config.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
    Timer_A_configureUpMode(TIMER_A2_BASE, &config);
    Timer_A_registerInterrupt(TIMER_A2_BASE, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT, Timer_ISR);
    Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_UP_MODE);
}

int8_t bumper_pressed(void) {
    uint8_t i = 0;
    for (i = 0; i < BMP_LEN; ++i) {
        if (!GPIO_getInputPinValue(GPIO_PORT_P4, BMP_PINS[i])) {
            return i;
        }
    }
    return -1;
}


void set_LED_color(uint8_t color) {
    const uint8_t* target_COLORS = COLORS[color];
    if (target_COLORS[0] == 1) {
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
    } else {
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
    }
    if (target_COLORS[1] == 1) {
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
    } else {
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
    }
    if (target_COLORS[2] == 1) {
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
    } else {
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
    }
}

void LED_off(void) {
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
}

bool PB_pressed(void) {
    return GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN6);
}

void main_game(void) {
    // generate sequence
    uint8_t i;
    for (i = 0; i < 10; ++i) {
        sequence[i] = rand() % 6;
    }

    uint8_t stage;
    bool won = true;
    int8_t bumper_held = -1;

    for(stage = 1; stage <= 10 && won == true; ++stage) {
        BiLED_red();
        wait(10);
        for (i = 0; i < stage; ++i) {
            wait(10);
            set_LED_color(sequence[i]);
            wait(10);
            LED_off();
        }
        BiLED_green();
        counter = 0;
        uint8_t step = 0;
        while(1) {
            if (counter > 60) {
                // 3 seconds -- lost
                won = false;
                break;
            }
            int8_t bmp = bumper_pressed();
            if (bmp == -1) {
                LED_off();
                if (bumper_held != -1) {
                    bumper_held = -1;
                    if (PB_pressed()) {
                        continue;
                    } else {
                        if (bumper_held == sequence[step]) {
                            ++step;
                            if (step == stage) {
                                // next stage
                                break;
                            }
                        } else {
                            // wrong -- lost
                            won = false;
                            break;
                        }
                    }
                }
            } else {
                set_LED_color((uint8_t)bmp);
                bumper_held = bmp;
            }
        }
    }
    // win lose
    if (won) {
        printf("YOU WIN!");
        counter = 0;
        while(!PB_pressed()) {
            if (counter > 10) {
                BiLED_off();
                counter = 0;
            } else if (counter > 5) {
                BiLED_green();
            }
        }
    } else {
        printf("INCORRECT. YOU LOSE!");
        counter = 0;
        while(!PB_pressed()) {
            if (counter > 10) {
                BiLED_off();
                counter = 0;
            } else if (counter > 5) {
                BiLED_red();
            }
        }
    }
}

void BiLED_red(void) {
    GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN1);
}

void BiLED_green(void) {    
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN0);
    GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN1);
}

void BiLED_off(void) {
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN1);
}

void wait(uint8_t periods) {
    counter = 0;
    while (counter < periods);
}

// Add interrupt functions last so they are easy to find
void Timer_ISR(void) {
    Timer_A_clearInterruptFlag(TIMER_A2_BASE);
    counter++;
}