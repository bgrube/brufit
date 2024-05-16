////////////////////////////////////////////////////////////////
///
///Class:               RooMcmc
///Description:
///           


#pragma once


#include "Minimiser.h"
#include <RooAbsData.h>
#include <TTree.h>
#include <RooStats/ModelConfig.h>
#include <RooStats/MarkovChain.h>
#include <RooStats/ProposalFunction.h>
#include <RooFitResult.h>

namespace HS{
  namespace FIT{

    class RooMcmc  : public Minimiser {
      
    public:

      RooMcmc(Int_t Niter=100,Int_t Nburn=10, Float_t norm=0.1): fNumIters(Niter),fNumBurnInSteps(Nburn),fNorm(norm){
	SetNameTitle("HSRooMcmc","RooMcmc minimiser");
      }
      // RooMcmc(const RooMcmc&)=default;
      //RooMcmc(RooMcmc&&)=default;
      ~RooMcmc() override;
      //RooMcmc& operator=(const RooMcmc& other)=default;
      //RooMcmc& operator=(RooMcmc&& other) = default;  

      void Run(Setup &setup,RooAbsData &fitdata) override;

      void FitTo();
      
      file_uptr SaveInfo() override;
      void AddFormulaToMCMCTree();
      
      Bool_t MakeChain();
      TMatrixDSym MakeMinuitCovarianceMatrix();
      TMatrixDSym MakeMcmcCovarianceMatrix(TTree* tree,size_t burnin);
      TMatrixDSym MakeMcmcRMSMatrix(TTree* tree, size_t burnin);
      TMatrixDSym MakeMcmcNonYieldCovarianceMatrix(TTree* tree,size_t burnin);
      TMatrixDSym MakeMcmcPrincipalCovarianceMatrix(TTree* tree,size_t burnin);
      TTree* GetTree(){return fTreeMCMC;}
      Double_t SumWeights();
      Double_t SumWeights2();
      
      void NoWeightCorrection(){fCorrectForWeights=kFALSE;}
      
      void Result();
      Double_t NLL(){return fChain->NLL();}



      //MCMCCalculator

      void SetModel(RooStats::ModelConfig *model);

      /// Set the DataSet if not already there
      void SetData(RooAbsData& data) { fData = &data; }

      /// Set the Pdf if not already there
      void SetPdf(RooAbsPdf& pdf) { fPdf = &pdf; }

      /// Set the Prior Pdf if not already there
      void SetPriorPdf(RooAbsPdf& pdf) { fPriorPdf = &pdf; }

      /// specify the parameters of interest in the interval
      void SetParameters(const RooArgSet& set) { fPOI.removeAll(); fPOI.add(set); }

      /// specify the parameters to store in the Markov chain
      /// By default all the parameters are stored
      void SetChainParameters(const RooArgSet & set) { fChainParams.removeAll(); fChainParams.add(set); }

      /// specify the nuisance parameters (eg. the rest of the parameters)
      void SetNuisanceParameters(const RooArgSet& set) {fNuisParams.removeAll(); fNuisParams.add(set);}

      /// set the conditional observables which will be used when creating the NLL
      /// so the pdf's will not be normalized on the conditional observables when computing the NLL
      void SetConditionalObservables(const RooArgSet& set) {fConditionalObs.removeAll(); fConditionalObs.add(set);}

      /// set the global observables which will be used when creating the NLL
      /// so the constraint pdf's will be normalized correctly on the global observables when computing the NLL
      void SetGlobalObservables(const RooArgSet& set) {fGlobalObs.removeAll(); fGlobalObs.add(set);}

      /// set the proposal function for suggesting new points for the MCMC
      void SetProposalFunction(RooStats::ProposalFunction& proposalFunction)
      { fPropFunc = &proposalFunction; }

      /// set the number of iterations to run the metropolis algorithm
      void SetNumIters(Int_t numIters)
      { fNumIters = numIters; }

      /// set the number of steps in the chain to discard as burn-in,
      /// starting from the first
      void SetNumBurnInSteps(Int_t numBurnInSteps)
      { fNumBurnInSteps = numBurnInSteps; }

      void SetupBasicUsage();
      void SetKeepStart(Bool_t keep=kTRUE){fKeepStart=keep;}

      virtual Int_t GetNumBurnInSteps()const {return fNumBurnInSteps;}

      void SetDesiredAcceptance(Double_t min,Double_t max,Double_t target=0){
	fMinAcc=min;
	fMaxAcc=max;
	if(target)
	  fTargetAcc=target;
	else
	  fTargetAcc = (max-min)/2;
      }
      void SetUncorrelateYields(Int_t un){fUncorrelateYields=un;}
      void SetParVals(RooArgSet* toThesePars);
      
    protected :
      void AddEntryBranch();
      void CleanMakeChain();
      
      RooStats::MarkovChain* fChain =nullptr; //!
      RooDataSet* fChainData=nullptr;//!
      TTree* fTreeMCMC=nullptr;//!
      Bool_t fCorrectForWeights=kTRUE;
      RooArgSet* fParams=nullptr;//!
      std::shared_ptr<TFile> fTempFile;//!
      file_uptr fOutFile;//!
      
