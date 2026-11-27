
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
// states
enum state {IDLE=1, READ_UART, DISPLAY, SUSPENDED};
enum state currentState = SUSPENDED;
enum msg {NOT_READY=1, READY};
enum msg message = NOT_READY;
enum updated {NOT_UPDATED=1, UPDATED};
enum updated serialUpdated = NOT_UPDATED;
// message variables
char global_msg[100] = "";
static char empty_msg[100] = "";
// prototypes
void startRX();
void stopRX();
void stateChanger(void * pvParameters);
static void btn_fxn(uint gpio, uint32_t eventMask);
void uartFxn();
void serial_task(void * pvParameters); 
void outputTask(void * pvParameters);
void recive_message(char msg);
void buzz_message();
void beep(char length); 
void translate_msg();
char translate_char(char word[]);

void startRX(){
    // Enable reciving messages by starting the RXs state machine
    currentState = IDLE;
}

void stopRX(){
    // Disable reciving messages by setting state to SUSPENDED 
    // also clear value of all global variables used in RX (global_msg, message, serialUpdated)
    clear_display();
    stop_display();
    currentState = SUSPENDED;
    memcpy(global_msg, empty_msg, strlen(global_msg));
    message = NOT_READY;
    serialUpdated = NOT_UPDATED;
}

