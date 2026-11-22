
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
enum state currentState = IDLE;
enum msg {NOT_READY=1, READY};
enum msg message = NOT_READY;

// message variables
char global_msg[30] = "";
static char empty_msg[30] = "";

// prototypes
void stateChanger(void * pvParameters);
static void btn_fxn(uint gpio, uint32_t eventMask);
void uartFxn();
void serial_task(); 
void recive_message(char msg[]);
void write_message();

void stateChanger(void * pvParameters) {
    // Handles changing states
    write_text("waiting");
    while(1){
        printf("called");
        if (currentState == IDLE){
            currentState = READ_UART;
            printf("reading uart");
        } else if (currentState == READ_UART && message == READY){
            // Only change into display state if message is ready and currently reading uart
            message = NOT_READY;
            currentState = DISPLAY;
            printf("displaying");
        } else if (currentState == DISPLAY) {
            write_message();
            printf("returing to idle");
            currentState = IDLE;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
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

void serial_task() {
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
    memcpy(global_msg, empty_msg, strlen(global_msg));
    return;
}

int main() {
    stdio_init_all();
    init_hat_sdk();

    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    init_display();

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);

    TaskHandle_t stateChangerHandle, serialTaskHandle = NULL;
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

    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    // Never reach this line.
    return 0;
}
