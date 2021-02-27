#define CLK_PIN 13  // or SCK
#define DATA_PIN 11 // or MOSI
#define CS_PIN 10   // or SS

#define JOYSTICK_X (0)
#define JOYSTICK_Y (1)
#define JOYSTICK_SW (30)
#define JOYSTICK_DEADZONE (1024 * .10)
#define JOYSTICK_NEUTRAL (512)
#define JOYSTICK_LOW_THRESHOLD (JOYSTICK_NEUTRAL - JOYSTICK_DEADZONE)
#define JOYSTICK_HIGH_THRESHOLD (JOYSTICK_NEUTRAL + JOYSTICK_DEADZONE)

#define LCD_D0 (40)
#define LCD_D1 (41)
#define LCD_D2 (42)
#define LCD_D3 (43)
#define LCD_D4 (44)
#define LCD_D5 (45)
#define LCD_D6 (46)
#define LCD_D7 (47)

// DATA LATCH - LATCH ON RISE, EXEC ON FALL
#define LCD_LATCH (31)
// READ/WRITE SELECT
#define LCD_RW (32)
// REGISTER SELECT - DATA OR INSTRUCTION
#define LCD_RS (33)

#define NUM_COLUMNS (8)
#define NUM_ROWS (8)

template <typename T> struct Point {
    T x;
    T y;
    Point<T>(const T &xIn, const T &yIn) : x(xIn), y(yIn) {}
    Point &operator+(const Point &rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    Point &operator-(const Point &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
};

struct LedDisplay {
  private:
    uint8_t pinClock;
    uint8_t pinData;
    uint8_t pinChipSelect;
    uint8_t data[8];

  public:
    LedDisplay(uint8_t pinClock, uint8_t pinData, uint8_t pinChipSelect)
        : pinClock(pinClock), pinData(pinData), pinChipSelect(pinChipSelect) {}

    void writeByte(uint8_t data) {
        digitalWrite(CS_PIN, LOW);
        for (uint8_t i = 0; i < 8; i++) {
            digitalWrite(pinClock, LOW);
            digitalWrite(pinData, data & 0x80);
            data <<= 1;
            digitalWrite(pinClock, HIGH);
        }
    }

    void writeAddressAndByte(uint8_t addr, uint8_t data) {
        digitalWrite(pinChipSelect, LOW);
        writeByte(addr);
        writeByte(data);
        digitalWrite(pinChipSelect, HIGH);
    }

    void clear() { memset(data, 0, sizeof(data)); }

    void init() {
        pinMode(pinChipSelect, OUTPUT);
        pinMode(pinData, OUTPUT);
        pinMode(pinClock, OUTPUT);
        analogReference(EXTERNAL);
        writeAddressAndByte(0x09, 0x00); // decoding BCD
        writeAddressAndByte(0x0a, 0x03); // brightness
        writeAddressAndByte(0x0b, 0x07); // scanlimit；8LEDs
        writeAddressAndByte(0x0c,
                            0x01); // power-downmode：0，normalmode：1
        writeAddressAndByte(0x0f,
                            0x00); // testdisplay：1；EOT，display：0
        clear();
    }

    void setPixel(uint8_t x, uint8_t y, bool state) {
        if (state) {
            data[y] |= (1 << ((NUM_COLUMNS - 1) - x));
        } else {
            data[y] &= ~(1 << ((NUM_COLUMNS - 1) - x));
        }
    }

    template <typename T> void setPixel(const T &point, bool state) {
        setPixel(point.x, point.y, state);
    }

    void update() {
        for (auto i = 0; i < 8; i++) {
            writeAddressAndByte((uint8_t)(i + 1), data[i]);
        }
    }
};

struct LcdDisplay {

    enum RwMode { READ, WRITE };
    enum CmdMode { DATA, INSTRUCTION };
    enum AddrMode { INCREMENT, DECREMENT };
    enum FontSize { SMALL, LARGE };

  private:
    //! Data pins 0-7
    uint8_t pinD0;
    uint8_t pinD1;
    uint8_t pinD2;
    uint8_t pinD3;
    uint8_t pinD4;
    uint8_t pinD5;
    uint8_t pinD6;
    uint8_t pinD7;
    //! Register select
    uint8_t pinRs;
    //! Read/write
    uint8_t pinRw;
    //! Clock/latch pin
    uint8_t pinLatch;
    //! Number of lines currently configured for
    uint8_t numLines = 2;
    //! Font size configured for
    FontSize fontType = SMALL;
    bool showCursor = true;
    bool blinkCursor = true;
    bool displayOn = true;

  public:
    LcdDisplay(uint8_t pinD0, uint8_t pinD1, uint8_t pinD2, uint8_t pinD3,
               uint8_t pinD4, uint8_t pinD5, uint8_t pinD6, uint8_t pinD7,
               uint8_t pinRs, uint8_t pinRw, uint8_t pinLatch)
        : pinD0(pinD0), pinD1(pinD1), pinD2(pinD2), pinD3(pinD3), pinD4(pinD4),
          pinD5(pinD5), pinD6(pinD6), pinD7(pinD7), pinRs(pinRs), pinRw(pinRw),
          pinLatch(pinLatch) {}

    void muxRw(RwMode mode) {
        switch (mode) {
            case RwMode::READ: {
                digitalWrite(LCD_RW, HIGH);
                return;
            }
            case RwMode::WRITE: {
                digitalWrite(LCD_RW, LOW);
                return;
            }
        }
    }

    void muxCmd(CmdMode mode) {
        switch (mode) {
            case CmdMode::DATA: {
                digitalWrite(LCD_RS, HIGH);
                return;
            }
            case CmdMode::INSTRUCTION: {
                digitalWrite(LCD_RS, LOW);
                return;
            }
        }
    }

    void writeByte(uint8_t data) {
        digitalWrite(pinD0, ((data)&0x01));
        digitalWrite(pinD1, ((data >> 1) & 0x01));
        digitalWrite(pinD2, ((data >> 2) & 0x01));
        digitalWrite(pinD3, ((data >> 3) & 0x01));
        digitalWrite(pinD4, ((data >> 4) & 0x01));
        digitalWrite(pinD5, ((data >> 5) & 0x01));
        digitalWrite(pinD6, ((data >> 6) & 0x01));
        digitalWrite(pinD7, ((data >> 7) & 0x01));
        _NOP();
        digitalWrite(LCD_LATCH, HIGH);
        _NOP();
        _NOP();
        _NOP();
        _NOP();
        _NOP();
        _NOP();
        _NOP();
        digitalWrite(LCD_LATCH, LOW);
    }

    void goHome() {}

    void muxPins() {
        pinMode(pinD0, OUTPUT);
        pinMode(pinD1, OUTPUT);
        pinMode(pinD2, OUTPUT);
        pinMode(pinD3, OUTPUT);
        pinMode(pinD4, OUTPUT);
        pinMode(pinD5, OUTPUT);
        pinMode(pinD6, OUTPUT);
        pinMode(pinD7, OUTPUT);
        pinMode(pinLatch, OUTPUT);
        pinMode(pinRs, OUTPUT);
        pinMode(pinRw, OUTPUT);
    }

    void init() {
        muxPins();
        delay(1);
        updateControl();
        updateFont();
    }

  private:
    void updateControl() {
        muxCmd(INSTRUCTION);
        muxRw(WRITE);
        uint8_t command = 0x08;
        if (displayOn) {
            command |= 0x04;
        }
        if (showCursor) {
            command |= 0x02;
        }
        if (blinkCursor) {
            command |= 0x01;
        }
        writeByte(command);
        delay(1);
    }

    void updateFont() {
        muxCmd(INSTRUCTION);
        muxRw(WRITE);
        // 8 bit interface
        uint8_t command = 0x30;
        if (numLines == 2) {
            command |= 0x08;
        }
        if (fontType == LARGE) {
            command |= 0x04;
        }
        writeByte(command);
    }

  public:
    void power(bool on) {
        displayOn = on;
        updateControl();
    }
    void lines(uint8_t nlines) {
        numLines = nlines;
        updateFont();
    }
    void fontSize(FontSize size) {
        fontType = size;
        updateFont();
    }
    void writeText(uint8_t line, uint8_t pos, const char *str) {
        muxCmd(INSTRUCTION);
        muxRw(WRITE);
        uint8_t command = 0x80;
        if (line) {
            command |= 0x40;
        }
        command |= min(pos, 0x27);
        // move cursor to line, pos
        writeByte(command);
        muxCmd(DATA);
        // write text
        for (auto i = 0u; i < min(strlen(str), 40 - pos); i++) {
            writeByte(str[i]);
        }
    }
};

auto player = Point<int>(3, 3);
auto lcdDisplay = LcdDisplay(LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5,
                             LCD_D6, LCD_D7, LCD_RS, LCD_RW, LCD_LATCH);
auto ledDisplay = LedDisplay(CLK_PIN, DATA_PIN, CS_PIN);
const char *HELLO_WORLD_STR = "HELLO WORLD";
const auto HELLO_WORLD_LEN = strlen(HELLO_WORLD_STR);
auto helloWorldIndex = 0;
auto lastStart = millis();

void setup() {
    pinMode(JOYSTICK_SW, INPUT);
    ledDisplay.init();
    lcdDisplay.init();
    lcdDisplay.writeText(0, 5, "Howdy");
    lcdDisplay.writeText(1, 5, "Partner");
}

void loop() {
    if (millis() > lastStart + 250) {
        // get start of this frame
        lastStart = millis();
        // clear current framebuffer
        ledDisplay.clear();
        // read current joystick values (512 is neutral)
        auto joystickX = analogRead(JOYSTICK_X);
        auto joystickY = analogRead(JOYSTICK_Y);
        if (joystickX > JOYSTICK_HIGH_THRESHOLD &&
            (player.x < (NUM_COLUMNS - 1))) {
            player.x++;
        } else if (joystickX < JOYSTICK_LOW_THRESHOLD && (player.x > 0)) {
            player.x--;
        }
        if (joystickY > JOYSTICK_HIGH_THRESHOLD &&
            (player.y < (NUM_ROWS - 1))) {
            player.y++;
        } else if (joystickY < JOYSTICK_LOW_THRESHOLD && (player.y > 0)) {
            player.y--;
        }
        // highlight player
        ledDisplay.setPixel(player, true);
        // draw current framebuffer to display
        ledDisplay.update();
        return;
    }
}
