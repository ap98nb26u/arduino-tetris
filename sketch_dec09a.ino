//#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.
// #define SD_CS    4  // Chip select line for SD card
#define TFT_CS  10  // Chip select line for TFT display
#define TFT_DC  9   // Data/command line for TFT
#define TFT_RST 8   // Reset line for TFT (or connect to +5V)

#define JOY_X   A0
#define JOY_Y   A1
#define JOY_BTN 2

// Define colors to loosen coupling to screen implementation
#define COLOR_GRAY      tft.Color565(66, 66, 66)
#define COLOR_WHITE     ST7735_WHITE
#define COLOR_BLACK     ST7735_BLACK
#define COLOR_CYAN      ST7735_CYAN
#define COLOR_YELLOW    ST7735_YELLOW
#define COLOR_BLUE      ST7735_BLUE
#define COLOR_ORANGE    tft.Color565(255, 165, 0)
#define COLOR_LIME      tft.Color565(204, 255, 0)
#define COLOR_PURPLE    tft.Color565(128,0,128)
#define COLOR_RED       ST7735_RED

#define BOARD_COLOR       COLOR_GRAY
#define BACKGROUND_COLOR  COLOR_BLACK

#define BOARD_WIDTH     10
#define BOARD_HEIGHT    20
#define BOARD_OFFSET_X  2
#define BOARD_OFFSET_Y  17

#define MIN(X, Y)       (((X) < (Y)) ? (X) : (Y))
#define BLOCK_SIZE      MIN((tft.width() - 1) / BOARD_WIDTH, (tft.height() - 1) / BOARD_HEIGHT)

#define SHAPE_COUNT     7

#define SHAPE_I         0
#define SHAPE_J         1
#define SHAPE_L         2
#define SHAPE_O         3
#define SHAPE_S         4
#define SHAPE_T         5
#define SHAPE_Z         6

#define SHAPE_I_COLOR   COLOR_CYAN
#define SHAPE_J_COLOR   COLOR_BLUE
#define SHAPE_L_COLOR   COLOR_ORANGE
#define SHAPE_O_COLOR   COLOR_YELLOW
#define SHAPE_S_COLOR   COLOR_LIME
#define SHAPE_T_COLOR   COLOR_PURPLE
#define SHAPE_Z_COLOR   COLOR_RED

#define MOVE_DELAY 85

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
byte currentShape = 0;
short yOffset = -4;
short xOffset = 0;
short lastY = -4;
short lastX = 0;

short level = 300;
uint16_t grid[BOARD_WIDTH][BOARD_HEIGHT];
uint16_t shapeColors[SHAPE_COUNT];
const byte shapes[SHAPE_COUNT][4] = {
  {
    B0100,
    B0100,
    B0100,
    B0100
  }, {
    B0000,
    B0010,
    B0010,
    B0110
  }, {
    B0000,
    B0100,
    B0100,
    B0110
  }, {
    B0000,
    B0000,
    B0110,
    B0110
  }, {
    B0000,
    B0000,
    B0110,
    B1100
  }, {
    B0000,
    B0000,
    B0010,
    B0111
  }, {
    B0000,
    B0000,
    B0110,
    B0011
  }
};

uint16_t getCurrentShapeColor() {
  return shapeColors[currentShape];
}

void fillBlock(byte x, byte y, uint16_t color) {
  grid[x][y] = color;
  tft.fillRect(1 + BOARD_OFFSET_X + (x * BLOCK_SIZE), 1 + BOARD_OFFSET_Y + (y * BLOCK_SIZE), BLOCK_SIZE - 1, BLOCK_SIZE - 1, color);
}

byte getNthBit(byte c, byte n) {
  return ((c & (1 << n)) >> n);
}

bool yOffsetAtBottom() { // TODO: compensate for orientation
  return (4 + yOffset) >= BOARD_HEIGHT;
}

void nextShape() {
  yOffset = -4;
  xOffset = 0;
  currentShape = random(SHAPE_COUNT);
}

