#include <Regexp.h>

MatchState ms2;
unsigned long lastSendTime = 0;
const unsigned long sendEchoDelay = 500; // Time in milliseconds to ignore echoed data

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");

  // SDI-12
  Serial1.begin(1200, SERIAL_7E1); // SDI-12 UART configuration
  pinMode(7, OUTPUT); // DIRO Pin

  // DIRO Pin LOW to Send to SDI-12
  digitalWrite(7, LOW);
  writeToSDI12("HelloWorld"); // Send initial command to the sensor

  // Set DIRO Pin HIGH to Receive from SDI-12
  digitalWrite(7, HIGH);
}

void loop() {
  //unsigned long currentTime = millis();

  // Read data if it's been long enough since the last send to ignore echoes
  if (Serial1.available() ) {
    String input = Serial1.readStringUntil('\n'); // Read until newline character
    char buf[input.length()]; // Buffer to hold the incoming data
    memset(buf, 0, sizeof(buf)); // Initialize the buffer with zeroes

    // Copy the string into the buffer starting from the second character
    for (int i = 1; i < input.length(); i++) {
      buf[i - 1] = input[i];
    }
    
    // Print the received input for debugging
    Serial.println(input);

    // Debugging: print each character in hex starting from the first copied character
    for (int i = 0; i < input.length() - 1; i++) {
      Serial.print("0x");
      Serial.println(buf[i], HEX);
    }

    // Target the buffer for regex matching
    ms2.Target(buf);

    // Match the pattern
    if (ms2.Match(".*a!") == REGEXP_MATCHED) {
      writeToSDI12("Working");
    } else {
      Serial.println("Invalid input");
    }
    Serial.println("Enter a string:");
  }
}

void writeToSDI12(String data) {
  //lastSendTime = millis(); // Update the last send time

  // DIRO Pin LOW to Send to SDI-12
  digitalWrite(7, LOW);
  Serial1.println(data);
  Serial1.flush(); // Ensure all outgoing data has been transmitted

  // Clear any data that may have been echoed back
  while(Serial1.available() > 0) {
      char junk = Serial1.read(); // Read and discard the incoming byte
      // Optionally, print the junk data for debugging
      //Serial.print("Discarded byte: ");
      //Serial.println(junk, HEX);
  }

  delay(100); // Additional delay if necessary, adjust as needed

  // DIRO Pin HIGH to Receive from SDI-12
  digitalWrite(7, HIGH);
}