void stateChanger(void * pvParameters) {
    // Handles changing states
    while(1){
        if (currentState == IDLE){
            serialUpdated = NOT_UPDATED;
            currentState = READ_UART;
            printf("reading uart");
        } else if (currentState == READ_UART && message == READY){
            // Only change into display state if message is ready and currently reading uart
            message = NOT_READY;
            currentState = DISPLAY;
            printf("displaying");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void btn_fxn(uint gpio, uint32_t eventMask) {
    // Enables and disables rx
    if (currentState == SUSPENDED){
        printf("Starting");
        startRX();
    } else if (currentState == READ_UART){
        //only allow stopping the rx if state is READ_UART so that it cannot be stopped during any output task
        printf("Ending");
        stopRX();
    }
}

void serial_task(void * pvParameters) {
    uart_init(uart0, 9600);
    while(1){
        if (currentState == READ_UART){
            int ch = getchar_timeout_us(0);      
            if (ch >= 0) {
                recive_message((char)ch);          
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        }
}


void outputTask(void * pvParameters) {
    //Buzz message, display it on lcd and then clear global_msg
    //Task has a lower priority than the other ones, running only when the others are idle 
    while(1){
        if (currentState == DISPLAY){
            buzz_message();
            translate_msg();
            currentState = IDLE;
            memcpy(global_msg, empty_msg, strlen(global_msg));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void recive_message(char msg){
    // Append input into global_msg and set state of message to ready if its newline
    if (msg == '\n'){
        message = READY;
    } else if(msg >= 0){
        sprintf(global_msg+strlen(global_msg), "%c", msg);
    }
}

void write_message(char text[]) {
    // Write global_msg to display and clear it
    clear_display();
    write_text(text);
    return;
}

void buzz_message() {
    // Iterates over the global_msg and uses the buzzer to translate dots into short beeps and lines into long beeps
    int c;
    for (c = 0; c < strlen(global_msg); c++){
        if (global_msg[c] == '.'){
            beep('S');
            printf("short\n");
        } else if (global_msg[c] == '-'){
            beep('L');
            printf("long\n");
        }
        //short sleep so that its easier to see when one blink ends and other starts
        sleep_ms(1000);   
    }
}

void beep(char length) {
    // blinks the led for 300ms if - and 100ms if .
    if (length == 'L'){
        toggle_led();
        sleep_ms(3000);
        toggle_led();
    } else if (length == 'S'){
        toggle_led();
        sleep_ms(1000);
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

void translate_msg(){
    // Iterate over global_msg and translate it (outputted string includes spaces between words)
    int i;

    char empty_word[100] = "";
    char word_to_be_translated[100] = "";
    char sentance_to_be_built[100] = "";

    // Tried using strtok, did not find a simple way to have it include double spaces so wrote my own version of it
    for (i = 0; i < strlen(global_msg); i++){
        if (global_msg[i] != ' '){
            // add characters to the string that gets send to be translated (word_to_be_translated) if current char isint a space
            sprintf(word_to_be_translated+strlen(word_to_be_translated), "%c", global_msg[i]);
        } else if (global_msg[i] == ' ' && global_msg[i+1] == ' '){
            // if current char and next char is a space, append translated morse character AND a single space to the final message
            sprintf(sentance_to_be_built+strlen(sentance_to_be_built), "%c ", translate_char(word_to_be_translated));
            memcpy(word_to_be_translated, empty_word, strlen(word_to_be_translated));
        } else {
            // if only current char is a space, translate the current character and append it to the final string without adding a space after
            sprintf(sentance_to_be_built+strlen(sentance_to_be_built), "%c", translate_char(word_to_be_translated));
            memcpy(word_to_be_translated, empty_word, strlen(word_to_be_translated));
        }
    }
    clear_display();
    printf("writing");
    write_text(sentance_to_be_built);
}

char translate_char(char word[]){
    // Translate given morse word into a character,
    // using a massive elif ladder because of: https://stackoverflow.com/questions/4014827/how-can-i-compare-strings-in-c-using-a-switch-statement 
    if (!strcmp(word, ".-")){
        return 'a';
    } else if (!strcmp(word, "-...")) {  
        return 'b';
    } else if (!strcmp(word, "-.-.")) {  
        return 'c';
    } else if (!strcmp(word, "-..")) {  
        return 'd';
    } else if (!strcmp(word, ".")) {  
        return 'e';
    } else if (!strcmp(word, "..-.")) {  
        return 'f';
    } else if (!strcmp(word, "--.")) {  
        return 'g';
    } else if (!strcmp(word, "....")) {  
        return 'h';
    } else if (!strcmp(word, "..")) {  
        return 'i';
    } else if (!strcmp(word, ".---")) {  
        return 'j';
    } else if (!strcmp(word, "-.-")) {  
        return 'k';
    } else if (!strcmp(word, ".-..")) {  
        return 'l';
    } else if (!strcmp(word, "--")) {  
        return 'm';
    } else if (!strcmp(word, "-.")) {  
        return 'n';
    } else if (!strcmp(word, "---")) {  
        return 'o';
    } else if (!strcmp(word, ".--.")) {  
        return 'p';
    } else if (!strcmp(word, "--.-")) {  
        return 'q';
    } else if (!strcmp(word, ".-.")) {  
        return 'r';
    } else if (!strcmp(word, "...")) {
        return 's';
    } else if (!strcmp(word, "-")) {  
        return 't';
    } else if (!strcmp(word, "..-")) {  
        return 'u';
    } else if (!strcmp(word, "...-")) {
        return 'v';  
    } else if (!strcmp(word, ".--")) {  
        return 'w';
    } else if (!strcmp(word, "-..-")) {
        return 'x';  
    } else if (!strcmp(word, "-.--")) {  
        return 'y';
    } else if (!strcmp(word, "--..")) { 
        return 'z'; 
    } else if (!strcmp(word, " ")) {  
        return ' ';
    } else if (!strcmp(word, "")) {
        return (char)0;
    } else{
        return '?';
    }
}

int main() {
    stdio_init_all();
    init_hat_sdk();

    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    //init buzzer and dont init led if using buzzer, led is for testing purposes for the sim
    //init_buzzer();
    init_display();
    init_led();

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);

    TaskHandle_t stateChangerHandle, serialTaskHandle, outputTaskHandle = NULL;
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
    result = xTaskCreate(outputTask,   // (en) Task function
        "output",                   // (en) Name of the task 
        2048,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
        NULL,                    // (en) Arguments of the task 
        2,                       // (en) Priority of this task
        &outputTaskHandle);     // (en) A handle to control the execution of this task
    if(result != pdPASS) {
        printf("Output Task created failed\n");
        return 0;
    }
    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    // Never reach this line.
    return 0;
}
