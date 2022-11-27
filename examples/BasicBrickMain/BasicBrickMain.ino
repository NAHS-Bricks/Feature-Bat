#include <Arduino.h>
#include <nahs-Bricks-OS.h>
// include all features of brick
#include <nahs-Bricks-Feature-Bat.h>

void setup() {
  // Now register all the features under All
  // Note: the order of registration is the same as the features are handled internally by All
  FeatureAll.registerFeature(&FeatureBat);

  // Set Brick-Specific stuff
  BricksOS.setSetupPin(D5);
  FeatureAll.setBrickType(1);

  // Set Brick-Specific (feature related) stuff
  FeatureBat.setPins(D6, D7, A0);

  // Finally hand over to BrickOS
  BricksOS.handover();
}

void loop() {
  // Not used on Bricks
}