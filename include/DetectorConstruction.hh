#ifndef DetectorConstruction_H
#define DetectorConstruction_H

#include "G4VUserDetectorConstruction.hh"

class G4VPhysicalVolume;

class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
     G4VPhysicalVolume* Construct();
};

#endif //DetectorConstruction_H
