// Copyright (C) 2014 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include "StateUpdaterSF/ComputeStateDpsAr.hh"
#include "Framework/Log.hh"
#include "SConfig/ObjectProvider.hh"
#include "StateUpdaterSF/CoDc.hh"
#include "StateUpdaterSF/CoShock.hh"

//----------------------------------------------------------------------------//

using namespace std;
using namespace SConfig;

//----------------------------------------------------------------------------//

namespace ShockFitting {

//----------------------------------------------------------------------------//

// this variable instantiation activates the self-registration mechanism
ObjectProvider<ComputeStateDpsAr, ComputeStateDps>
 computeStateDpsArProv("ComputeStateDpsAr");

//----------------------------------------------------------------------------//

ComputeStateDpsAr::ComputeStateDpsAr(const std::string& objectName) :
  ComputeStateDps(objectName)
{
}

//----------------------------------------------------------------------------//

ComputeStateDpsAr::~ComputeStateDpsAr()
{

}

//----------------------------------------------------------------------------//

void ComputeStateDpsAr::setup()
{
  LogToScreen(VERBOSE, "ComputeStateDpsAr::setup => start\n");

  LogToScreen(VERBOSE, "ComputeStateDpsAr::setup => end\n");
}

//----------------------------------------------------------------------------//

void ComputeStateDpsAr::unsetup()
{
  LogToScreen(VERBOSE, "ComputeStateDpsAr::unsetup\n");
}

//----------------------------------------------------------------------------//

void ComputeStateDpsAr::update()
{
  LogToScreen(INFO, "ComputeStateDpsAr::update()\n");

  logfile.Open(getClassName());

  setMeshData();
  setPhysicsData();

  setAddress();

  setDiscSpeedSize();

  // create object of CoShock class
  CoShock computenewStateForShock;

  // create object of CoDc class
  CoDc computenewStateForDc;

  alphad.resize( (*nsp) );
  alphau.resize( (*nsp) );
  xd.resize(4);
  xu.resize(4);

  unsigned I;

  for(unsigned ISH=0; ISH<(*nShocks); ISH++) {

   unsigned ivalue = ISH+1;
   logfile("Shock/Disc. n. ",ivalue, "\n");

   for(unsigned IV=0; IV<nShockPoints->at(ISH); IV++) {
    ++TotnbShockPoints;

    R2.resize(nShockPoints->at(ISH),(*nShocks));

    I=IV;
    dx = (*vShNor)(0,I,ISH);
    dy = (*vShNor)(1,I,ISH);

    // upload downstream status
    recoverDownState(IV,ISH);

    // upload upstream status
    recoverUpState(IV,ISH);

    // initialize discontinuity speed
    WS = 0.0;

    if(typeSh->at(ISH)=="S") {
     computenewStateForShock.callCoShock(xd,xu,R2(IV,ISH));
     xd = computenewStateForShock.getnewDownValues();
     WS = computenewStateForShock.getnewDiscSpeed();
    }

    if(typeSh->at(ISH)=="D") {
     computenewStateForDc.callCoDc(xd,xu);
     xd = computenewStateForDc.getnewDownValues();
     xu = computenewStateForDc.getnewUpValues();
     WS = computenewStateForDc.getnewDiscSpeed();
    }

    // enforce tangential component equality for the shock case
    if(typeSh->at(ISH)=="S") { xd.at(3) = xu.at(3); }

    // save old downstream status
    saveDownState(IV, ISH);

    // compute downstream variables and assing the new values
    // to Zroe array (for the shock case)
    computeDownState(IV, ISH);

    // compute upstream variables and assing the new values 
    // to Zroe array (for the shock case)
    computeUpState(IV, ISH);

    // set the new discontinuity speed
    (*WSh)(0,IV,ISH) = WS * dx;
    (*WSh)(1,IV,ISH) = WS * dy;

    ivalue = IV+1;
    logfile("S/D point nr. ", ivalue, " Speed: ");
    logfile((*WSh)(0,IV,ISH), ", ", (*WSh)(1,IV,ISH), "\n");
   }
  }
  logfile.Close();
}

//----------------------------------------------------------------------------//

void ComputeStateDpsAr::recoverDownState(unsigned IV, unsigned ISH)
{
  double ZRHOSH = 0;
  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   ZRHOSH = ZRHOSH + (*ZroeShd)(ISP,IV,ISH);
  }
  // density
  xd.at(0) = ZRHOSH*ZRHOSH;

  double RHOHF = 0;
  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   RHOHF = RHOHF + (*ZroeShd)(ISP,IV,ISH)* hf->at(ISP);
  }
  // formation enthalpy
  RHOHF = RHOHF*ZRHOSH;  // (rho*Hf)

  xd.at(3) = ( -(*ZroeShd)((*ix),IV,ISH) * dy + (*ZroeShd)((*iy),IV,ISH) * dx );
  xd.at(3) = xd.at(3)/ZRHOSH; // tangential
  xd.at(2) = ( (*ZroeShd)((*ix),IV,ISH) * dx + (*ZroeShd)((*iy),IV,ISH) * dy );
  xd.at(2) = xd.at(2)/ZRHOSH; // normal

  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   // chemical species concentrations
   alphad.at(ISP) = (*ZroeShd)(ISP,IV,ISH)/ZRHOSH;
  }

  help = pow( (*ZroeShd)((*ix),IV,ISH) , 2) + pow( (*ZroeShd)((*iy),IV,ISH) , 2);
  // pressure
  xd.at(1) = ((*gref)-1)/(*gref) * (ZRHOSH *  (*ZroeShd)((*ie),IV,ISH) -
              0.5 * help - RHOHF);
  R2(IV,ISH) = sqrt((*gref)*xd.at(1)/xd.at(0)) + 0.5 * ((*gref)-1) * xd.at(2);

  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   logfile("Zd(",ISP,") ",(*ZroeShd)(ISP,IV,ISH),"\n");
  }
  logfile("Zd(IE) ", (*ZroeShd)((*ie),IV,ISH),"\n");
  logfile("Zd(IX) ", (*ZroeShd)((*ix),IV,ISH),"\n");
  logfile("Zd(IY) ", (*ZroeShd)((*ix),IV,ISH),"\n");
  for(unsigned i=0; i<4; i++) { logfile(xd.at(i), " "); }
  logfile("\n");
}

