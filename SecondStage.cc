#include "SessionManager.hh"
#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "QGSP_BIC_HP.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// File formats
//
// INPUT
//
//  Binary:
//       new event line: 0xEE(char) EventNumber(int)
//       new record:     0xFF(char) ParticleName(string) 0x00(char) Energy(double)[keV] X(double)[mm] Y(double)[mm] Z(double)[mm] DirX(double)[mm] DirY(double)[mm] DirZ(double)[mm] Time(double)[ns]
//  Ascii:
//       new event:      #EventNumber
//       new record:     ParticleName Energy[keV] X[mm] Y[mm] Z[mm] DirX[mm] DirY[mm] DirZ[mm] Time[ns]
//
// OUTPUT
//
//   Ascii:
//     in config with ideal detectors:
//       new event line: #EventNumber
//       new record:     IdelDetIndex ParticleName Energy[keV] Time[ns]
//     in config with scintillators:
//       new event line: #EventNumber
//       new record:     ScintIndex ParticleName EnergyDeposition[keV] Time[ns] X[mm] Y[mm] Z[mm]
//   Binary:
//     in config with ideal detectors:
//          new event line: 0xEE(char) EventNumber(int)
//          new record:     0xFF(char) ScintNumber(int) ParticleEnergy(double)[keV] Time(double)[ns] ParticleName(string) 0x00(char)
//     in config with scintillators:
//          new event line: 0xEE(char) EventNumber(int)
//          new record:     0xFF(char) ScintNumber(int) ParticleEnergy(double)[keV] Time(double)[ns] X(double)[mm] Y(double)[mm] Z(double)[mm] ParticleName(string) 0x00(char)
//
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    SessionManager& SM = SessionManager::getInstance();

    // --- user inits ---

    SM.bGuiMode             = false;

    SM.bYAP                 = true;
    SM.bTwoScintPerAperture = false;

    //SM.DetectorType       = SessionManager::IdealDetectors;
    SM.DetectorType         = SessionManager::Scintillators;

    long Seed               = 111111;

    SM.TimeLimit            = 3.13e6; // ignore all particles appearing 0.00313+ ms after the start of irradiation

    std::string WorkingDir  = "/home/andr/WORK/ants-proto/G6";

    SM.bBinaryInput         = true;
    std::string InputFile   = "g6-0-60000.bin";

    SM.bBinaryOutput        = true;

    SM.OutputPrecision      = 8; // only affects ascii output

    // --- end of user inits ---



    SM.FileName_Input  = WorkingDir + "/" + InputFile;

    //std::string tmp = InputFile;
    //tmp.resize(InputFile.size()-4);
    SM.FileName_Output = WorkingDir + "/" + (SM.DetectorType == SessionManager::Scintillators ? "DepoScint__" : "Ideal__") + InputFile+ "__";
    SM.FileName_Output += (SM.bBinaryOutput ? ".bin" : ".txt");

    std::cout << "Output file name: " << SM.FileName_Output << std::endl;

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

    std::cout << "Output file name:" << std::endl << SM.FileName_Output << std::endl;
}
