const int POTI_PIN = ANALOG_1;

void setup() {

}

void loop() {
    int value = analogRead(POTI_PIN);
    send(value);
}