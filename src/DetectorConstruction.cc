#include "DetectorConstruction.hh"
#include "SessionManager.hh"
#include "SensitiveDetectorIdeal.hh"
#include "SensitiveDetectorScint.hh"

#include "G4SystemOfUnits.hh"
#include "G4Element.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
#include "G4Transform3D.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SDManager.hh"

#include <QDebug>

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    G4NistManager * man = G4NistManager::Instance();
    SessionManager & SM = SessionManager::getInstance();

    // -- Materials --

    G4Material * matVacuum = man->FindOrBuildMaterial("G4_Galactic");

    G4Material * matDummy = new G4Material("Dummy", 1.0, 1.008*g/mole, 1.0e-25*g/cm3);

    std::vector<G4int> natoms;
    std::vector<G4String> elements;
    elements.push_back("Gd");     natoms.push_back(2);
    elements.push_back("Si");     natoms.push_back(1);
    elements.push_back("O");      natoms.push_back(5);
    G4double density = 6.7*g/cm3;
    G4Material * matScint = man->ConstructNewMaterial("GSO", elements, natoms, density);

    G4Material * matTeflon = man->FindOrBuildMaterial("G4_TEFLON");
    G4Material * matW = man->FindOrBuildMaterial("G4_W");

    // -- SD --
    SensitiveDetectorIdeal * pSD_Ideal = new SensitiveDetectorIdeal("Ideal");
    G4SDManager::GetSDMpointer()->AddNewDetector(pSD_Ideal);
    SensitiveDetectorScint * pSD_Scint = new SensitiveDetectorScint("Scint");
    G4SDManager::GetSDMpointer()->AddNewDetector(pSD_Scint);

    // -- Geometry --

    G4double BeamToCollSurface = 250.0*mm;

    G4double colSizeX  = 200.0*mm; // along the beam
    G4double colSizeY  = 500.0*mm; // width
    G4double colSizeZ  = 200.0*mm; // height

    G4double septa     = 2.4*mm;
    G4double aperture  = 5.1*mm;
    G4double pitch     = septa + aperture;

    G4double sciSizeX  = 2.0*mm; // should be < 0.5*aperture !
    G4double sciSizeZ  = 30.0*mm;
    G4double tapeX     = (aperture - 2.0 * sciSizeX) * 0.25;

    G4int    numEl     = colSizeX / pitch;
    //numEl = 1;
    qDebug() << "Number of scintillators:" << numEl*2;
    G4int HalfNumWalls = ceil(0.5*colSizeY / sciSizeZ);
    G4double slabSize = 0.5*colSizeY / HalfNumWalls;
    qDebug() << "Number or scintillator slabs per plane: " << HalfNumWalls*2 << "      Slab size: " << slabSize;

    G4double IdealDetectorHeight = 0.1*mm;

    G4Box             * solidWorld = new G4Box("World", 450.0*mm, 300.0*mm, colSizeZ + 2.0*BeamToCollSurface + 10.0*mm);
    G4LogicalVolume   * logicWorld = new G4LogicalVolume(solidWorld, matVacuum, "World");
    G4VPhysicalVolume * physWorld = new G4PVPlacement(nullptr, G4ThreeVector(0.0, 0.0, 0.0), logicWorld, "World", 0, false, 0);
    logicWorld->SetVisAttributes(G4VisAttributes(G4Colour(0.0, 1.0, 0.0)));
    logicWorld->SetVisAttributes(G4VisAttributes::Invisible);

    G4VSolid * solidColBlade = new G4Box("ColBlade", 0.5*septa, 0.5*colSizeY, 0.5*colSizeZ);
    G4VSolid * solidTeflon   = new G4Box("Teflon", 0.5*aperture, 0.5*colSizeY, 0.5*sciSizeZ);
    G4VSolid * solidScint    = new G4Box("Scint", 0.5*sciSizeX, 0.5*colSizeY, 0.5*sciSizeZ);
    G4VSolid * solidWall     = new G4Box("Wall", 0.5*sciSizeX, 0.1*mm, 0.5*sciSizeZ);
    G4VSolid * solidIdeal    = new G4Box("Ideal", 0.5*sciSizeX, 0.5*colSizeY, 0.5*IdealDetectorHeight);

    G4LogicalVolume * logicWall = new G4LogicalVolume(solidWall, matTeflon, "Wall");
    logicWall->SetVisAttributes(G4VisAttributes(G4Colour(1.0, 0, 0)));

    G4LogicalVolume * logicIdeal = new G4LogicalVolume(solidIdeal, matDummy, "Ideal");
    logicIdeal->SetSensitiveDetector(pSD_Ideal);
    logicIdeal->SetVisAttributes(G4VisAttributes(G4Colour(1.0, 1.0, 0)));

    std::vector<double> scintPos;
    int iCounter = 0;
    for (int iCol = 0; iCol <= numEl; iCol++)
    {
        G4double offset = iCol*pitch;

        //collimator blade
        G4LogicalVolume * logicColBlade = new G4LogicalVolume(solidColBlade, matW, "ColBlade");
        logicColBlade->SetVisAttributes(G4VisAttributes(G4Colour(0, 0.0, 1.0)));
        G4double bladeZ = -BeamToCollSurface - 0.5*colSizeZ;
        new G4PVPlacement(nullptr, G4ThreeVector(offset, 0, bladeZ), logicColBlade, "Blade", logicWorld, true, iCol);

        if (iCol == numEl) break; // last blade

        if (SM.DetectorType == SessionManager::Scintillators)
        {
            //teflon wrapper box
            G4LogicalVolume * logicTeflon = new G4LogicalVolume(solidTeflon, matTeflon, "Teflon");
            logicTeflon->SetVisAttributes(G4VisAttributes(G4Colour(1.0, 0.0, 0.0)));
            G4double teflonBoxZ = -BeamToCollSurface - colSizeZ + 0.5*sciSizeZ;
            new G4PVPlacement(nullptr, G4ThreeVector(offset + 0.5*pitch, 0, teflonBoxZ), logicTeflon, "Tf", logicWorld, true, iCol);

            //scintillator: Left
            G4LogicalVolume * logicScintL = new G4LogicalVolume(solidScint, matScint, "ScintL");
            logicScintL->SetSensitiveDetector(pSD_Scint);
            logicScintL->SetVisAttributes(G4VisAttributes(G4Colour(1.0, 1.0, 1.0)));
            G4double posLeft  = -tapeX - 0.5*sciSizeX;
            new G4PVPlacement(nullptr, G4ThreeVector(posLeft , 0, 0), logicScintL, "ScL", logicTeflon, true, iCol*2);
            scintPos.push_back(offset + 0.5*pitch + posLeft);
            //scintillator: Right
            G4LogicalVolume * logicScintR = new G4LogicalVolume(solidScint, matScint, "ScintR");
            logicScintR->SetSensitiveDetector(pSD_Scint);
            logicScintR->SetVisAttributes(G4VisAttributes(G4Colour(1.0, 1.0, 1.0)));
            G4double posRight =  tapeX + 0.5*sciSizeX;
            new G4PVPlacement(nullptr, G4ThreeVector(posRight, 0, 0), logicScintR, "ScR", logicTeflon, true, iCol*2+1);
            scintPos.push_back(offset + 0.5*pitch + posRight);

            //teflon wall splitters
            for (int iW = 0; iW< HalfNumWalls; iW++)
            {
                new G4PVPlacement(nullptr, G4ThreeVector(0, slabSize*iW, 0), logicWall, "Wall", logicScintL, true, iCounter++);
                if (iW != 0)
                    new G4PVPlacement(nullptr, G4ThreeVector(0, -slabSize*iW, 0), logicWall, "Wall", logicScintL, true, iCounter++);

                new G4PVPlacement(nullptr, G4ThreeVector(0, slabSize*iW, 0), logicWall, "Wall", logicScintR, true, iCounter++);
                if (iW != 0)
                    new G4PVPlacement(nullptr, G4ThreeVector(0, -slabSize*iW, 0), logicWall, "Wall", logicScintR, true, iCounter++);
            }
        }
        else
        {
            G4double Z = -BeamToCollSurface - colSizeZ;
            G4double posLeft   = offset + 0.5*pitch  - tapeX - 0.5*sciSizeX;
            new G4PVPlacement(nullptr, G4ThreeVector(posLeft, 0, Z), logicIdeal, "IdealL", logicWorld, true, iCounter++);
            G4double posRight  = offset + 0.5*pitch  + tapeX + 0.5*sciSizeX;
            new G4PVPlacement(nullptr, G4ThreeVector(posRight, 0, Z), logicIdeal, "IdealL", logicWorld, true, iCounter++);
        }

    }

    return physWorld;
}

