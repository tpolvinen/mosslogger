#include <Controllino.h>

void setup() {
  pinMode(CONTROLLINO_D0, OUTPUT);
  pinMode(CONTROLLINO_D1, OUTPUT);

}

void loop() {
  digitalWrite(CONTROLLINO_D0, HIGH);
  delay(1000);
  digitalWrite(CONTROLLINO_D0, LOW);
  delay(10);
  digitalWrite(CONTROLLINO_D1, HIGH);
  delay(1000);
  digitalWrite(CONTROLLINO_D1, LOW);
  delay(10);
}