void gameOver() {
  tft.setCursor(90, 20);
  tft.print("GAME");
  tft.setCursor(90, 30);
  tft.print("OVER");
  while (true) {
    bool isClicked = (digitalRead(JOY_BTN) == LOW);
    if (isClicked) {
      delay(150);
      isClicked = (digitalRead(JOY_BTN) == LOW);
    }

    if (isClicked) {
      tft.fillRect(90, 20, 25, 20, COLOR_BLACK);
      for (byte i = 0; i < BOARD_WIDTH; i++) {
        for (byte j = 0; j < BOARD_HEIGHT; j++) {
          fillBlock(i, j, COLOR_BLACK);
        }
      }

      nextShape();
      break;
    }

    delay(100);
  }

}

bool isShapeColliding(byte shapeNumber, int xPos, int yPos) {
  if (shapeNumber == SHAPE_Z && grid[xPos + 2][yPos + 3] != COLOR_BLACK) {
    return true;
  }

  for (byte i = 0; i < 4; i++) {
    if ((getNthBit(shapes[shapeNumber][3], i) == 1) && (grid[xPos + i][yPos + 4] != COLOR_BLACK)) {
      if (yPos < -1) {
        gameOver();
      }

      return true;
    }
  }

  return false;
}

void detectCurrentShapeCollision() {
  if (yOffsetAtBottom() || isShapeColliding(currentShape, xOffset, yOffset)) {
    nextShape();
  }
}

void gravity(bool apply) {
  static byte lastXoffset = 0;

  for (byte k = 0; k < 2; k++) {
    for (byte i = 0; i < 4; i++) {
      int x = 0;
      for (byte j = 0; j != 4; j++) {
        if (getNthBit(shapes[currentShape][i], j) == 1) {
          fillBlock((k == 0 ? lastXoffset : xOffset) + x, yOffset + i, k == 0 ? COLOR_BLACK : getCurrentShapeColor());
        }

        x++;
      }
    }

    if (k == 0 && apply) {
      yOffset++;
    }
  }


  if (xOffset != lastXoffset) {
    lastXoffset = xOffset;
  }
}

void drawGrid() {
  for (int i = 0; i < BOARD_HEIGHT + 1; i++) {
    tft.drawFastHLine(BOARD_OFFSET_X, BOARD_OFFSET_Y + (i * BLOCK_SIZE), (BLOCK_SIZE * BOARD_WIDTH), BOARD_COLOR);
  }

  for (int i = 0; i < BOARD_WIDTH + 1; i++) {
    tft.drawFastVLine(BOARD_OFFSET_X + (i * BLOCK_SIZE), BOARD_OFFSET_Y, (BLOCK_SIZE * BOARD_HEIGHT), BOARD_COLOR);
  }
}

void joystickMovement() {
  int joyX = analogRead(JOY_X);
  static unsigned long lastMove = millis();
  //int joyY = analogRead(JOY_Y);

  // left
  if (joyX == 0 && xOffset > -1 && (millis() - lastMove) > MOVE_DELAY) { // todo: allow moving according to tile width
    lastMove = millis();
    xOffset--;
  }

  // right
  if (joyX > 1015 && xOffset < BOARD_WIDTH - 3 && (millis() - lastMove) > MOVE_DELAY) {
    lastMove = millis();
    xOffset++;
  }
}

unsigned long stamp = 0;

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);

  pinMode(JOY_BTN, INPUT_PULLUP);
  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);

  randomSeed(analogRead(A2));

  shapeColors[SHAPE_I] = SHAPE_I_COLOR;
  shapeColors[SHAPE_J] = SHAPE_J_COLOR;
  shapeColors[SHAPE_L] = SHAPE_L_COLOR;
  shapeColors[SHAPE_O] = SHAPE_O_COLOR;
  shapeColors[SHAPE_S] = SHAPE_S_COLOR;
  shapeColors[SHAPE_T] = SHAPE_T_COLOR;
  shapeColors[SHAPE_Z] = SHAPE_Z_COLOR;

  drawGrid();
  nextShape();
  stamp = millis();
}

void loop() {
  if ((millis() - stamp) > level) {
    stamp = millis();
    gravity(true);
  }

  if ((lastX != xOffset) || (lastY != yOffset)) {
    gravity(false);
    detectCurrentShapeCollision();
    lastX = xOffset;
    lastY = yOffset;
  }
  
  joystickMovement();
}

