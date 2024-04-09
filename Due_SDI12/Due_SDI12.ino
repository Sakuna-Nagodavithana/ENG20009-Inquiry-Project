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
  //Receive SDI-12 over UART and then print to Serial Monitor
  if (Serial1.available()) {
    char c = Serial1.read();
    Serial.print( c );
    //Serial1.println(c);
  }

  Serial1.println("HAA");
}