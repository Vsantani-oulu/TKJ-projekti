#include <math.h>
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

#define LED_SEND_DELAY_MS 500 //led on päällä 0,5s kun viesti lähetetään
// states
enum state {LISTEN=1, PRINT};
enum state TXcurrentState = LISTEN;
enum msg {NOT_READY=1, READY};
enum msg message = NOT_READY;
int initialized = 0;
// message variables
char pending_msg[100] = ""; //sens taskin koostama viesti
static char empty_msg[100] = ""; 
int updated = 0;
char tx_msg[100] = ""; //napin painalluksella sens taski siirtää viestin tähän

static void valomerkki(int lenght); // valomerkki(merkin pituus)
static void btn_fxn(uint gpio, uint32_t eventMask); // tulee valomerkki ja viesti siirtyy lähetettäväksi
char asento_morseksi(float ax);
void sensor_task(void *arg);

void valomerkki(int lenght) {
    toggle_led();
    sleep_ms(500);
    toggle_led();
    //vesa
}

void send_morse_code(const char morse[])
{
    printf(morse);
    fputs(morse, stdout);
    fputs("  \n", stdout);
    fflush(stdout);
    //rami
}

static void btn_fxn(uint gpio, uint32_t eventMask) {
    // tulee valomerkki ja viesti siirtyy lähetettäväksi
    TXcurrentState = PRINT;
    //vesa
}

void TXstateChanger(void * pvParameters) {
    // Handles changing states
    while(1){
        if (TXcurrentState == PRINT){
            valomerkki(300);
            send_morse_code(pending_msg);
            memcpy(pending_msg, empty_msg, strlen(pending_msg));
            TXcurrentState = LISTEN;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

char asento_morseksi(float ax){
    if (ax > 0.75){
        return '.';  
    }

    if (ax < -0.75){
        return '-';
    }
    if (ax > -0.5 && ax < 0.5){
        return 'r'; //'reset' eli on laite käynyt yläasennossa
    }
    return '\0'; //bufferi asentojen välissä
    //vesa
}
void sensor_task(void *arg){
    float ax, ay, az;
    float gx, gy, gz;
    float temp;

    char symbol;
    char symbol2;
    int asentoReset = 1; //onko laite käynyt yläasennossa viime merkin jälkeen
    int spaceReady = 0; //onko yläasennossa oleva laite printannut välilyöntiä (1 -> voi printata spacen, 0 -> ei)
    int resetKesto = 0; //kauanko ollut yläasennossa, jotta tietää milloin pitää sprintf " "
    int cooldown = 0;
    //alustetaan sensorit
    init_ICM42670();
    ICM42670_start_with_default_values();
    //task
    for(;;){
        ICM42670_read_sensor_data(
            &ax, &ay, &az,
            &gx, &gy, &gz,
            &temp
        );
        
        symbol = asento_morseksi(ax);
        symbol2 = asento_morseksi(ax);
        if (symbol == symbol2){
            cooldown++;
        } else {
            cooldown = 0;
        }
        //printf("symbol: %c, resetKesto: %d, spaceReady: %d",symbol,resetKesto,spaceReady);
        if (cooldown >= 20){
        if(symbol == 'r'){ //laite on yläasennossa
            asentoReset = 1;
            resetKesto++;
            if(resetKesto >= 3 && spaceReady){ //0,5 s kohdalla
                sprintf(
                    pending_msg + strlen(pending_msg),
                    " "
                );
                spaceReady = 0;

            }
        } 
        
        else if(symbol != '\0')
        {   
            resetKesto = 0;
            if(asentoReset) //jos laite on käynyt yläasennossa edellisen merkin jälkeen
            {  
                sprintf(
                    pending_msg + strlen(pending_msg),
                    "%c",
                    symbol
                );
                resetKesto = 0;
                spaceReady = 1;
                asentoReset = 0; //pitää käydä yläasennossa uudestaan ennen seuraavaa merkkiä

            }
        }
        //muuten symbol == '\0' ei aiheuta toimenpiteitä
        cooldown = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        }

    //vesa
}

int main() {

    stdio_init_all();

    // Uncomment this lines if you want to wait till the serial monitor is connected
    /*while (!stdio_usb_connected()){
        sleep_ms(10);
    }*/ 
    
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.


    // Alusta painike ja LED ja rekisteröi vastaava keskeytys.
    // Keskeytyskäsittelijä on määritelty yläpuolella nimellä btn_fxn
    init_sw1();
    init_led();
    
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);
    
    TaskHandle_t sensorTaskHandle, TXstateChangerHandle = NULL;

    BaseType_t result = xTaskCreate(sensor_task, // Task function
        "sensor", // Name of the task
        1024, // Size of the stack for this task (in words).
        NULL, // Arguments of the task 
        2, // Priority of this task
        &sensorTaskHandle //A handle to control the execution of this task
    );
    if(result != pdPASS) {
        printf("Sensor Task created failed\n");
        return 0;
    }
    result = xTaskCreate(TXstateChanger,   // (en) Task function
        "stateChanger",                   // (en) Name of the task 
        1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
        NULL,                    // (en) Arguments of the task 
        2,                       // (en) Priority of this task
        &TXstateChangerHandle);     // (en) A handle to control the execution of this task
    if(result != pdPASS) {
        printf("state changer Task created failed\n");
        return 0;
    }    

    // Start the scheduler (never returns)
    vTaskStartScheduler();
    // Never reach this line.
    return 0;

    //vesa
}