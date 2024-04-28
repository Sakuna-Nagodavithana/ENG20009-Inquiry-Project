const int buttonPins[] = {2, 3, 4, 5};
const int numButtons = 4;
volatile int buttonValues[] = {0, 0, 0, 0};

void buttonPressed(int index) {
  buttonValues[index]++;
}

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP); // Enable internal pull-up resistor
    // Configure PIO for rising edge detection
    REG_PIOC_ABSR &= ~(1 << (buttonPins[i] - 2)); // Set pin to peripheral A (PIO Controller)
    REG_PIOC_PUER |= (1 << (buttonPins[i] - 2));  // Enable pull-up resistor
    REG_PIOC_ODR |= (1 << (buttonPins[i] - 2)); // Disable open-drain mode
    REG_PIOC_PER |= (1 << (buttonPins[i] - 2)); // Enable PIO control
    REG_PIOC_IER |= (1 << (buttonPins[i] - 2)); // Enable interrupt
    REG_PIOC_ESR |= (1 << (buttonPins[i] - 2)); // Rising edge detection
  }
}

void loop() {
  // Do other tasks here
  for (int i = 0; i < numButtons; i++) {
    Serial.print(i); Serial.print(" - "); Serial.println(buttonValues[i]);
  }
  delay(1000); // Example delay
}

// PIO Controller interrupt handler
void PIO_Handler() {
  for (int i = 0; i < numButtons; i++) {
    if ((REG_PIOC_PDSR & (1 << (buttonPins[i] - 2))) != 0) {
      // Button pressed, increment value
      buttonPressed(i);
    }
  }
  REG_PIOC_ISR; // Clear interrupt status register
}
