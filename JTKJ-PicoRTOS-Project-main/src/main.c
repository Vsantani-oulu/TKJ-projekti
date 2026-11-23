
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <timers.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
// states
enum state {IDLE=1, READ_UART, DISPLAY};
enum state currentState = DISPLAY;
enum msg {NOT_READY=1, READY};
enum msg message = NOT_READY;

// message variables
char global_msg[30] = "waiting";
static char empty_msg[30] = "";

// prototypes
void stateChanger(void * pvParameters);
static void btn_fxn(uint gpio, uint32_t eventMask);
void uartFxn();
void serial_task(); 
void recive_message(char msg[]);
void write_message();
void buzz_message();
void beep(char length); 
void stateChanger(void * pvParameters) {
    // Handles changing states
    while(1){
        if (currentState == IDLE){
            currentState = READ_UART;
            printf("reading uart");
        } else if (currentState == READ_UART && message == READY){
            // Only change into display state if message is ready and currently reading uart
            message = NOT_READY;
            currentState = DISPLAY;
            printf("displaying");
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void btn_fxn(uint gpio, uint32_t eventMask) {
    // This is for debug purposes for checking states/variables during testing
    printf("current state: %d",currentState);
}

void uartFxn() {
    // Take char from UART and give it to recive_message()
    if (currentState == READ_UART){
        char input;
        char buf[30];
        input = uart_getc(uart0);
        sprintf(buf, "%c", input);
        recive_message(buf);
    }
}

void serial_task(void * pvParameters) {
    uart_init(uart0, 9600);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);

    irq_set_exclusive_handler(UART0_IRQ, uartFxn);

    while(1){
        // Enable uart only if state is READ_UART
        if (currentState == READ_UART){
            irq_set_enabled(UART0_IRQ, true);
            uart_set_irq_enables(uart0, true, false);
        } else {
            irq_set_enabled(UART0_IRQ, false);
            uart_set_irq_enables(uart0, false, false);   
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void commTask(void * pvParameters) {
    //Buzz message, display it on lcd and then clear global_msg
    //Task has a lower priority than the other ones, running only when the others are idle 
    while(1){
        if (currentState == DISPLAY){
            buzz_message();
            write_message();
            currentState = IDLE;
            memcpy(global_msg, empty_msg, strlen(global_msg));
        }
    }
}

void recive_message(char msg[]){
    // Append input into global_msg and set state of message to ready if its newline
    if (msg[0] == '\n'){
        message = READY;
    } else if(msg[0] != '\0'){
        sprintf(global_msg+strlen(global_msg), "%s", msg);
    }
}

void write_message() {
    // Write global_msg to display and clear it
    clear_display();
    write_text(global_msg);
    return;
}

void buzz_message() {
    // Iterates over the global_msg and uses the buzzer to translate dots into short beeps and lines into long beeps
    int len = (sizeof(global_msg) / sizeof(global_msg[0]));
    int c;
    for (c = 0; c < len; c++){
        if (global_msg[c] == '.'){
            beep('S');
            printf("short\n");
        } else if (global_msg[c] == '-'){
            beep('L');
            printf("long\n");
        }
        //short sleep so that its easier to see when one blink ends and other starts
        sleep_ms(100);   
    }
}

void beep(char length) {
    // blinks the led for 300ms if - and 100ms if .
    if (length == 'L'){
        toggle_led();
        sleep_ms(300);
        toggle_led();
    } else if (length == 'S'){
        toggle_led();
        sleep_ms(100);
        toggle_led();
    }

    // This part of the code uses the buzzer, the simualtion struggles with the buzzer so for testing purposes led is used instead
    // beeps either for 300ms (long) or 100ms (short), frequency used is 100hz 
    //if (length == 'L'){
    //    buzzer_play_tone(500,  300);
    //    printf("long");
    //} else if (length == 'S'){
    //    buzzer_play_tone(500,  100);
    //    printf("Short");
}
int main() {
    stdio_init_all();
    init_hat_sdk();

    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    init_display();

    //init buzzer and dont init led if using buzzer, led is for testing purposes for the sim
    //init_buzzer();
    init_led();

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);

    TaskHandle_t stateChangerHandle, serialTaskHandle, commTaskHandle = NULL;
    BaseType_t result = xTaskCreate(stateChanger,   // (en) Task function
        "stateChanger",                   // (en) Name of the task 
        1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
        NULL,                    // (en) Arguments of the task 
        2,                       // (en) Priority of this task
        &stateChangerHandle);     // (en) A handle to control the execution of this task
    if(result != pdPASS) {
        printf("state changer Task created failed\n");
        return 0;
    }    
    result = xTaskCreate(serial_task,   // (en) Task function
        "serial",                   // (en) Name of the task 
        1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
        NULL,                    // (en) Arguments of the task 
        2,                       // (en) Priority of this task
        &serialTaskHandle);     // (en) A handle to control the execution of this task
    if(result != pdPASS) {
        printf("Serial Task created failed\n");
        return 0;
    }
    result = xTaskCreate(commTask,   // (en) Task function
        "comm",                   // (en) Name of the task 
        1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
        NULL,                    // (en) Arguments of the task 
        1,                       // (en) Priority of this task
        &commTaskHandle);     // (en) A handle to control the execution of this task
    if(result != pdPASS) {
        printf("Comm Task created failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    // Never reach this line.
    return 0;
}
