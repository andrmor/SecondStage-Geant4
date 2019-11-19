#ifndef SensitiveDetectorIdeal_h
#define SensitiveDetectorIdeal_h

#include "G4VSensitiveDetector.hh"

class G4Step;
class G4HCofThisEvent;

class SensitiveDetectorIdeal : public G4VSensitiveDetector
{
public:
    SensitiveDetectorIdeal(const G4String & name);

    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history);
};

#endif // SensitiveDetectorIdeal_h
