#include "SensitiveDetectorIdeal.hh"
#include "SessionManager.hh"

#include <sstream>

#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

SensitiveDetectorIdeal::SensitiveDetectorIdeal(const G4String & name)
    : G4VSensitiveDetector(name) {}

G4bool SensitiveDetectorIdeal::ProcessHits(G4Step* aStep, G4TouchableHistory*)
{  
    SessionManager & SM = SessionManager::getInstance();

    // this will be triggered on exit from the sensitive detector

    const G4StepPoint * preStep = aStep->GetPreStepPoint();

    /*
    std::cout << "Particle: " << aStep->GetTrack()->GetParticleDefinition()->GetParticleName()
              << "volume: " << preStep->GetPhysicalVolume()->GetName()
              << "index: " << preStep->GetPhysicalVolume()->GetCopyNo()
              << "Energy: " << preStep->GetKineticEnergy()/keV
              << "Time: " << preStep->GetGlobalTime() << std::endl;
    */

    const double time = preStep->GetGlobalTime()/ns;
    if (time > SM.TimeLimit) return true;

    SM.saveRecord_Ideal(aStep->GetTrack()->GetParticleDefinition()->GetParticleName(),
                        preStep->GetPhysicalVolume()->GetCopyNo(),
                        preStep->GetKineticEnergy()/keV,
                        time);

    aStep->GetTrack()->SetTrackStatus(fStopAndKill);
    return true;
}
