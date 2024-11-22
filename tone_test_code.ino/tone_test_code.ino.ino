#include <MsTimer2.h>
#include <SPI.h>
#include <Tone2.h>

Tone toneT2;
Tone toneT1;

void setup() {

  toneT2.begin(13);
  toneT1.begin(12);
  
  
}

void loop() {
  int lh = 3;

  // abnormal condition
  if(lh == 0){
      toneT1.play(200);
  }
  // low condition
  else if (lh == 1){
    toneT1.play(400);
  }
  // high condition
  else{
    while(1){
      toneT1.play(1000);
      delay(1000);
      toneT1.stop();
      delay(1000);
    }
  }

}
