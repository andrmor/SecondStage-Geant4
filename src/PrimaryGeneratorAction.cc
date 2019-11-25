#include "PrimaryGeneratorAction.hh"
#include "SessionManager.hh"

#include "G4ParticleGun.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : G4VUserPrimaryGeneratorAction()
{
    fParticleGun = new G4ParticleGun(1);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event * anEvent)
{
    SessionManager & SM = SessionManager::getInstance();
    const std::vector<ParticleRecord> & GeneratedPrimaries = SM.getNextEventPrimaries();

    //std::cout << '#' << SM.NextEventId << std::endl;

    for (const ParticleRecord & r : GeneratedPrimaries)
    {
        //std::cout << r.Particle->GetParticleName()
        //          <<"  E:"<< r.Energy <<"  T:"<< r.Time
        //          <<"  Pos:"<< r.Position[0]  << ' ' << r.Position[1]  << ' ' << r.Position[2]
        //          <<"  Dir:"<< r.Direction[0] << ' ' << r.Direction[1] << ' ' << r.Direction[2] << std::endl;

        fParticleGun->SetParticleDefinition(r.Particle);
        fParticleGun->SetParticlePosition(r.Position); //position in millimeters - no need units
        fParticleGun->SetParticleMomentumDirection(r.Direction);
        fParticleGun->SetParticleEnergy(r.Energy*keV);
        fParticleGun->SetParticleTime(r.Time*ns);

        fParticleGun->GeneratePrimaryVertex(anEvent);
    }
}
