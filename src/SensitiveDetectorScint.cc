#include "SensitiveDetectorScint.hh"
#include "SessionManager.hh"

#include <sstream>

#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

SensitiveDetectorScint::SensitiveDetectorScint(const G4String & name)
    : G4VSensitiveDetector(name) {}

G4bool SensitiveDetectorScint::ProcessHits(G4Step* aStep, G4TouchableHistory*)
{  
    const G4double edep = aStep->GetTotalEnergyDeposit()/keV;
    if (edep == 0) return false;

    SessionManager & SM = SessionManager::getInstance();
    const G4StepPoint * postStep = aStep->GetPostStepPoint();

    const double time = postStep->GetGlobalTime()/ns;
    if (time > SM.TimeLimit) return true;

    const G4ThreeVector & pos = postStep->GetPosition();

    /*
    std::cout << "Particle: " << aStep->GetTrack()->GetParticleDefinition()->GetParticleName()
              << "volume: " << postStep->GetPhysicalVolume()->GetName()
              << "index: " << postStep->GetPhysicalVolume()->GetCopyNo()
              << "Energy: " << postStep->GetKineticEnergy()/keV
              << "Time: " << postStep->GetGlobalTime()
              << "XYZ:" << pos[0] << ' ' << pos[1] << ' ' << pos[2] << std::endl;
    */

    double posArr[3];
    posArr[0] = pos[0]/mm;
    posArr[1] = pos[1]/mm;
    posArr[2] = pos[2]/mm;

    SM.saveRecord_Scint(aStep->GetTrack()->GetParticleDefinition()->GetParticleName(),
                        postStep->GetPhysicalVolume()->GetCopyNo(),
                        edep,
                        time,
                        posArr);

    return true;
}
