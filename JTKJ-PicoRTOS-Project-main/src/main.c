
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
// states
enum state {IDLE=1, READ_UART, DISPLAY};
enum state currentState = DISPLAY;
enum msg {NOT_READY=1, READY};
enum msg message = NOT_READY;
int initialized = 0;
// message variables
char global_msg[100] = "";
static char empty_msg[100] = "";
int updated = 0;
// Veetin prototyypit voipi poistaa
void stateChanger(void * pvParameters);


static void btn_fxn(uint gpio, uint32_t eventMask) {
    // Prints msg
    printf(global_msg);
}

void stateChanger(void * pvParameters) {
    // Handles changing states
    while(1){
        if (currentState == IDLE){
            updated = 0;
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

////MORSEN LUKEMINEN SENSOREILLA:

int asentoReset = 1; //onko laite käynyt yläasennossa viime merkin jälkeen
int spaceReset = 0; //onko yläasennossa oleva laite printannut välilyöntiä

char asento_morseksi(float ax){
    if (ax > 0.75){
        return '.';  
    }

    if (ax < -0.75){
        return '-';
    }
    if (0.5 > ax && ax > -0.5){
        return 'r'; //'reset' eli on laite käynyt yläasennossa
    }
    return '\0'; //bufferi asentojen välissä
}

void sensor_task(void *arg){
    float ax, ay, az;
    float gx, gy, gz;
    float temp;

    char symbol;

    int resetKesto = 0;

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
        //printf("symbol: %c, resetKesto: %d, spaceReset: %d",symbol,resetKesto,spaceReset);
        if(symbol == 'r'){ //laite on yläasennossa
            asentoReset = 1;
            resetKesto++;
            if(resetKesto >= 50 && spaceReset){ //0,5 s kohdalla
                sprintf(
                    global_msg + strlen(global_msg),
                    " "
                );
                printf("vali\n");
                spaceReset = 0;

        resetKesto = 0;

            }
        } 
        
        else if(symbol != '\0')
        {   
            resetKesto = 0;
            if(asentoReset) //jos laite on käynyt yläasennossa edellisen merkin jälkeen
            {   
                sprintf(
                    global_msg + strlen(global_msg),
                    "%c",
                    symbol
                );
                spaceReset = 1;

                asentoReset = 0; //pitää käydä yläasennossa uudestaan ennen seuraavaa merkkiä

                printf("%c\n", symbol);
            }
        }
        //muuten symbol == '\0' ei aiheuta toimenpiteitä

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void sensor_test(void){
    float ax, ay, az;
    float gx, gy, gz;
    float temp;

    init_ICM42670();
    ICM42670_start_with_default_values();

    for (;;)
    {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &temp) == 0) {
            
            printf("Accel: X=%f, Y=%f, Z=%f | Gyro: X=%f, Y=%f, Z=%f| Temp: %2.2f°C\n", ax, ay, az, gx, gy, gz, temp);

        } 
        else {
            printf("Failed to read imu data\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));

        printf("ax=%.2f ay=%.2f az=%.2f\n",
            ax, ay, az);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {

    // Exercise 4: Comment the statement stdio_init_all(); 
    //             Instead, add AT THE END OF MAIN (before vTaskStartScheduler();) adequate statements to enable the TinyUSB library and the usb-serial-debug.
    //             You can see hello_dual_cdc for help
    //             In CMakeLists.txt add usb_serial_debug to target_link_libraries.
    //             In CMakeLists.txt deactivate pico_enable_stdio_usb
    // Tehtävä 4:  Kommentoi lause stdio_init_all();
    //             Sen sijaan lisää MAIN LOPPUUN (ennen vTaskStartScheduler();) tarvittavat komennot aktivoidaksesi TinyUSB-kirjaston ja usb-serial-debugin.
    //             Voit katsoa apua esimerkistä hello_dual_cdc.
    //             Lisää CMakeLists.txt-tiedoston target_link_libraries-kohtaan usb_serial_debug.
    //             Poista CMakeLists.txt-tiedostosta käytöstä pico_enable_stdio_usb

    stdio_init_all();

    // Uncomment this lines if you want to wait till the serial monitor is connected
    /*while (!stdio_usb_connected()){
        sleep_ms(10);
    }*/ 
    
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    // Exercise 1: Initialize the button and the led and define an register the corresponding interrupton.
    //             Interruption handler is defined up as btn_fxn
    // Tehtävä 1:  Alusta painike ja LEd ja rekisteröi vastaava keskeytys.
    //             Keskeytyskäsittelijä on määritelty yläpuolella nimellä btn_fxn
    init_sw1();
    init_led();

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);
    
    TaskHandle_t sensorTaskHandle = NULL;

    // Exercise 4: Uncomment this xTaskCreate to create the task that enables dual USB communication.
    // Tehtävä 4: Poista tämän xTaskCreate-rivin kommentointi luodaksesi tehtävän,
    // joka mahdollistaa kaksikanavaisen USB-viestinnän.

    /*
    xTaskCreate(usbTask, "usb", 2048, NULL, 3, &hUSB);
    #if (configNUMBER_OF_CORES > 1)
        vTaskCoreAffinitySet(hUSB, 1u << 0);
    #endif
    */


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

    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    // Never reach this line.
    return 0;
}