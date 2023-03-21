
// https://github.com/fsalomon/HD44780-decoder/blob/main/sniff.ino

#define NUM_CHARS 80
#include "esphome.h"
#define LINE_LENGTH 20
#define DDRAM_SIZE 104
#define CGRAM_SIZE 64
#define MAX_UTF8_CHAR_LENGTH 7 // 4 bytes for character, 3 bytes for variation selector

#define LCD_STR_BEDTEMP      '\x00'
#define LCD_STR_DEGREE       '\x01'
#define LCD_STR_THERMOMETER  '\x02'
#define LCD_STR_UPLEVEL      '\x03'
#define LCD_STR_REFRESH      '\x04'
#define LCD_STR_FOLDER       '\x05'
#define LCD_STR_FEEDRATE     '\x06'
#define LCD_STR_ARROW_2_DOWN '\x06'
#define LCD_STR_CLOCK        '\x07'
#define LCD_STR_CONFIRM      '\x07'
#define LCD_STR_ARROW_RIGHT  '\x7E'
#define LCD_STR_SOLID_BLOCK  '\xFF'

// https://github.com/prusa3d/Prusa-Firmware/commit/1c26875e0e729ce6bc339f1e746c0a9f767e2096
#define LCD_STR_ARROW_2_DOWN_OLD '\x01'
#define LCD_STR_CONFIRM_OLD '\x02'

gpio_num_t EN = GPIO_NUM_19;
gpio_num_t RS = GPIO_NUM_18;
gpio_num_t D4 = GPIO_NUM_21;
gpio_num_t D5 = GPIO_NUM_17;
gpio_num_t D6 = GPIO_NUM_22;
gpio_num_t D7 = GPIO_NUM_16;

const uint8_t lcd_chardata_arr2down[8] = {
	B00000,
	B00000,
	B10001,
	B01010,
	B00100,
	B10001,
	B01010,
	B00100,
};

const uint8_t lcd_chardata_confirm[8] = {
	B00000,
	B00001,
	B00011,
	B10110,
	B11100,
	B01000,
	B00000,
};

void decodeTask(void *arg);
void evalCommand(uint8_t);

bool gotUpNib = false;
uint8_t upperNibble;
long changetime = 0;

uint8_t DDRAM[DDRAM_SIZE] = { 32 };
uint8_t CGRAM[CGRAM_SIZE] = { 0 };
uint8_t ddramIndex = 0;
uint8_t cgramIndex = 0;
bool receivingDdram = false;
bool receivingCgram = false;

xQueueHandle interputQueue;

static void gpio_interrupt_handler(void *args)
{
  uint8_t rs = gpio_get_level(RS);
  uint8_t d7 = gpio_get_level(D7);
  uint8_t d6 = gpio_get_level(D6);
  uint8_t d5 = gpio_get_level(D5);
  uint8_t d4 = gpio_get_level(D4);

  uint8_t data = (rs << 7) + (d7 << 3) + (d6 << 2) + (d5 << 1) + d4;
  xQueueSendFromISR(interputQueue, &data, NULL);
}

void setup1() {
  gpio_set_direction(RS, GPIO_MODE_INPUT);
  gpio_set_direction(D7, GPIO_MODE_INPUT);
  gpio_set_direction(D6, GPIO_MODE_INPUT);
  gpio_set_direction(D5, GPIO_MODE_INPUT);
  gpio_set_direction(D4, GPIO_MODE_INPUT);

  interputQueue = xQueueCreate(20, sizeof(int));
  xTaskCreate(decodeTask, "DecodeTask", 2048, NULL, 1, NULL);

	gpio_set_direction(EN, GPIO_MODE_INPUT);
	gpio_set_intr_type(EN, GPIO_INTR_NEGEDGE);
	gpio_intr_enable(EN);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(EN, gpio_interrupt_handler, NULL);
}