      Bool_t fKeepStart=kFALSE; //randomise starting values
      Bool_t fMCMCHelp=kFALSE;//automate acceptance etc.
      //MCMCCalculator
      RooStats::ModelConfig *fModelConfig=nullptr;
      
      RooArgSet   fPOI;        //! parameters of interest for interval
      RooArgSet   fNuisParams; //! nuisance parameters for interval (not really used)
      RooArgSet   fChainParams; //! parameters to store in the chain (if not specified they are all of them )
      RooArgSet   fConditionalObs; //! conditional observables
      RooArgSet   fGlobalObs;     //! global observables
      RooStats::ProposalFunction* fPropFunc{}; //! Proposal function for MCMC integration
      RooAbsPdf * fPdf=nullptr;        //! pointer to common PDF (owned by the workspace)
      RooAbsPdf * fPriorPdf=nullptr;   //! pointer to prior  PDF (owned by the workspace)
      //RooAbsData * fData=nullptr;     //! pointer to the data (owned by the workspace)
      Int_t fNumIters; // number of iterations to run metropolis algorithm
      Int_t fNumBurnInSteps; // number of iterations to discard as burn-in, starting from the first

      Int_t fNumBins{}; // set the number of bins to create for each
      Int_t fWarmup{}; //ignore these events
      Float_t fNorm=1;
      Int_t fNumStepsThres=500000; //Number of steps past which the chain will not adjust norm (only adjust position)

      Int_t fNumBurnInStepsCov; //Number of steps to remove from chain to make covariance matrix for proposal function

      Bool_t fDontDeleteChain = kFALSE; //Default to start chain again

      vector<Double_t> _formVals;//(formulas.getSize(),0);
      vector<TBranch*> _formBranches;//(formulas.getSize(),nullptr);

      Double_t fChainAcceptance=0;//!
      Double_t fMinAcc=0.15;
      Double_t fMaxAcc=0.3;
      Double_t fTargetAcc=0.234;
      Int_t  fUncorrelateYields=0;
      
      ClassDefOverride(HS::FIT::RooMcmc,1);
      
     };

    class RooMcmcSeq  : public RooMcmc {
      
    public:

    RooMcmcSeq(Int_t Niter=100,Int_t Nburn=10, Float_t norm=0.1):RooMcmc(Niter,Nburn,norm){
	SetNameTitle("HSRooMcmcSeq","RooMcmcSeq minimiser");
      }
      //  RooMcmcSeq(const RooMcmcSeq&)=default;
      // RooMcmcSeq(RooMcmcSeq&&)=default;
      ~RooMcmcSeq() override =default;
      // RooMcmcSeq& operator=(const RooMcmcSeq& other)=default;
      //RooMcmcSeq& operator=(RooMcmcSeq&& other) = default;  

      void Run(Setup &setup,RooAbsData &fitdata) override;

      ClassDefOverride(HS::FIT::RooMcmcSeq,1);
   };

    class RooMcmcSeqHelper  : public RooMcmc {
      
    public:
   RooMcmcSeqHelper(Int_t Niter=100,Int_t Nburn=10, Float_t norm=0.1):RooMcmc(Niter,Nburn,norm){
	SetNameTitle("HSRooMcmcSeqHelper","RooMcmcSeqHelper minimiser");
      }
      //RooMcmcSeqHelper(const RooMcmcSeqHelper&)=default;
      //RooMcmcSeqHelper(RooMcmcSeqHelper&&)=default;
      ~RooMcmcSeqHelper() override =default;
      //RooMcmcSeqHelper& operator=(const RooMcmcSeqHelper& other)=default;
      //RooMcmcSeqHelper& operator=(RooMcmcSeqHelper&& other) = default;  

      void Run(Setup &setup,RooAbsData &fitdata) override;

      ClassDefOverride(HS::FIT::RooMcmcSeqHelper,1);
   };

 class RooMcmcSeqCov  : public RooMcmc {
      
    public:

   RooMcmcSeqCov(Int_t Nburn=10,Int_t Niter=100,Int_t NburnCov=10, Float_t norm=0.1):RooMcmc(Niter,NburnCov,norm),fNumBurnInStepsForCov(Nburn){
	SetNameTitle("HSRooMcmcSeqCov","RooMcmcSeqCov minimiser");
      }
   // RooMcmcSeqCov(const RooMcmcSeqCov&)=default;
   // RooMcmcSeqCov(RooMcmcSeqCov&&)=default;
      ~RooMcmcSeqCov() override =default;
   // RooMcmcSeqCov& operator=(const RooMcmcSeqCov& other)=default;
   // RooMcmcSeqCov& operator=(RooMcmcSeqCov&& other) = default;  

      void Run(Setup &setup,RooAbsData &fitdata) override;

   Int_t fNumBurnInStepsForCov=50; //Number of burn in steps to discard from the chain to make covariance matrix

   ClassDefOverride(HS::FIT::RooMcmcSeqCov,1);
   };

 class RooMcmcSeqThenCov  : public RooMcmc {
      
