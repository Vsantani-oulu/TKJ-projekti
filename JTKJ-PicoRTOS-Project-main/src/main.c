
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <task.h>

#include "tkjhat/sdk.h"

// RX  start
// states
enum state {IDLE=1, READ_USB, DISPLAY, SUSPENDED};
enum state RXcurrentState = SUSPENDED;
enum msg {NOT_READY=1, READY};
enum msg RXmessage = NOT_READY;
enum updated {NOT_UPDATED=1, UPDATED};
enum updated RXserialUpdated = NOT_UPDATED;
// message variables
char RX_msg[100] = "";
static char RX_empty_msg[100] = "";
// Prototypes
void startRX();
void stopRX();
void RX_stateChanger(void * pvParameters);
static void btn_fxn(uint gpio, uint32_t eventMask);
void RX_serial_task(void * pvParameters); 
void RX_outputTask(void * pvParameters);
void RX_recive_message(char msg);
void buzz_message();
void beep(char length); 
void translate_msg();
char translate_char(char word[]);
// RX end

void startRX(){
    // Enable reciving messages by starting the RXs state machine
    RXcurrentState = IDLE;
    //By Veeti
}

void stopRX(){
    // Disable reciving messages by setting state to SUSPENDED 
    // also clear value of all global variables used in RX (RX_msg, message, serialUpdated)
    clear_display();
    stop_display();
    RXcurrentState = SUSPENDED;
    memcpy(RX_msg, RX_empty_msg, strlen(RX_msg));
    RXmessage = NOT_READY;
    RXserialUpdated = NOT_UPDATED;
    //By Veeti
}

void RX_stateChanger(void * pvParameters) {
    // Handles changing states
    while(1){
        if (RXcurrentState == IDLE){
            init_display();
            RXserialUpdated = NOT_UPDATED;
            RXcurrentState = READ_USB;
            printf("reading USB");
        } else if (RXcurrentState == READ_USB && RXmessage == READY){
            // Only change into display state if message is ready and currently reading usb
            RXmessage = NOT_READY;
            RXcurrentState = DISPLAY;
            printf("displaying");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    //By Veeti
}

static void btn_fxn(uint gpio, uint32_t eventMask) {
    // Enables and disables rx
    if (RXcurrentState == SUSPENDED){
        printf("Starting");
        startRX();
    } else if (RXcurrentState == READ_USB){
        //only allow stopping the rx if state is READ_USB so that it cannot be stopped during any output task
        printf("Ending");
        stopRX();
    }
}

void RX_serial_task(void * pvParameters) {
    while(1){
        if (RXcurrentState == READ_USB){
            int ch = getchar_timeout_us(0);      
            if (ch >= 0) {
                RX_recive_message((char)ch);          
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        }
    //By Veeti
}


void RX_outputTask(void * pvParameters) {
    //Buzz message, display it on lcd and then clear RX_msg
    //Task has a lower priority than the other ones, running only when the others are idle 
    while(1){
        if (RXcurrentState == DISPLAY){
            buzz_message();
            translate_msg();
            RXcurrentState = IDLE;
            memcpy(RX_msg, RX_empty_msg, strlen(RX_msg));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    //By Veeti
}

void RX_recive_message(char msg){
    // Append input into RX_msg and set state of message to ready if its newline
    if (msg == '\n'){
        RXmessage = READY;
    } else if(msg >= 0){
        sprintf(RX_msg+strlen(RX_msg), "%c", msg);
    }
    //By Veeti
}

void buzz_message() {
    // Iterates over the RX_msg and uses the buzzer to translate dots into short beeps and lines into long beeps
    int c;
    for (c = 0; c < strlen(RX_msg); c++){
        if (RX_msg[c] == '.'){
            beep('S');
            printf("short\n");
        } else if (RX_msg[c] == '-'){
            beep('L');
            printf("long\n");
        } else if (RX_msg[c] == ' '){
            sleep_ms(500);
        }
        //short sleep so that its easier to see when one blink ends and other starts
        sleep_ms(100);   
    }
    //By Veeti
}

void beep(char length) {
    // buzzes for either 300ms (long) or 100ms (short)
    // freq is 700hz
    if (length == 'L'){
        buzzer_play_tone(700, 300);
    } else if (length == 'S'){
        buzzer_play_tone(700, 100);
    }
    //By Veeti
}

void translate_msg(){
    // Iterate over RX_msg and translate it (outputted string includes spaces between words)
    int i;

    char empty_word[100] = "";
    char word_to_be_translated[100] = "";
    char sentance_to_be_built[100] = "";

    // Tried using strtok, did not find a simple way to have it include double spaces so wrote my own version of it
    for (i = 0; i < strlen(RX_msg); i++){
        if (RX_msg[i] != ' '){
            // add characters to the string that gets send to be translated (word_to_be_translated) if current char isint a space
            sprintf(word_to_be_translated+strlen(word_to_be_translated), "%c", RX_msg[i]);
        } else if (RX_msg[i] == ' ' && RX_msg[i+1] == ' '){
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
    write_text_xy(0, 0, sentance_to_be_built);
    //By Veeti
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
    //By Veeti
}

int main() {
    stdio_init_all();
    init_hat_sdk();

    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    
    init_buzzer();
    init_display();

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);

    TaskHandle_t stateChangerHandle, serialTaskHandle, outputTaskHandle = NULL;
    BaseType_t result = xTaskCreate(RX_stateChanger,   // (en) Task function
        "stateChanger",                   // (en) Name of the task 
        1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
        NULL,                    // (en) Arguments of the task 
        2,                       // (en) Priority of this task
        &stateChangerHandle);     // (en) A handle to control the execution of this task
    if(result != pdPASS) {
        printf("state changer Task created failed\n");
        return 0;
    }    
    result = xTaskCreate(RX_serial_task,   // (en) Task function
        "serial",                   // (en) Name of the task 
        1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
        NULL,                    // (en) Arguments of the task 
        2,                       // (en) Priority of this task
        &serialTaskHandle);     // (en) A handle to control the execution of this task
    if(result != pdPASS) {
        printf("Serial Task created failed\n");
        return 0;
    }
    result = xTaskCreate(RX_outputTask,   // (en) Task function
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