void decode(uint8_t data, bool rs) {
  if (gotUpNib) {
    data = (upperNibble << 4 | data);
    gotUpNib = false;
  } else {
    upperNibble = data;
    gotUpNib = true;
    return;
  }
  if (rs == 1) {
    // store received byte
    if (receivingCgram) {
      CGRAM[cgramIndex] = data;
      cgramIndex = (cgramIndex + 1) % CGRAM_SIZE;
    } else if (receivingDdram) {
      DDRAM[ddramIndex] = data;
      ddramIndex = (ddramIndex + 1) % DDRAM_SIZE;
    }
  } else {
    // command received
    evalCommand(data);
  }
}


void decodeTask(void *arg) {
  uint8_t data;
  while (true) {
    if (xQueueReceive(interputQueue, &data, portMAX_DELAY)) {
      decode(data & 0xF, data >> 7);
    }
  }
}

const uint8_t LCD_CLEAR =  0x01;
const uint8_t LCD_CLEAR_MASK = 0xFF;

const uint8_t LCD_RETURN_HOME =  0x02;
const uint8_t LCD_RETURN_HOME_MASK = 0xFE;

const uint8_t LCD_SETDDRAMADDR = 0x80;
const uint8_t LCD_SETDDRAMADDR_MASK = 0x80;

const uint8_t LCD_SETCGRAMADDR = 0x40;
const uint8_t LCD_SETCGRAMADDR_MASK = 0xC0;

const uint8_t LCD_SETFUNCTION_4BIT = 0x20;
const uint8_t LCD_SETFUNCTION_8BIT = 0x30;
const uint8_t LCD_SETFUNCTION_MASK = 0xF0;

void evalCommand(uint8_t command) {
  receivingCgram = false;
  receivingDdram = false;

  if ((command & LCD_CLEAR_MASK) == LCD_CLEAR) {
    for (int i = 0; i < DDRAM_SIZE; i++) {
      DDRAM[i] = 32;
    }
    ddramIndex = 0;
    return;
  }
  if ((command & LCD_RETURN_HOME_MASK) == LCD_RETURN_HOME) {
    ddramIndex = 0;
    return;
  }
  if ((command & LCD_SETCGRAMADDR_MASK) == LCD_SETCGRAMADDR) {
    cgramIndex = (command & 0x3f) % CGRAM_SIZE;
    receivingCgram = true;
    return;
  }
  if ((command & LCD_SETDDRAMADDR_MASK) == LCD_SETDDRAMADDR) {
    ddramIndex = (command & 0x7f) % DDRAM_SIZE;
    receivingDdram = true;
    return;
  }

  // unknown command
}

inline bool isBitmapInCgram(uint8_t c, const uint8_t *bitmap) {
  for (int i = 0; i < 8; i++) {
    if ((CGRAM[c * 8 + i] & 0x1F) != bitmap[i]) {
      return false;
    }
  }
  return true;
}

inline void writeLineBufferByte(char *buffer, uint8_t c, int *j) {
  if (c != buffer[*j]) {
    changetime = millis();
    buffer[*j] = (char)c;
  }
  (*j)++;
}

