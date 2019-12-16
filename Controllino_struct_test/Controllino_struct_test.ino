typedef struct NAMELIST {
  char name[10];
  int8_t ads0Channels[4];
  int deviceID;
};

NAMELIST nameList[5] = {
  {"1st name", {0, 1, 2, 3}, 1},
  {"2nd name", {0, 1, 2, 3}, 2},
  {"3rd name", {0, 1, 2, 3}, 3},
  {"4th name", {0, 1, 2, 3}, 4},
  {"5th name", {0, 1, 2, 3}, 5},
};

void setup() {
  Serial.begin(115200);

  // Wait for USB Serial
  while (!Serial) {
    ;
  }
  Serial.println(nameList[0].name);




}

void loop() {

  int8_t arraySize = sizeof(nameList) / sizeof(nameList[0]);


  for (int i = 0; i < arraySize; i++) {
    Serial.print(nameList[i].name); Serial.print(", ");
    int8_t arraySize1 = sizeof(nameList[i].ads0Channels) / sizeof(nameList[i].ads0Channels);
    Serial.println(arraySize1);
    for (int j = 0; j < arraySize1; j++) { // Always 0 ???s
      Serial.print(nameList[i].ads0Channels[j]); Serial.print(", ");
    }
    Serial.println();
  }


}
