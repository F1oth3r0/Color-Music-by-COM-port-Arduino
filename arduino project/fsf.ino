// кредитс мозг флотхеро, и файлы алекс гавера
// документация к tinyLED.h на ассемблере))
// https://alexgyver.ru/microled/

// а еще скачай библу 
// открываешь папку C:\Program Files (x86)\Arduino\libraries, создаешь папку microLED 
// и перетаскиваешь туда эти файлы https://github.com/AlexGyver/GyverLibs/tree/master/microLED

#define TLED_ORDER ORDER_GRB
#define TLED_CHIP LED_WS2812

tinyLED<2> strip; // пин ленты

/* если ваш пин ленты от 0 до 7 (у меня второй пин, смотри строку выше) */ 
#define TLED_PORT PORTD
#define TLED_DDR DDRD

/* если ваш пин ленты от 8 до 13 */
//#define TLED_PORT PORTB
//#define TLED_DDR DDRB

#include "tinyLED.h"
#include "microUART_ISR.h"
microUART_ISR uart;

void setup() {
  uart.begin(2000000);
}

void loop() {
  while (uart.available()) {          // ждем что то с ком порта
    byte data = uart.read();          // читаем это как байты
    for (short i = 0; i < 600; i++)   // у меня 600 диодов. вы пишите свое значение лампочек на потолку
      strip.sendRGB(data, 0, 0);      // r, g, b - я ебашу всю комнату красным
  }
  /*// радуга (кому нужна)
  static byte count;
  count++;
  for (int i = 0; i < 600; i++) {
    strip.send(mWheel8(count + i * (255.0 / 600))); // выводим радугу
  }
  delay(20);*/
}
