
#include <Regexp.h>

MatchState ms2;
void setup() {
  //Arduino IDE Serial Monitor
  Serial.begin(9600);
  Serial.println("Starting");

  //SDI-12
  Serial1.begin(1200, SERIAL_7E1);      //SDI-12 UART, configures serial port for 7 data bits, even parity, and 1 stop bit
  pinMode(7, OUTPUT);                   //DIRO Pin

  //DIRO Pin LOW to Send to SDI-12
  digitalWrite(7, LOW); 
  Serial1.println("HelloWorld");
  delay(100);

  //HIGH to Receive from SDI-12
  digitalWrite(7, HIGH); 
}

void loop() {
  // //Receive SDI-12 over UART and then print to Serial Monitor
  // while (Serial1.available() == 0) {}
  //   String command = Serial1.readString();
  //   //sdi12.runCommand(command);
  //   Serial.println(command);

  //   char buf[input.length() + 1];
  //   input.toCharArray(buf, input.length() + 1);
  //   ms2.Target(buf);
  //   Serial.println(input);
  //   if (ms2.Match(".+\?!") == REGEXP_MATCHED) {
  //     Serial.println("working");
  //   } else {
  //     Serial.println("Invalid input");
  //   }
  //   Serial.println("Enter a string:");

  if (Serial1.available()) {
  String input = Serial1.readStringUntil('\n'); // Read until newline character
  char buf[input.length() + 1]; // +1 for the null terminator
  //input.toCharArray(buf, input.length() + 1); // Ensure null termination
  Serial.println(input);
  
  // Debugging: print each character in hex
  for (int i = 1; i < input.length(); i++) {
    buf[i] = input[i];
    Serial.println(buf[i]);
    Serial.print("0x");
    Serial.println(buf[i], HEX);
  }
    ms2.Target(buf);
    
    if (ms2.Match(".*a!") == REGEXP_MATCHED) {
      Serial.println("working");
    } else {
      Serial.println("Invalid input");
    }
    Serial.println("Enter a string:");
  }
}

void writeToSDI12(String data){
  //DIRO Pin LOW to Send to SDI-12
  digitalWrite(7, LOW); 
  Serial1.println(data);
  delay(100);

  //HIGH to Receive from SDI-12
  digitalWrite(7, HIGH); 
}