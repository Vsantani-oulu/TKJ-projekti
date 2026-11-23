#include <stdio.h>
#include <ctype.h>
#include "pico/stdlib.h"
// morse code mapping for A-Z and 0-9, current code doesn¨t respect lower characters
// uses index locations to map character to morse code, exmpl A = 0, B = 1, Z = 25, 0 = 26...
static const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static const char *const morse_codes[] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
    "..-", "...-", ".--", "-..-", "-.--", "--..",
    "-----", ".----", "..---", "...--", "....-", ".....",
    "-....", "--...", "---..", "----."
};

void send_morse_code(const char text[])
{
    bool first_character = true;

    while (text[0] != '\0') { // loop through each character, by checking for null terminator
        char character = (char)toupper((unsigned char)text[0]); // convert to uppercase
        text++;
        if (character == ' ') {
            if (!first_character) {
                printf(" / "); // use slash to indicate space between words
                first_character = true;
            }
            continue;

        }

        for (int i = 0; i < 36; i++) {
            if (characters[i] == character) { // find the corresponding morse code
                if (!first_character) {
                    putchar(' '); // space between letters
                }
                fputs(morse_codes[i], stdout); //print morse code
                first_character = false;
                break;
            }
        }

    }

    puts(""); //print newline at the end of morse code
}
        
        

int main()
{
    stdio_init_all();

    send_morse_code("HELLO WORLD"); //example test, TO BE CHANGED TO USER INPUT
    while (true) {
        sleep_ms(1000);
    }
}
