#include "SessionManager.hh"
#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "QGSP_BIC_HP.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

int main(int argc, char** argv)
{
    SessionManager& SM = SessionManager::getInstance();

    SM.bGuiMode = false;
    SM.DetectorType = SessionManager::IdealDetectors;  // output format: Index Particle Energy[keV] Time[ns]
    //SM.DetectorType = SessionManager::Scintillators;    // output format: Index Particle EnergyDeposition[keV] Time[ns] X[mm] Y[mm] Z[mm]

    long Seed = 111111;

    SM.FileName_Input  = "/home/andr/tmp/2stages/Target-1e8-0-111111-c2.txt";
    SM.FileName_Output = "/home/andr/tmp/2stages/SecStage_Ideal_111111_of_Target-1e8-0-111111-c2.txt";

    SM.TimeLimit = 3.13e6; // ignore all particles appearing 0.00313+ ms after the start of irradiation
    SM.OutputPrecision = 8;

    CLHEP::RanecuEngine* randGen = new CLHEP::RanecuEngine();
    randGen->setSeed(Seed);
    G4Random::setTheEngine(randGen);

    G4UIExecutive* ui =  0;
    if (SM.bGuiMode) ui = new G4UIExecutive(argc, argv);

    G4RunManager* runManager = new G4RunManager;

    DetectorConstruction * theDetector = new DetectorConstruction();
    runManager->SetUserInitialization(theDetector);

    G4VModularPhysicsList* physicsList = new QGSP_BIC_HP;
    physicsList->RegisterPhysics(new G4StepLimiterPhysics());
    runManager->SetUserInitialization(physicsList);

    runManager->SetUserInitialization(new ActionInitialization());

    G4UImanager* UImanager = G4UImanager::GetUIpointer();
    UImanager->ApplyCommand("/run/initialize");
    UImanager->ApplyCommand("/control/verbose 0");
    UImanager->ApplyCommand("/run/verbose 0");
    if (SM.bGuiMode)
    {
        UImanager->ApplyCommand("/hits/verbose 2");
        UImanager->ApplyCommand("/tracking/verbose 2");
        UImanager->ApplyCommand("/control/saveHistory");
    }

    UImanager->ApplyCommand("/run/setCut 0.1 mm");
    UImanager->ApplyCommand("/process/em/fluo true");
    UImanager->ApplyCommand("/process/em/auger true");
    UImanager->ApplyCommand("/process/em/augerCascade true");
    UImanager->ApplyCommand("/process/em/pixe true");
    UImanager->ApplyCommand("/process/em/deexcitationIgnoreCut false");

    UImanager->ApplyCommand("/run/initialize");

    SM.startSession();

    G4VisManager* visManager = 0;

    if (SM.bGuiMode)
    {
        visManager = new G4VisExecutive("Quiet");
        visManager->Initialize();
        UImanager->ApplyCommand("/control/execute vis.mac");
        ui->SessionStart();
    }
    else
    {
        SM.runSimulation();
    }

    delete visManager;
    delete runManager;
    delete ui;

    SM.endSession();
}
