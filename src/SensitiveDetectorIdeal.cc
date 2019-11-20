#include "SensitiveDetectorIdeal.hh"
#include "SessionManager.hh"

#include <sstream>

#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

#include <QDebug>

SensitiveDetectorIdeal::SensitiveDetectorIdeal(const G4String & name)
    : G4VSensitiveDetector(name) {}

G4bool SensitiveDetectorIdeal::ProcessHits(G4Step* aStep, G4TouchableHistory*)
{  
    SessionManager & SM = SessionManager::getInstance();

    // this will trigger on exit from this volume

    const G4StepPoint * preStep = aStep->GetPreStepPoint();

    /*
    qDebug() << "Particle: " << aStep->GetTrack()->GetParticleDefinition()->GetParticleName()
             << "volume: " << preStep->GetPhysicalVolume()->GetName()
             << "index: " << preStep->GetPhysicalVolume()->GetCopyNo()
             << "Energy: " << preStep->GetKineticEnergy()/keV
             << "Time: " << preStep->GetGlobalTime();
    */

    std::stringstream text;
    text << preStep->GetPhysicalVolume()->GetCopyNo() << ' '
         << aStep->GetTrack()->GetParticleDefinition()->GetParticleName() << ' '
         << preStep->GetKineticEnergy()/keV << ' '
         << preStep->GetGlobalTime()/ns;

    SM.sendLineToOutput(text); // format: Index Particle Energy[keV] Time[ns]

    aStep->GetTrack()->SetTrackStatus(fStopAndKill);
    return true;
}
