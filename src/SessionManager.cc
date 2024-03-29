#include "SessionManager.hh"

#include <iostream>
#include <sstream>
#include <fstream>

#include "G4ParticleDefinition.hh"
#include "G4UImanager.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4SystemOfUnits.hh"

SessionManager &SessionManager::getInstance()
{
    static SessionManager instance; // Guaranteed to be destroyed, instantiated on first use.
    return instance;
}

SessionManager::SessionManager()
{
    std::vector<std::string> allElements = {"H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S","Cl","Ar","K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr","Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I","Xe","Cs","Ba","La","Ce","Pr","Nd","Pm","Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf","Ta","W","Re","Os","Ir","Pt","Au","Hg","Tl","Pb","Bi","Po","At","Rn","Fr","Ra","Ac","Th","Pa","U","Np","Pu","Am","Cm","Bk","Cf","Es","Fm","Md","No","Lr","Rf","Db","Sg","Bh","Hs"};

    for (size_t i = 0; i < allElements.size(); i++)
        ElementToZ.emplace( std::make_pair(allElements[i], i+1) );
}

SessionManager::~SessionManager()
{
    endSession();
}

void SessionManager::startSession()
{
     /*
    // TEST : ion generation from name
    G4ParticleDefinition * pd = G4IonTable::GetIonTable()->GetIon(74, 150, 12.52*keV);  // 0*keV
    std::string name = pd->GetParticleName(); //or set directly = "Be9[123.4]";
    std::cout << "Original particle name: " << name << std::endl;
    int Z, A;
    double E;
    bool ok = extractIonInfo(name, Z, A, E);
    std::cout << "Extracted data:  Success=" << ok << "  Z=" << Z << "  A=" << A << "  E=" << E << std::endl;
    G4ParticleDefinition * newpd = G4IonTable::GetIonTable()->GetIon(Z, A, E*keV);
    std::cout << "Generated particle name: " << newpd->GetParticleName() << std::endl;
    return;
     */

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

bool SessionManager::extractIonInfo(const std::string & text, int & Z, int & A, double & E)
{
    size_t size = text.length();
    if (size < 2) return false;

    // -- extracting Z --
    const char & c0 = text[0];
    if (c0 < 'A' || c0 > 'Z') return false;
    std::string symbol;
    symbol += c0;

    size_t index = 1;
    const char & c1 = text[1];
    if (c1 >= 'a' && c1 <= 'z')
    {
        symbol += c1;
        index++;
    }
    try
    {
        Z = ElementToZ.at(symbol);
    }
    catch (...)
    {
        return false;
    }

    // -- extracting A --
    A = 0; E = 0;
    char ci;
    while (index < size)
    {
        ci = text[index];
        if (ci < '0' || ci > '9') break;
        A = A*10 + (int)ci - (int)'0';
        index++;
    }
    if (A == 0) return false;

    if (index == size) return true;

    // -- extracting excitation energy --
    if (ci != '[') return false;
    index++;
    std::stringstream energy;
    while (index < size)
    {
        ci = text[index];
        if (ci == ']')
        {
            energy >> E;
            return !energy.fail();
        }
        energy << ci;
        index++;
    }
    return false;
}

void SessionManager::runSimulation()
{
    G4UImanager* UImanager = G4UImanager::GetUIpointer();
    while (!isEndOfInputFileReached())
    {
        if (NextEventId % 1000 == 0)
            std::cout << NextEventId << std::endl;

        saveEventNumber();
        UImanager->ApplyCommand("/run/beamOn");
    }
}

bool SessionManager::isEndOfInputFileReached() const
{
    if (!inStream) return true;
    return inStream->eof();
}

void SessionManager::saveRecord_Ideal(const std::string & particleName, int scintNumber, double energy, double time)
{
    if (bBinaryOutput)
    {
        *outStream << char(0xff);
        outStream->write((char*)&scintNumber, sizeof(int));
        outStream->write((char*)&energy, sizeof(double));
        outStream->write((char*)&time, sizeof(double));
        *outStream << particleName << char(0x00);
    }
    else
    {
        std::stringstream text;
        text.precision(OutputPrecision);

        text << scintNumber << ' '
             << particleName << ' '
             << energy << ' '
             << time;

        *outStream << text.rdbuf() << '\n';
    }
}

void SessionManager::saveRecord_Scint(const std::string & particleName, int scintNumber, double depoEnergy, double time, double * pos)
{
    if (bBinaryOutput)
    {
        *outStream << char(0xff);

        outStream->write((char*)&scintNumber, sizeof(int));
        outStream->write((char*)&depoEnergy,  sizeof(double));
        outStream->write((char*)&time,        sizeof(double));
        outStream->write((char*)pos,        3*sizeof(double));

        *outStream << particleName << char(0x00);
    }
    else
    {
        std::stringstream text;
        text.precision(OutputPrecision);

        text << scintNumber << ' '
             << particleName << ' '
             << depoEnergy << ' '
             << time << ' '
             << pos[0] << ' ' << pos[1] << ' ' << pos[2];

        *outStream << text.rdbuf() << '\n';
    }
}

void SessionManager::saveEventNumber() const
{
    if (bBinaryOutput)
    {
        *outStream << char(0xee);
        outStream->write((char*)&NextEventId, sizeof(int));
    }
    else
    {
        *outStream << '#' << NextEventId << '\n';
    }
}

void SessionManager::prepareInputStream()
{
    if (bBinaryInput) inStream = new std::ifstream(FileName_Input, std::ios::in | std::ios::binary);
    else              inStream = new std::ifstream(FileName_Input);

    if (!inStream->is_open())
        terminateSession("Cannot open input file: " + FileName_Input);

    //first line should start with the event tag and contain the event number
    if (bBinaryInput)
    {
        char header = 0;
        *inStream >> header;
        if (header != char(0xee))
            terminateSession("Unexpected format of the input file (Event tag not found)");
        inStream->read((char*)&NextEventId, sizeof(int));
        if (inStream->fail())
            terminateSession("Unexpected format of the input file (Event tag not found)");
    }
    else
    {
        std::getline( *inStream, EventIdString );
        if (EventIdString.size() < 2 || EventIdString[0] != '#')
            terminateSession("Unexpected format of the input file (Event tag not found)");
        try
        {
            NextEventId = std::stoi(EventIdString.substr(1, EventIdString.size()-1));
        }
        catch (...)
        {
            terminateSession("Unexpected format of the input file (Event tag not found)");
        }
    }
    std::cout << '#' << NextEventId << std::endl;
}

void SessionManager::prepareOutputStream()
{
    outStream = new std::ofstream();

    if (bBinaryOutput) outStream->open(FileName_Output, std::ios::out | std::ios::binary);
    else               outStream->open(FileName_Output);

    if (!outStream->is_open())
        terminateSession("Cannot open output file: " + FileName_Output);
}

std::vector<ParticleRecord> & SessionManager::getNextEventPrimaries()
{
    GeneratedPrimaries.clear();

    if (bBinaryInput)  readEventFromBinaryInput();
    else               readEventFromTextInput();

    return GeneratedPrimaries;
}

void SessionManager::readEventFromTextInput()
{
    for( std::string line; std::getline( *inStream, line ); )
    {
        //std::cout << line << std::endl;
        if (line.size() < 1) continue; //allow empty lines

        if (line[0] == '#')
        {
            try
            {
                NextEventId = std::stoi( line.substr(1, line.size()-1) );
            }
            catch (...)
            {
                terminateSession("Unexpected format of the input file: event number format error");
            }
            break; //event finished
        }

        ParticleRecord r;
        std::string particle;

        std::stringstream ss(line);  // units in file are mm keV and ns
        ss >> particle
           >> r.Energy
           >> r.Position[0]  >> r.Position[1] >>  r.Position[2]
           >> r.Direction[0] >> r.Direction[1] >> r.Direction[2]
           >> r.Time;
        if (ss.fail())
            terminateSession("Unexpected format of a line in the file with the input particles");

        r.Particle = makeGeant4Particle(particle);
        //std::cout << str << ' ' << r.Energy << ' ' << r.Time << ' ' << r.Position[0] << ' ' << r.Position[1] << ' ' << r.Position[2] << ' ' << r.Direction[0] << ' ' << r.Direction[1] << ' ' << r.Direction[2] << std::endl;
        GeneratedPrimaries.push_back(r);
    }
}

void SessionManager::readEventFromBinaryInput()
{
    char header = 0;

    while (*inStream >> header)
    {
        if (header == char(0xee))
        {
            inStream->read((char*)&NextEventId, sizeof(int));
            //std::cout << '#' << NextEventId << std::endl;
            break; //event finished
        }
        else if (header == char(0xff))
        {
            ParticleRecord r;

            char ch;
            std::string str;
            while (*inStream >> ch)
            {
                if (ch == 0x00) break;
                str += ch;
            }
            inStream->read((char*)&r.Energy,       sizeof(double));
            inStream->read((char*)&r.Position[0],  sizeof(double));
            inStream->read((char*)&r.Position[1],  sizeof(double));
            inStream->read((char*)&r.Position[2],  sizeof(double));
            inStream->read((char*)&r.Direction[0], sizeof(double));
            inStream->read((char*)&r.Direction[1], sizeof(double));
            inStream->read((char*)&r.Direction[2], sizeof(double));
            inStream->read((char*)&r.Time,         sizeof(double));

            if (inStream->fail())
                terminateSession("Unexpected format of a line in the binary file with the input particles");

            r.Particle = makeGeant4Particle(str);
            //std::cout << str << ' ' << r.Energy << ' ' << r.Time << ' ' << r.Position[0] << ' ' << r.Position[1] << ' ' << r.Position[2] << ' ' << r.Direction[0] << ' ' << r.Direction[1] << ' ' << r.Direction[2] << std::endl;
            GeneratedPrimaries.push_back(r);
        }
        else
        {
            terminateSession("Unexpected format of binary input");
            break;
        }
    }
}

G4ParticleDefinition * SessionManager::makeGeant4Particle(const std::string & particleName)
{
    G4ParticleDefinition * Particle = G4ParticleTable::GetParticleTable()->FindParticle(particleName);

    if (!Particle)
    {
        // is it an ion?
        int Z, A;
        double E;
        bool ok = extractIonInfo(particleName, Z, A, E);
        if (!ok)
            terminateSession("Found an unknown particle: " + particleName);

        Particle = G4ParticleTable::GetParticleTable()->GetIonTable()->GetIon(Z, A, E*keV);

        if (!Particle)
            terminateSession("Failed to generate ion: " + particleName);

        //std::cout << particleName << "   ->   " << Particle->GetParticleName() << std::endl;
    }

    return Particle;
}

void SessionManager::terminateSession(const std::string & ErrorMessage)
{
    std::cerr << ErrorMessage << std::endl;
    endSession();
    exit(1);
}
