/*****************************************************************************
 * Project: RooFit                                                           *
 *                                                                           *
  * This code was autogenerated by RooClassFactory                            * 
 *****************************************************************************/
#pragma once

#include "RooHSEventsPDF.h"
#include <RooRealProxy.h>
#include <RooCategoryProxy.h>
#include <RooAbsReal.h>
#include <RooAbsCategory.h>
#include <RooDataHist.h>
#include <RooGaussian.h>
#include <RooConstVar.h>
#include <RooRealVar.h>
#include <RooAbsPdf.h>
#include <TH2.h>


namespace HS{
  namespace FIT{
    
    class RooHSEventsHistPDF : public RooHSEventsPDF {
    public:
      
      RooHSEventsHistPDF() =default ; 
       
      RooHSEventsHistPDF(const char *name, const char *title, RooAbsReal& _x,RooAbsReal& _alpha,RooAbsReal& _offset, RooAbsReal& _scale);
      
      RooHSEventsHistPDF(const RooHSEventsHistPDF& other, const char* name=nullptr) ;
      TObject* clone(const char* newname) const override { return new RooHSEventsHistPDF(*this,newname); }
      ~RooHSEventsHistPDF() override;

    protected:
      Double_t fMCx{};

      RooRealProxy x ;
      RooRealProxy offset ;
      RooRealProxy scale ;
      RooRealProxy alpha ;
  
      Double_t evaluate() const override ;
      Double_t evaluateMC(const vector<Float_t> *vars,const  vector<Int_t> *cats) const override ;
      Double_t evaluateMC(Double_t mcx) const ;
      void MakeSets();
  
      RooDataHist* fHist=nullptr;
      TH2D* fRHist=nullptr;
      Double_t fVarMax{};
  
      RooRealVar* fx_off=nullptr; //variables for hist
      RooRealVar* falpha=nullptr;

    private:
      RooGaussian *fAlphaConstr=nullptr;
      RooGaussian *fOffConstr=nullptr;
      RooGaussian *fScaleConstr=nullptr;
  
      mutable Double_t _min=0;
      mutable Double_t _max=0;
      mutable Double_t _delta=0;
      mutable const RooRealVar* _var=nullptr;
      mutable Int_t _minBin=-1;
      mutable Int_t _maxBin=-1;
      
      Int_t _Nbins=5000;

    public:

      Bool_t SetEvTree(TTree* tree,TString cut,TTree* MCGenTree=nullptr) override;
      void CreateHistPdf();
      virtual void ResetTree();
  
      Int_t getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars,const char* rangeName) const override;
      Double_t analyticalIntegral(Int_t code,const char* rangeName) const override;
  
      RooGaussian* AlphaConstraint() {return fAlphaConstr;};
      RooGaussian* OffConstraint() {return fOffConstr;};
      RooGaussian* ScaleConstraint() {return fScaleConstr;};

      TH2* GetRootHist() {return fRHist;}
      ClassDefOverride(HS::FIT::RooHSEventsHistPDF,1); // Your description goes here...
    };//Class

    
  }//namespace FIT
}//namespace HS

 