void writeLineBufferCharacter(char *buffer, int i, int *j) {
  uint8_t c = DDRAM[i];

  if (c == LCD_STR_ARROW_2_DOWN || c == LCD_STR_ARROW_2_DOWN_OLD) {
    if (isBitmapInCgram(c, lcd_chardata_arr2down)) {
      // Downwards Paired Arrows
      writeLineBufferByte(buffer, 226, j);
      writeLineBufferByte(buffer, 135, j);
      writeLineBufferByte(buffer, 138, j);

      // Downwards Triangle-Headed Paired Arrows
      // Missing on iOS
      // writeLineBufferByte(buffer, 226, j);
      // writeLineBufferByte(buffer, 174, j);
      // writeLineBufferByte(buffer, 135, j);
      return;
    }
  }

  if (c == LCD_STR_CONFIRM || c == LCD_STR_CONFIRM_OLD) {
    if (isBitmapInCgram(c, lcd_chardata_confirm)) {
      // Check Mark
      writeLineBufferByte(buffer, 226, j);
      writeLineBufferByte(buffer, 156, j);
      writeLineBufferByte(buffer, 147, j);
      return;
    }
  }

  switch(c) {
    case 32:
      // No-Break Space
      writeLineBufferByte(buffer, 194, j);
      writeLineBufferByte(buffer, 160, j);
      return;
    case LCD_STR_BEDTEMP:
      // Negative Squared Latin Capital Letter H
      writeLineBufferByte(buffer, 240, j);
      writeLineBufferByte(buffer, 159, j);
      writeLineBufferByte(buffer, 133, j);
      writeLineBufferByte(buffer, 183, j);

      // Squared Latin Capital Letter H
      // writeLineBufferByte(buffer, 240, j);
      // writeLineBufferByte(buffer, 159, j);
      // writeLineBufferByte(buffer, 132, j);
      // writeLineBufferByte(buffer, 183, j);

      // Square with Orthogonal Crosshatch Fill
      // writeLineBufferByte(buffer, 226, j);
      // writeLineBufferByte(buffer, 150, j);
      // writeLineBufferByte(buffer, 166, j);
      return;
    case LCD_STR_DEGREE:
      // Degree Sign
      writeLineBufferByte(buffer, 194, j);
      writeLineBufferByte(buffer, 176, j);
      return;
    case LCD_STR_THERMOMETER:
      // Thermometer
      writeLineBufferByte(buffer, 240, j);
      writeLineBufferByte(buffer, 159, j);
      writeLineBufferByte(buffer, 140, j);
      writeLineBufferByte(buffer, 161, j);
      // Variation Selector-15 (text)
      writeLineBufferByte(buffer, 239, j);
      writeLineBufferByte(buffer, 184, j);
      writeLineBufferByte(buffer, 142, j);
      return;
    case LCD_STR_UPLEVEL:
      // Rightwards Arrow with Tip Upwards
      writeLineBufferByte(buffer, 226, j);
      writeLineBufferByte(buffer, 172, j);
      writeLineBufferByte(buffer, 143, j);

      // Rightwards Triangle-Headed Arrow with Long Tip Upwards
      // Missing on iOS
      // writeLineBufferByte(buffer, 226, j);
      // writeLineBufferByte(buffer, 174, j);
      // writeLineBufferByte(buffer, 165, j);
      return;
    case LCD_STR_REFRESH:
      // Clockwise Gapped Circle Arrow
      writeLineBufferByte(buffer, 226, j);
      writeLineBufferByte(buffer, 159, j);
      writeLineBufferByte(buffer, 179, j);

      // Clockwise Right and Left Semicircle Arrows
      // Missing on iOS
      // writeLineBufferByte(buffer, 240, j);
      // writeLineBufferByte(buffer, 159, j);
      // writeLineBufferByte(buffer, 151, j);
      // writeLineBufferByte(buffer, 152, j);
      return;
    case LCD_STR_FOLDER:
      // File Folder Emoji
      writeLineBufferByte(buffer, 240, j);
      writeLineBufferByte(buffer, 159, j);
      writeLineBufferByte(buffer, 147, j);
      writeLineBufferByte(buffer, 129, j);
      // Variation Selector-15 (text)
      writeLineBufferByte(buffer, 239, j);
      writeLineBufferByte(buffer, 184, j);
      writeLineBufferByte(buffer, 142, j);

      // Folder
      // Missing on iOS
      // writeLineBufferByte(buffer, 240, j);
      // writeLineBufferByte(buffer, 159, j);
      // writeLineBufferByte(buffer, 151, j);
      // writeLineBufferByte(buffer, 128, j);
      return;
    case LCD_STR_FEEDRATE:
      // Open-Outlined Rightwards Arrow
      writeLineBufferByte(buffer, 226, j);
      writeLineBufferByte(buffer, 158, j);
      writeLineBufferByte(buffer, 190, j);

      // Rightwards Triangle-Headed Paired Arrows
      // Missing on iOS
      // writeLineBufferByte(buffer, 226, j);
      // writeLineBufferByte(buffer, 174, j);
      // writeLineBufferByte(buffer, 134, j);
      return;
    case LCD_STR_CLOCK:
      // Clock Face Two Oclock
      writeLineBufferByte(buffer, 240, j);
      writeLineBufferByte(buffer, 159, j);
      writeLineBufferByte(buffer, 149, j);
      writeLineBufferByte(buffer, 145, j);
      // Variation Selector-15 (text)
      writeLineBufferByte(buffer, 239, j);
      writeLineBufferByte(buffer, 184, j);
      writeLineBufferByte(buffer, 142, j);
      return;
    case LCD_STR_ARROW_RIGHT:
      // Rightwards Arrow
      writeLineBufferByte(buffer, 226, j);
      writeLineBufferByte(buffer, 134, j);
      writeLineBufferByte(buffer, 146, j);

      // Rightwards Triangle-Headed Arrow
      // Replaced for consistency with Rightwards Arrow with Tip Upwards
      // writeLineBufferByte(buffer, 226, j);
      // writeLineBufferByte(buffer, 173, j);
      // writeLineBufferByte(buffer, 162, j);
      return;
    case LCD_STR_SOLID_BLOCK:
      // Full Block
      writeLineBufferByte(buffer, 226, j);
      writeLineBufferByte(buffer, 150, j);
      writeLineBufferByte(buffer, 136, j);
      return;
    default:
      writeLineBufferByte(buffer, c, j);
      return;
  }
}

