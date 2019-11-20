#include "SessionManager.hh"

#include <iostream>
#include <sstream>
#include <fstream>

#include "G4ParticleDefinition.hh"
#include "G4UImanager.hh"
#include "G4ParticleTable.hh"

//#include <QDebug>

SessionManager &SessionManager::getInstance()
{
    static SessionManager instance; // Guaranteed to be destroyed, instantiated on first use.
    return instance;
}

SessionManager::SessionManager(){}

SessionManager::~SessionManager()
{
    endSession();
}

void SessionManager::startSession()
{
    prepareInputStream();
    prepareOutputStream();
}

void SessionManager::endSession()
{
    if (inStream)   inStream->close();
    delete inStream; inStream = nullptr;

    if (outStream) outStream->close();
    delete outStream; outStream = nullptr;
}

void SessionManager::runSimulation()
{
    int iCounter = 0;
    G4UImanager* UImanager = G4UImanager::GetUIpointer();
    while (!isEndOfInputFileReached())
    {
        if (iCounter % 100 == 0)
            std::cout << "Event # " << iCounter << std::endl;

        saveEventId();
        UImanager->ApplyCommand("/run/beamOn");
    }
}

bool SessionManager::isEndOfInputFileReached() const
{
    if (!inStream) return true;
    return inStream->eof();
}

void SessionManager::sendLineToOutput(const std::stringstream & text) const
{
    *outStream << text.rdbuf() << std::endl;
}

void SessionManager::saveEventId() const
{
     *outStream << EventId.data() << std::endl;
}

void SessionManager::prepareInputStream()
{
    inStream = new std::ifstream(FileName_Input);
    if (!inStream->is_open())
        terminateSession("Cannot open input file: " + FileName_Input);

    getline( *inStream, EventId );
    if (EventId.size() < 2 || EventId[0] != '#')
        terminateSession("Unexpected format of the input file (Event tag not found)");

    std::cout << EventId << std::endl;
}

void SessionManager::prepareOutputStream()
{
    outStream = new std::ofstream();
    outStream->open(FileName_Output);
    if (!outStream->is_open())
        terminateSession("Cannot open output file: " + FileName_Output);
}

std::vector<ParticleRecord> & SessionManager::getNextEventPrimaries()
{
    GeneratedPrimaries.clear();

    for( std::string line; getline( *inStream, line ); )
    {
        //std::cout << line << std::endl;
        if (line.size() < 1) continue; //allow empty lines

        if (line[0] == '#')
        {
            EventId = line;
            break; //event finished
        }

        ParticleRecord r;
        std::string particle;

        std::stringstream ss(line);
        ss >> particle >> r.Energy >> r.Time
           >> r.Position[0]  >> r.Position[1] >>  r.Position[2]
           >> r.Direction[0] >> r.Direction[1] >> r.Direction[2];
        //qDebug() << particle.data() << r.Energy << r.Time << r.Position[0]  << r.Position[1] <<  r.Position[2] << r.Direction[0] << r.Direction[1] << r.Direction[2];

        if (ss.fail())
            terminateSession("Unexpected format of a line in the file with the input particles");

        r.Particle = G4ParticleTable::GetParticleTable()->FindParticle(particle);
        if (!r.Particle) terminateSession("Found an unknown particle: " + particle); // *** todo: ions!!!

        GeneratedPrimaries.push_back(r);
    }

    return GeneratedPrimaries;
}

void SessionManager::terminateSession(const std::string & ErrorMessage)
{
    std::cerr << ErrorMessage << std::endl;
    endSession();
    exit(1);
}
