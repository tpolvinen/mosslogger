/*
   Using struct to define data types and structure,
   then create an array of structs, with the initial data/variable values for each,
   then iterate over the struct array, in turn iterating over arrays in each struct.
*/


typedef struct NAMELIST {
  char name[10];
  int8_t adsChannels[4];
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
  while (!Serial) {
    ;
  }
  for (int repeats = 0; repeats < 3; repeats ++) {

    Serial.print("Repeat "); Serial.println(repeats +1);

    int8_t nameListSize = sizeof(nameList) / sizeof(nameList[0]);

    for (int i = 0; i < nameListSize; i++) {
      Serial.print(nameList[i].name); Serial.print(", ");

      int8_t adsChannelsSize = sizeof(nameList[i].adsChannels) / sizeof(nameList[i].adsChannels[0]);

      Serial.println(adsChannelsSize);

      for (int j = 0; j < adsChannelsSize; j++) {
        Serial.print(nameList[i].adsChannels[j]); Serial.print(", ");
      }
      Serial.println();
      delay(1000);
    }
  }
}

void loop() {

}