class HD44780Display : public PollingComponent {
  char lineBuffer0[LINE_LENGTH * MAX_UTF8_CHAR_LENGTH + 1] = { 0 };
  char lineBuffer1[LINE_LENGTH * MAX_UTF8_CHAR_LENGTH + 1] = { 0 };
  char lineBuffer2[LINE_LENGTH * MAX_UTF8_CHAR_LENGTH + 1] = { 0 };
  char lineBuffer3[LINE_LENGTH * MAX_UTF8_CHAR_LENGTH + 1] = { 0 };

  public:
    TextSensor *line0 = new TextSensor();
    TextSensor *line1 = new TextSensor();
    TextSensor *line2 = new TextSensor();
    TextSensor *line3 = new TextSensor();

    HD44780Display() : PollingComponent(100) { }

    void setup() override {
      setup1();
    }
    void update() override {
      int j = 0;
      for (int i = 0; i < 20; i++) {
        writeLineBufferCharacter(lineBuffer0, i, &j);
      }
      lineBuffer0[j] = 0;
      
      j = 0;
      for (int i = 64; i < 84; i++) {
        writeLineBufferCharacter(lineBuffer1, i, &j);
      }
      lineBuffer1[j] = 0;

      j = 0;
      for (int i = 20; i < 40; i++) {
        writeLineBufferCharacter(lineBuffer2, i, &j);
      }
      lineBuffer2[j] = 0;

      j = 0;
      for (int i = 84; i < 104; i++) {
        writeLineBufferCharacter(lineBuffer3, i, &j);
      }
      lineBuffer3[j] = 0;

      // if (changetime > 0 && changetime < millis() - 500) {
      if (changetime > 0) {
        // char buffer[100];
        // Serial.println("DDRAM:");
        // for (int i = 0; i < DDRAM_SIZE; i++) {
        //   sprintf(buffer, "%d = %d (%c)", i, DDRAM[i], DDRAM[i]);
        //   Serial.println(buffer);
        // }

        line0->publish_state(lineBuffer0);
        line1->publish_state(lineBuffer1);
        line2->publish_state(lineBuffer2);
        line3->publish_state(lineBuffer3);
        changetime = 0;

        // Serial.println("CGRAM:");
        // for (int a = 0; a < 8; a++) {
        //   Serial.print(a);
        //   Serial.println(": ");
        //   for (int i = 0; i < 8; i++) {
        //     char m = 16;
        //     char x = CGRAM[a * 8 + i];
        //     for (int j = 0; j < 5; j++) {
        //       if (x & m) {
        //         Serial.print('x');
        //       } else {
        //         Serial.print(' ');
        //       }
        //       m >>= 1;
        //     }
        //     Serial.println();
        //   }
        // }
      }
    }
};