//----------------------------------------------------------------------------//

void ComputeStateDpsAr::recoverUpState(unsigned IV, unsigned ISH)
{
  double ZRHOSH = 0;
  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   ZRHOSH = ZRHOSH + (*ZroeShu)(ISP,IV,ISH);
  }
  // density 
  xu.at(0) = ZRHOSH*ZRHOSH;
  
  double RHOHF = 0;
  for(unsigned ISP=0; ISP<(*nsp); ISP++) { 
   RHOHF = RHOHF + (*ZroeShu)(ISP,IV,ISH) * hf->at(ISP);
  }
  // formation enthalpy  
  RHOHF = RHOHF*ZRHOSH;  // (rho*Hf)

  xu.at(3) = ( -(*ZroeShu)((*ix),IV,ISH) * dy + (*ZroeShu)((*iy),IV,ISH) * dx );
  xu.at(3) = xu.at(3)/ZRHOSH; // tangential
  xu.at(2) = ( (*ZroeShu)((*ix),IV,ISH) * dx + (*ZroeShu)((*iy),IV,ISH) * dy );
  xu.at(2) = xu.at(2)/ZRHOSH; // normal

  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   // chemical species concentrations
   alphau.at(ISP) = (*ZroeShu)(ISP,IV,ISH)/ZRHOSH;
  }

  help = pow( (*ZroeShu)((*ix),IV,ISH) , 2) + pow( (*ZroeShu)((*iy),IV,ISH) , 2);
  // pressure
  xu.at(1) = ((*gref)-1)/(*gref) * (ZRHOSH *  (*ZroeShu)((*ie),IV,ISH) -
                    0.5 * help - RHOHF);

  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   logfile("Zu(",ISP,") ",(*ZroeShu)(ISP,IV,ISH),"\n");
  }
  logfile("Zu(IE) ", (*ZroeShu)((*ie),IV,ISH),"\n");
  logfile("Zu(IX) ", (*ZroeShu)((*ix),IV,ISH),"\n");
  logfile("Zu(IY) ", (*ZroeShu)((*ix),IV,ISH),"\n");
  for(unsigned i=0; i<4; i++) { logfile(xu.at(i), " "); }
  logfile("\n");
} 

//----------------------------------------------------------------------------//

void ComputeStateDpsAr::computeDownState(unsigned IV, unsigned ISH)
{
  double z1, z2, z3, z4, kine, enthalpy;
  vector<double> zs(*nsp);

  z1 = sqrt(xd.at(0));
  kine = 0.5 * (pow(xd.at(2),2) + pow(xd.at(3),2));

  double hfd = 0;
  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   hfd = hfd +  alphad.at(ISP) * hf->at(ISP);
  }

  enthalpy = (*gref)/((*gref)-1) * xd.at(1)/xd.at(0) + kine + hfd;

  z2 = z1 * enthalpy;
  z3 = z1 * (xd.at(2) * dx - xd.at(3) * dy);
  z4 = z1 * (xd.at(2) * dy + xd.at(3) * dx);

  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   zs.at(ISP) = alphad.at(ISP) * z1;
  }

  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   (*ZroeShd)(ISP,IV,ISH) = zs.at(ISP);
  }
  (*ZroeShd)((*ie),IV,ISH) = z2;
  (*ZroeShd)((*ix),IV,ISH) = z3;
  (*ZroeShd)((*iy),IV,ISH) = z4;
}

//----------------------------------------------------------------------------//

void ComputeStateDpsAr::computeUpState(unsigned IV, unsigned ISH)
{
  double z1, z2, z3, z4, kine, enthalpy;
  vector<double> zs(*nsp);

  z1 = sqrt(xu.at(0));
  kine = 0.5 * (pow(xu.at(2),2) + pow(xu.at(3),2));

  double hfu = 0;
  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   hfu = hfu +  alphau.at(ISP) * hf->at(ISP);
  }

  enthalpy = (*gref)/((*gref)-1) * xu.at(1)/xu.at(0) + kine + hfu;

  z2 = z1 * enthalpy;
  z3 = z1 * (xu.at(2) * dx - xu.at(3) * dy);
  z4 = z1 * (xu.at(2) * dy + xu.at(3) * dx);

  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   zs.at(ISP) = alphau.at(ISP) * z1;
  }

  for(unsigned ISP=0; ISP<(*nsp); ISP++) {
   (*ZroeShu)(ISP,IV,ISH) = zs.at(ISP);
  }
  (*ZroeShu)((*ie),IV,ISH) = z2;
  (*ZroeShu)((*ix),IV,ISH) = z3;
  (*ZroeShu)((*iy),IV,ISH) = z4;
}

//----------------------------------------------------------------------------//

void ComputeStateDpsAr::saveDownState(unsigned IV, unsigned ISH)
{
  for(unsigned I=0; I<(*ndof); I++) {
   (*ZroeShdOld)(I,IV,ISH) = (*ZroeShd)(I,IV,ISH);
  }
}

//----------------------------------------------------------------------------//

} // namespace ShockFitting