#pragma once

////////////////////////////////////////////////////////////////
///
///Class:               PhotoTwoSpin0Amps
///Description: Formalism for photoproduction of gp->Xp with X->yz
///            where y and z are spin 0 and g is polarised
#include <TString.h>
#include "FitManager.h"
#include "AmpConfigure.h"
#include "ComponentsPdfParser.h"

namespace HS{
  namespace FIT{

    class PhotoTwoSpin0Amps : public AmpConfigure{

    public:
      PhotoTwoSpin0Amps()=default;
      PhotoTwoSpin0Amps(const std::string& name):_Name{name} {};
      PhotoTwoSpin0Amps(const PhotoTwoSpin0Amps&)=default;
      PhotoTwoSpin0Amps(PhotoTwoSpin0Amps&&)=default;
      ~PhotoTwoSpin0Amps() =default;
      PhotoTwoSpin0Amps& operator=(const PhotoTwoSpin0Amps& other)=default;
      PhotoTwoSpin0Amps& operator=(PhotoTwoSpin0Amps&& other) = default;

      void SetManager(FitManager* manager){
	_Setup=&(manager->SetUp());
      }
     void SetSetup(Setup* setup){
       _Setup=setup;
      }
      void SetDecayAngleCosTh(const TString& var){
	_Setup->LoadVariable(var);
	_DecayAngleCosTh=var(0,var.First('['));
      }
      void SetDecayAnglePhi(const TString& var){
	_Setup->LoadVariable(var);
	_DecayAnglePhi=var(0,var.First('['));
      }
      void SetPolPhi(const TString& var){
	_Setup->LoadVariable(var);
	_PolPhi=var(0,var.First('['));
      }
      void SetPolarisation(const TString& var){
	_Setup->LoadVariable(var);
	_Polarisation=var(0,var.First('['));
      }
      void SetConstPolarisation(const TString& var){ //e.g. Pol[0.5]
	_Setup->LoadConstant(var);
	//	_Polarisation=var(0,var.First('['));
	//extract numerical value for polairsation
	_Polarisation=var(var.First('[')+1,var.First(']')-var.First('[')-1);
	_constPol=kTRUE;
      }
      
      void SetLmax(Int_t lm){
	_Lmax = lm;
	_Mmax = lm; //default = Lmax
      }
      void SetMmax(Int_t lm){
	_Mmax = lm;
      }
      void SetNrefl(Int_t r){
	_Nref = r;
      }
      void PositiveMOnly(){
	_NegativeM=kFALSE;
      }
      void SetOnlyEvenWaves(){
	_OnlyEven=kTRUE;
      }
      std::string  ConfigureMoments() override;
      
      std::string ConfigurePWAs() override;

      void LoadModelPDF(Long64_t Nevents=1) override;
      
      void PrintModel();
      
      const std::string&  GetName(){return _Name;}

  
    protected:

      ComponentsPdfParser PolarisedSphHarmonicMoments();
      
    private:

      std::string _DecayAngleCosTh;
      std::string _DecayAnglePhi;
      std::string _PolPhi;
      std::string _Polarisation;
      std::string _Sum;
      std::string _Name;
      
      Int_t _Lmax=0;
      Int_t _Mmax=0;
      Int_t _Nref=2;
      Bool_t _OnlyEven=kFALSE;
      Bool_t _NegativeM=kTRUE;
      Bool_t _constPol=kFALSE;

      
    };
    
  }
}