 public:
   
   RooMcmcSeqThenCov(Int_t Niter=100,Int_t Nburn=10,  Float_t norm=0.1,Int_t NiterThenCov=100,Int_t NburnCov=10,Float_t normThenCov=1):RooMcmc(Niter,Nburn,norm ),fNumItersThenCov(NiterThenCov),fNumBurnInStepsThenCov(NburnCov),fNormThenCov(normThenCov){
	SetNameTitle("HSRooMcmcSeqThenCov","RooMcmcSeqThenCov minimiser");
      }
   // RooMcmcSeqThenCov(const RooMcmcSeqThenCov&)=default;
   // RooMcmcSeqThenCov(RooMcmcSeqThenCov&&)=default;
      ~RooMcmcSeqThenCov() override =default;
   // RooMcmcSeqThenCov& operator=(const RooMcmcSeqThenCov& other)=default;
   // RooMcmcSeqThenCov& operator=(RooMcmcSeqThenCov&& other) = default;  

      void Run(Setup &setup,RooAbsData &fitdata) override;
      Int_t GetNumBurnInSteps()const final{return fNumBurnInStepsThenCov;}

 private:
   
   Int_t fNumItersThenCov=100; //number of iterations to run second metropolis algorithm with covariance proposal
   Int_t fNumBurnInStepsThenCov=50; //Number of burn in steps to discard from the chain to make covariance matrix
   Float_t fNormThenCov=1;

      ClassDefOverride(HS::FIT::RooMcmcSeqThenCov,1);
   };

class RooMcmcSeqThenRMS  : public RooMcmc {
      
 public:
   
 RooMcmcSeqThenRMS(Int_t Niter=100,Int_t Nburn=10,  Float_t norm=0.1,Int_t NiterThenCov=100,Int_t NburnCov=10,Float_t normThenCov=1,Int_t Nthres=500000):RooMcmc(Niter,Nburn,norm ),fNumItersThenCov(NiterThenCov),fNumBurnInStepsThenCov(NburnCov),fNormThenCov(normThenCov){fNumStepsThres = Nthres;
	SetNameTitle("HSRooMcmcSeqThenRMS","RooMcmcSeqThenRMS minimiser");
      }
  //  RooMcmcSeqThenRMS(const RooMcmcSeqThenRMS&)=default;
  //  RooMcmcSeqThenRMS(RooMcmcSeqThenRMS&&)=default;
      ~RooMcmcSeqThenRMS() override =default;
      //  RooMcmcSeqThenRMS& operator=(const RooMcmcSeqThenRMS& other)=default;
      //RooMcmcSeqThenRMS& operator=(RooMcmcSeqThenRMS&& other) = default;  

      void Run(Setup &setup,RooAbsData &fitdata) override;
      Int_t GetNumBurnInSteps()const final{return fNumBurnInStepsThenCov;}


 private:
   
   Int_t fNumItersThenCov=100; //number of iterations to run second metropolis algorithm with covariance proposal
   Int_t fNumBurnInStepsThenCov=50; //Number of burn in steps to discard from the chain to make covariance matrix
   Float_t fNormThenCov=1;
  

      ClassDefOverride(HS::FIT::RooMcmcSeqThenRMS,1);
   };


     class RooMcmcUniform2Seq  : public RooMcmc {
      
    public:

     RooMcmcUniform2Seq(Int_t Niter=100,Int_t Nburn=10, Float_t norm=0.1): RooMcmc(Niter,Nburn,norm){
	SetNameTitle("HSRooMcmcUniform2Seq","RooMcmcUniform2Seq minimiser");
      }
       // RooMcmcUniform2Seq(const RooMcmcUniform2Seq&)=default;
       //RooMcmcUniform2Seq(RooMcmcUniform2Seq&&)=default;
      ~RooMcmcUniform2Seq() override =default;
       //RooMcmcUniform2Seq& operator=(const RooMcmcUniform2Seq& other)=default;
       //RooMcmcUniform2Seq& operator=(RooMcmcUniform2Seq&& other) = default;  

      void Run(Setup &setup,RooAbsData &fitdata) override;

      ClassDefOverride(HS::FIT::RooMcmcUniform2Seq,1);
     };
 
    class RooMcmcGaus  : public RooMcmc {
      
    public:

      RooMcmcGaus(Int_t Niter=100,Int_t Nburn=10, Float_t norm=0.1): RooMcmc(Niter,Nburn,norm){
	SetNameTitle("HSRooMcmcGaus","RooMcmcGaus minimiser");
      }
      //RooMcmcGaus(const RooMcmcGaus&)=default;
      //RooMcmcGaus(RooMcmcGaus&&)=default;
      ~RooMcmcGaus() override =default;
      //RooMcmcGaus& operator=(const RooMcmcGaus& other)=default;
      //RooMcmcGaus& operator=(RooMcmcGaus&& other) = default;  

      void Run(Setup &setup,RooAbsData &fitdata) override;

      ClassDefOverride(HS::FIT::RooMcmcGaus,1);

    };
 
  }//namespace FIT
}//namespace HS

