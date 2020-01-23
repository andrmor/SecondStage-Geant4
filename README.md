Second stage: 
a) Ideal detectors
b) GSO scintillators

Particles are loaded from the input file, and 
(a) particles crossing the ideal detectors are saved
(b) energy deposition in the scintillators is saved

File formats
   Input
     for ascii files
       new event line: #EventNumber
       new record:     ParticleName Energy[keV] Time[ns] X[mm] Y[mm] Z[mm] DirX DirY DirZ
     for binary files
       new event line: 0xEE(char) EventNumber(int)
       new record:     0xFF(char) Energy(double)[keV] Time(double)[ns] X(double)[mm] Y(double)[mm] Z(double)[mm] DirX(double)[mm] DirY(double)[mm] DirZ(double)[mm] ParticleName(string) 0x00(char)

   Output for ascii mode
     in config with ideal detectors:
       new event line: #EventNumber
       new record:     IdelDetIndex ParticleName Energy[keV] Time[ns]
     in config with scintillators:
       new event line: #EventNumber
       new record:     ScintIndex ParticleName EnergyDeposition[keV] Time[ns] X[mm] Y[mm] Z[mm]
   Output for binary mode
     in config with ideal detectors:
          new event line: 0xEE(char) EventNumber(int)
          new record:     0xFF(char) ScintNumber(int) ParticleEnergy(double)[keV] Time(double)[ns] ParticleName(string) 0x00(char)
     in config with scintillators:
          new event line: 0xEE(char) EventNumber(int)
          new record:     0xFF(char) ScintNumber(int) ParticleEnergy(double)[keV] Time(double)[ns] X(double)[mm] Y(double)[mm] Z(double)[mm] ParticleName(string) 0x00(char)
