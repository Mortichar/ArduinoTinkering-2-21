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
        writeAddressAndByte(0x09, 0x00); // decoding BCD (binary-coded decimal)
        writeAddressAndByte(0x0a, 0x03); // brightness
        writeAddressAndByte(0x0b, 0x07); // scanlimit；8LEDs
        writeAddressAndByte(0x0c,
                            0x01); // power-downmode：0，normalmode：1
        writeAddressAndByte(0x0f,
                            0x00); // testdisplay：1；EOT，display：0
        clear();
    }

    template <typename T> void setPixel(T x, T y, bool state) {
        if (x >= 0 && x < NUM_COLUMNS && y >= 0 && y < NUM_ROWS) {
            if (state) {
                data[y] |= (1 << ((NUM_COLUMNS - 1) - x));
            } else {
                data[y] &= ~(1 << ((NUM_COLUMNS - 1) - x));
            }
        }
    }

    template <typename T> void setPixel(const T &point, bool state) {
        setPixel(point.x, point.y, state);
    }

    void update() {
        for (auto i = 0; i < NUM_ROWS; i++) {
            writeAddressAndByte((uint8_t)(i + 1), data[i]);
        }
    }
};

struct Square {
    Square(const Point<int16_t> &start, LedDisplay &context)
        : start(start), context(context) {}
    static constexpr int16_t MAX_RADIUS = 8;
    Point<int16_t> start;
    int16_t currentRadius = 0;
    LedDisplay &context;
    void reset(const Point<int16_t> newStart) {
        start = newStart;
        currentRadius = 0;
    }
    void update() {
        if (currentRadius && currentRadius < MAX_RADIUS) {
            for (auto x = start.x - currentRadius; x < start.x + currentRadius;
                 x++) {
                context.setPixel(Point<int16_t>(x, start.y - currentRadius),
                                 false);
                context.setPixel(Point<int16_t>(x, start.y + currentRadius),
                                 false);
            }
            for (auto y = start.y - currentRadius; y < start.y + currentRadius;
                 y++) {
                context.setPixel(Point<int16_t>(start.x - currentRadius, y),
                                 false);
                context.setPixel(Point<int16_t>(start.x + currentRadius, y),
                                 false);
            }
        }
        currentRadius++;
        if (currentRadius < MAX_RADIUS) {
            for (auto x = start.x - currentRadius; x < start.x + currentRadius;
                 x++) {
                context.setPixel(Point<int16_t>(x, start.y - currentRadius),
                                 true);
                context.setPixel(Point<int16_t>(x, start.y + currentRadius),
                                 true);
            }
            for (auto y = start.y - currentRadius; y <= start.y + currentRadius;
                 y++) {
                context.setPixel(Point<int16_t>(start.x - currentRadius, y),
                                 true);
                context.setPixel(Point<int16_t>(start.x + currentRadius, y),
                                 true);
            }
        }
    }
};

auto player = Point<int16_t>(3, 3);
auto ledDisplay = LedDisplay(CLK_PIN, DATA_PIN, CS_PIN);
auto lastStart = millis();
auto squareEntity = Square(player, ledDisplay);

void setup() {
    pinMode(JOYSTICK_SW, INPUT_PULLUP);
    ledDisplay.init();
}
void loop() {
    if (millis() > lastStart + 125) {
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
        auto joystickSw = digitalRead(JOYSTICK_SW);
        if (!joystickSw) {
            squareEntity.reset(player);
        }
        squareEntity.update();
        // highlight player
        ledDisplay.setPixel(player, true);
        // draw current framebuffer to display
        ledDisplay.update();
        return;
    }
}
