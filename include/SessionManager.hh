#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <string>
#include <vector>

#include "G4ThreeVector.hh"

class G4ParticleDefinition;

struct ParticleRecord
{
    G4ParticleDefinition * Particle = nullptr;
    G4double Energy = 0;
    G4ThreeVector Position  = {0, 0, 0};
    G4ThreeVector Direction = {0, 0, 0};
    G4double Time = 0;
};

class G4Material;

class SessionManager
{
    public:
        static SessionManager& getInstance();

    private:
        SessionManager();
        ~SessionManager();

    public:
        SessionManager(SessionManager const&) = delete;
        void operator=(SessionManager const&) = delete;

        void startSession();
        void endSession();

        void runSimulation();

        bool bGuiMode           = false;

        enum SimMode {IdealDetectors = 0, Scintillators = 1};
        SimMode DetectorType;

        std::string FileName_Input;
        std::string FileName_Output;

public:
        void sendLineToOutput(const std::stringstream & text) const;
        void saveEventId() const;
        void prepareInputStream();
        void prepareOutputStream();
        void terminateSession(const std::string & ErrorMessage);
        bool isEndOfInputFileReached() const;

        std::vector<ParticleRecord> & getNextEventPrimaries();

private:
        std::ifstream * inStream = nullptr;
        std::ofstream * outStream = nullptr;
        std::vector<ParticleRecord> GeneratedPrimaries;
        std::string EventId = "#0";
};

#endif // SESSIONMANAGER_H
