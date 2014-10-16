// Copyright (C) 2014 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef ShockFitting_ReferenceInfo_hh
#define ShockFitting_ReferenceInfo_hh

//--------------------------------------------------------------------------//

#include <vector>
#include "SConfig/ConfigObject.hh"
#include "Framework/FileLogManip.hh"
#include "Framework/Log.hh"

//--------------------------------------------------------------------------//

namespace ShockFitting {

//--------------------------------------------------------------------------//

/// This class defines ReferenceInfo, whose task is to read freestream
/// conditions, computes:
/// .) @param gref   : heat specific ratio
/// .) @param Tref   : reference temperature
/// .) @param pref   : reference pressure
/// .) @param rhoref : reference density
/// .) @param uref   : reference velocity
/// and uses them for adimensionalization of MODEL = TCneq

class ReferenceInfo :   public SConfig::Counter,
                        public SConfig::ConfigObject {
public:

  /// Constructor
  /// @param objectName the concrete class name
  ReferenceInfo(const std::string& objectName);

  /// Destructor
  virtual ~ReferenceInfo();

  /// Set up object before its first use
  virtual void setup();

  ///Unset up object before its last use
  virtual void unsetup();

  /// Run reference values reading
  virtual void read();

  /// get the name of the parent
  virtual std::string getParentName() const {return getName();}

protected:

  /// Configures the options for this object.
  /// To be extended by derived classes.
  /// @param args is the ConfigArgs with the arguments to be parsed.
  virtual void configure(SConfig::OptionMap& cmap, const std::string& prefix);

private: // helper functions

  /// get class name
  std::string getClassName() const {return "ReferenceInfo";}

  /// set reference parameters
  void setReferenceParam();

  /// adimensionalization of TCneq
  void TCneqAdim();

  /// set @param: rhoref
  void setRhoRef();

  /// set @params: R, alpha and Cv
  void setAlpha_Rgas_Cv();

  /// set @param: gref
  void setGamRef();

  /// set @param: gm1ref
  void setGm1Ref();

  /// assign values read by ReferenceInfo to PhysicsData pattern
  void setPhysicsData();

private: // data

  ///
  std::string m_var;

  ///
  std::string m_adim;

  /// heat specific ratio
  double m_gam;

  /// gas constant
  double m_R;

  /// freestream temperature [K]
  double m_Tref;

  /// freestream pressure
  double m_pref;

  /// freestream velocity
  double m_uref;

  /// species densities
  std::vector <double> m_rhor;

  ///
  double m_Lref;

  /// freestream pressure (assignable to PhysicsData
  double* pref;

  /// freestream temperature (assignable to PhysicsData)
  double* Tref;

  /// freestream velocity (assignable to PhysicsData)
  double* uref;

  /// reference pressure (assignable to PhysicsData)
  double* rhoref;

  /// heat specific ratio (assignable to PhysicsData)
  double* gref;

  /// @param gm1ref = gref-1 (assignable to PhysicsData)
  double* gm1ref;

  /// number of species
  unsigned* nsp;

  /// gas model
  std::vector <std::string>* model;

  /// species molecular weights
  std::vector <double>* mm;

  /// species heat specific ratios
  std::vector <double>* gams;

  /// names of species
  std::vector <std::string>* namesp;

  /// constant gas for each species
  std::vector <double>* Rs;

  /// formation enthalpies for each species
  std::vector <double>* hf;

  /// gas constant
  double R;

  /// specific heat
  double Cv;

  /// chemical species concentrations
  std::vector <double> alpha; 

  /// store file log infos
  FileLogManip logfile;
};

//--------------------------------------------------------------------------//

} // namespace ShockFitting

//--------------------------------------------------------------------------//

#endif // ShockFitting_ReferenceInfo_hh