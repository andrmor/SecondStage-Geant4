#include "SensitiveDetectorScint.hh"
#include "SessionManager.hh"

#include <sstream>

#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

//#include <QDebug>

SensitiveDetectorScint::SensitiveDetectorScint(const G4String & name)
    : G4VSensitiveDetector(name) {}

G4bool SensitiveDetectorScint::ProcessHits(G4Step* aStep, G4TouchableHistory*)
{  
    const G4double edep = aStep->GetTotalEnergyDeposit()/keV;
    if (edep == 0) return false;

    SessionManager & SM = SessionManager::getInstance();
    const G4StepPoint * postStep = aStep->GetPostStepPoint();
    const G4ThreeVector & pos = postStep->GetPosition();

    /*
    qDebug() << "Particle: " << aStep->GetTrack()->GetParticleDefinition()->GetParticleName()
             << "volume: " << postStep->GetPhysicalVolume()->GetName()
             << "index: " << postStep->GetPhysicalVolume()->GetCopyNo()
             << "Energy: " << postStep->GetKineticEnergy()/keV
             << "Time: " << postStep->GetGlobalTime();
    */

    std::stringstream text;  // output format: Index Particle EnergyDeposition[keV] X[mm] Y[mm] Z[mm] Time[ns]
    text << postStep->GetPhysicalVolume()->GetCopyNo() << ' '
         << aStep->GetTrack()->GetParticleDefinition()->GetParticleName() << ' '
         << edep << ' '
         << pos[0] << ' ' << pos[1] << ' ' << pos[2] << ' '
         << postStep->GetGlobalTime()/ns;

    SM.sendLineToOutput(text);

    return true;
}
