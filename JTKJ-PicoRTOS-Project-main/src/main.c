
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
void display_task();
uint8_t display_state = 0;

static void btn_fxn(uint gpio, uint32_t eventMask) {
    toggle_led();
    if (display_state == 1){
        display_state = 0;
    } else if (display_state == 0){
        display_state = 1;
    }
    printf("%d",display_state);
}
void uartFxn() {
        char input;
        char buf[30];
        input = uart_getc(uart0);
        sprintf(buf, "recived: %c\n", input);
        uart_puts(uart0, buf);
        printf(buf);

}
void serial_task() {


    uart_init(uart0, 9600);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);

    // Asetetaan keskeytyksen käsittelijä uartFxn UART0:n keskeytyksille
    irq_set_exclusive_handler(UART0_IRQ, uartFxn);
    // Aletaan vastaanottamaan keskeytyksiä
    irq_set_enabled(UART0_IRQ, true);

    // Kerrotaan UART:ille, että haluamme ottaa vastaan keskeytyksiä,
    // kun dataa on luettavissa.
    uart_set_irq_enables(uart0, true, false);

    while(1){
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}



void display_task() {
    while(1){
    if (display_state == 0){
        clear_display();
        write_text("click");
    } else if (display_state == 1){
        clear_display();
        write_text("clack");
    }
    vTaskDelay(pdMS_TO_TICKS(4000));
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
    init_display();

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);
    TaskHandle_t displayTaskHandle, serialTaskHandle = NULL;
    BaseType_t result = xTaskCreate(display_task,   // (en) Task function
            "display",                   // (en) Name of the task 
                1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                    // (en) Arguments of the task 
                2,                       // (en) Priority of this task
                &displayTaskHandle);     // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Display Task created failed\n");
        return 0;
    }
    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    // Never reach this line.
    return 0;
}
