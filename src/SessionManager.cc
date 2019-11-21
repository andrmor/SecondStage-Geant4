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
    int iCounter = 0;
    G4UImanager* UImanager = G4UImanager::GetUIpointer();
    while (!isEndOfInputFileReached())
    {
        if (iCounter % 1000 == 0)
            std::cout << EventId << std::endl;

        saveEventId();
        UImanager->ApplyCommand("/run/beamOn");
        iCounter++;
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

        std::stringstream ss(line);  // units in file are mm keV and ns
        ss >> particle >> r.Energy >> r.Time
           >> r.Position[0]  >> r.Position[1] >>  r.Position[2]
           >> r.Direction[0] >> r.Direction[1] >> r.Direction[2];
        //qDebug() << particle.data() << r.Energy << r.Time << r.Position[0]  << r.Position[1] <<  r.Position[2] << r.Direction[0] << r.Direction[1] << r.Direction[2];

        if (ss.fail())
            terminateSession("Unexpected format of a line in the file with the input particles");

        r.Particle = G4ParticleTable::GetParticleTable()->FindParticle(particle);
        if (!r.Particle)
        {
            // is it an ion?
            int Z, A;
            double E;
            bool ok = extractIonInfo(particle, Z, A, E);
            if (!ok)
                terminateSession("Found an unknown particle: " + particle);

            r.Particle = G4ParticleTable::GetParticleTable()->GetIonTable()->GetIon(Z, A, E*keV);

            //std::cout << particle << "   ->   " << r.Particle->GetParticleName() << std::endl;

            if (!r.Particle)
                terminateSession("Failed to generate ion: " + particle);
        }

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
