/**
	\class Weights
	
	Class to control weight structures in HaSpect.
	Consists of 1 tree with event IDs
	And 1 tree with the weights of the different species
	It relies on being able to read the trees into memory
	to sort the ordering, this may cause issues with order >10^7
	events
	
	
*/

#include "Weights.h"
#include <TTreeIndex.h>
#include <TSystem.h>
#include <TFile.h>
#include <TMath.h>
#include <TCut.h>
#include <TH1.h>
#include <TLeaf.h>
#include <TEntryList.h>
#include <TError.h>


namespace HS{
  namespace FIT{



    Weights::Weights(const TString& name) :TNamed(name,name){
      fWTree=new TTree(name+"_W","Tree weights for each species");
      fWTree->SetDirectory(nullptr);
      fIDTree=new TTree(name+"_ID","event ids for each entry");
      fIDTree->SetDirectory(nullptr);
      fIDTree->Branch("WID",&fID,"WID/L");
      fCurrEntry=0;
      fIsSorted=kFALSE;
      fN=0;
      fSpecies.clear();
      fIDName="UID";
    }

    Weights::~Weights(){
      if(fFile)Save();			       
      if(fWeightList) delete fWeightList;
      if(fWTree) delete fWTree;
      if(fIDTree) delete fIDTree; 
      if(fFile) {delete fFile;}
      if(fBranchFile) delete fBranchFile;
    }

    void Weights::SetSpecies(TString name){
      UInt_t NSpecies=fSpecies.size(); 
      fSpecies.insert(pair<TString,Int_t>(name,NSpecies)); //save name in map
      fWVals.ResizeTo(NSpecies+1); //create entry in value array
      //make branch with name and connect to new fWVals entry
      fWVals[NSpecies]=0;
      cout<<"Insert species "<<name <<" "<<NSpecies<<" "<<fWVals[NSpecies]<<endl;
      fWTree->Branch(name,&fWVals[NSpecies],name+TString("/D")); 
    }

    //////////////////////////////////////////////////////////////
    ///Move down the tree until you find an event with correct ID.
    ///This requires the trees are sorted in ID
    ///and fWTree is a subset of whatever tree requires the weight.
    ///Fastest if both trees have same events
    Bool_t Weights::GetEntryFast(Long64_t id){

      if(!fIDv) BuildIndex();
      if(id!=fIDv[fCurrEntry]){//no weight for this id
	return fGotEntry=kFALSE;}
      fWTree->GetEntry(fCurrEntry++); //get entry now we know it exists 
      return fGotEntry=kTRUE;
    }

    ////////////////////////////////////////////////////////////
    ///for unsorted tree \n
    ///Move down the tree until find event with correct ID.
    ///This is faster if trees are sorted in ID and ReStart not required
    ///(in which case use GetEntryFast)
    ///and fWTree is a subset of whatever tree requires the weight.
    ///Fastest if both trees have same events
    Bool_t Weights::GetEntrySlow(Long64_t id){
      if(!fIDv) BuildIndex();
      Bool_t ReStart=false;
      if(id!=fIDv[fCurrEntry++]){//no weight for this id
	//    fIDTree->GetEntry(fCurrEntry++);
	if(fCurrEntry==fWTree->GetEntries()&&ReStart==0){fCurrEntry=0; ReStart=true;}
	else if(fCurrEntry==fWTree->GetEntries()){
	  cout<<"Weights::GetEntry entry not found "<<id<<endl; 
	  return fGotEntry=kFALSE;}
      }
      fCurrEntry--;
      fWTree->GetEntry(fCurrEntry++); //get entry now we know it exists 
      return fGotEntry=kTRUE;
    }

    /////////////////////////////////////////////////////////////
    ///Use a binary search to find the entry for an unsorted tree
    Bool_t Weights::GetEntryBinarySearch(Long64_t id){

      if(!fIDv) BuildIndex();
      Long64_t entry=TMath::BinarySearch(fN,fIDv,id);
      if(fIDv[entry]!=id) return fGotEntry=kFALSE;
      fWTree->GetEntry(fIDi[entry]);//entry should be OK if these arrays are be ordered...
      return fGotEntry=kTRUE;
    }

    //////////////////////////////////////////////////
    ///Function to merge weights from many root files
    Long64_t Weights::Merge(const TString& tempName,const TString& outName,const TString& wmName){

      if(outName!=TString("")) {
	//TFile* outFile=new TFile(outName,"recreate");
	SetFile(outName);
      }
      TString dirName=gSystem->DirName(tempName);
      TString prefix=gSystem->BaseName(tempName); //anything after directory in tempname
      if(prefix==TString("")) prefix="Weights";
      void *dir=gSystem->OpenDirectory(dirName);
      if(!dir) cout<<"Weights::Merge No directory found : "<<dirName<<endl;
      else cout<<"Weights::Merge Merging "<<prefix <<"* in directory "<<dirName<<endl;
      TString fileName;
      while( (fileName=(gSystem->GetDirEntry(dir)))){
	if(fileName==TString("")) break;
	if(fileName==TString("."))continue;
	if(fileName==TString(".."))continue;
	if(!fileName.Contains(prefix))continue;
	if(!fileName.Contains(".root"))continue;
	auto* wm=new Weights();
	wm->LoadSaved(dirName+"/"+fileName,wmName);
	fIDName=wm->GetIDName();
	Add(wm);
	delete wm;
	//wfile->Close();
	//delete wfile;
      }
      gSystem->FreeDirectory(dir);
  
      SortWeights();//needs sorted for binary search to work
      PrintWeight();
      return Size();
  
    }

    //////////////////////////////////////
    ///Find the name for a given index
    TString Weights::GetSpeciesName(UInt_t isp){
      if(isp>=fSpecies.size()) return TString("");
      //find the name for a given index
      for(auto & fSpecie : fSpecies)
	if(fSpecie.second==(Int_t)isp) return fSpecie.first;
      return TString("");
    }

    /////////////////////////////////////////////////////////////
    ///Add the tree \n
    ///Zero weight values in case of missing species in new weights.
    ///These branches will then just be filled with zero weight for
    ///the new entries.
    void Weights::Add(Weights* Wts){
      StrIntMap_t *sp0=GetSpeciesp();
      StrIntMap_t *sp1=Wts->GetSpeciesp();
      UInt_t Ns0=sp0->size();
      UInt_t Ns1=sp1->size();
      UInt_t NnewSp=0;
      StrIntMap_t New_sp; //map for additional species from Wts with original index
      //Check for new species
      for(auto & it1 : *sp1){
	if(!(sp0->count(it1.first))){//this species is not in map already
	  SetSpecies(it1.first);//add new species branch
	  for(Long64_t ie=0;ie<fWTree->GetEntries();ie++)//fill previous entries with 0
	    fWTree->GetBranch(it1.first)->Fill();
	}
      }
      //Add the tree
      for(UInt_t isp=0;isp<Ns0;isp++)
	fWVals[isp]=0;
      //if(New_sp==0){//no new species, just merge trees
      auto* tlist=new TList();
      tlist->Add(Wts->GetTree());
      fWTree->Merge(tlist);
      delete tlist;
      auto* wlist=new TList();
      wlist->Add(Wts->GetIDTree());
      fIDTree->Merge(wlist);
      delete wlist;
      //make a list of weights added, this can be used to select contributing entrylists
      if(!fWeightList) {fWeightList=new TList();fWeightList->SetOwner();}
      fWeightList->Add(new TNamed(Wts->GetTitle(),""));//include name of this bin

    }

	////////////////////////////////////////////////////////
	///Multiply weights from a different weights file onto this weights file \n
	///This is useful for combining weights from different sources.
	void Weights::Multiply(Weights* other,TString species){
		Int_t ispecies = fSpecies[species];
		Int_t ospecies = other->GetSpeciesID(species);
		
		if(ispecies<0 || ospecies<0) //check that species exist in both trees
			return;
		
		TTree *wtree=fWTree->CloneTree(0); //create empty tree with branch adresses set
		TLeaf *leafid=fIDTree->GetLeaf("WID");
		
		Double_t newWeight;
		wtree->SetBranchAddress(species,&newWeight); // newWeight = this.Weight * other.Weight
		
		for(Int_t i=0;i<fN;i++){
			fIDTree->GetEntry(i);
			Double_t id = leafid->GetValue();
			
			newWeight = 0;
			if(GetEntryBinarySearch(id) && other->GetEntryBinarySearch(id) ){//find the weight for this event in both files, if it only exists in one -> newWeight=0
				newWeight = GetWeight(ispecies)*other->GetWeight(ospecies);
			}
			wtree->Fill();
		}
		
		//swap sorted trees to datamembers
		delete fWTree;fWTree=nullptr;
		fWTree=wtree;
			
		fCurrEntry=0;
		SortWeights();
	}

    void Weights::PrintWeight(){
      cout<<"Weights "<<GetName()<<" contains "<<Size() <<" events associated file is ";
      if(fFile) cout<<fFile->GetName()<< " "<<fFile->GetTitle()<<endl;
      else cout<<endl;
      cout<<"ID branch name : "<<fIDName<<endl;
      cout<<"Species are : "<<endl;
      for(auto & fSpecie : fSpecies)
	cout<<fSpecie.first<<endl;
      Int_t Nit=0;
      cout<<"The first ten entries are :"<<endl;
      Int_t Nw=10;
      if(Size()<10) Nw=Size();
      for(Int_t i=0;i<Nw;i++){
	// for(Int_t i=3900;i<4100;i++){
	GetEntry(i);
	cout<<fID<<" "<<fWVals[0]<<" ";
	for(UInt_t iss=1;iss<fSpecies.size();iss++)
	  cout<<fWVals[iss]<< " ";
	cout<<endl;
      }
      if(fWeightList){
	cout<<"These weights are combined from :"<<endl;
	for(Int_t i=0;i<fWeightList->GetEntries();i++)
	  cout<<fWeightList->At(i)->GetName()<<endl;
      }
    }

    void Weights::BuildIndex(){
      // cout<<"Weights::BuildIndex "<<fIDTree->BuildIndex(TString("(Long64_t)WID"))<<endl;
      fIDTree->BuildIndex(TString("WID"));
      //  fIDTree->BuildIndex(TString("(Long64_t)WID"));
      auto *index = dynamic_cast<TTreeIndex*>(fIDTree->GetTreeIndex());
      fIDi=index->GetIndex();//entry numbers
      fIDv=index->GetIndexValues();//id values
      fN=fWTree->GetEntries();
    }

    ////////////////////////////////////////////////////////////////////////////
    ///GetEntryFast only works properly on trees where the ID is in order \n
    ///This is not guaranteed particualrly if weights are merged from different bins,
    ///reorder here
    void Weights::SortWeights(){
      TTree* idtree=fIDTree->CloneTree(0); //create empty tree with branch adresses set //Clone before create index so do not have to save index
      idtree->SetDirectory(fFile); //set file to save memory
      BuildIndex();
      TTree* Mwtree=fWTree->CloneTree(); //create clone tree in memory or very slow!
      Mwtree->SetDirectory(nullptr);
      cout<<"Weights::SortWeights() Clone tree to save "<<endl;
      TTree* wtree=fWTree->CloneTree(0); //create empty tree with branch adresses set
      wtree->SetDirectory(fFile);//set file to save memory
      for( Long64_t i =  0; i < fN ; i++ ) {
	fID=fIDv[i];
	idtree->Fill(); //fill as ordered by the build index
	//wtree is synched with id tree
	Mwtree->GetEntry(fIDi[i]);
	wtree->Fill(); //fill as ordered by the build index
      }
      //swap sorted trees to datamembers
      delete fIDTree;fIDTree=nullptr;
      delete fWTree;fWTree=nullptr;
      delete Mwtree;Mwtree=nullptr;
      fIDTree=idtree;
      fWTree=wtree;
      //resetbranch addresses
      fIDTree->SetBranchAddress("WID",&fID);
      for(UInt_t iss=0;iss<fSpecies.size();iss++)
	for(auto & fSpecie : fSpecies)
	  fWTree->SetBranchAddress(fSpecie.first,&fWVals[fSpecie.second]);
  
      //  cout<<"Weights::SortWeights Print new ordering"<<endl;
      // PrintWeight();
      //reset index
      fIDv=nullptr;//these have been deleted with orig fIDTree
      fIDi=nullptr;
 
      fIsSorted=kTRUE;
    }
    /////////////////////////////////////////////////////////////////
    ///Set file for keeping weights on disk
    //Should be done before sort etc to save memory
    void Weights::SetFile(const TString& filename){
      TDirectory *saveDir=gDirectory;
      fFile=new TFile(filename,"recreate");
      if(fIDTree)fIDTree->SetDirectory(fFile);
      if(fWTree)fWTree->SetDirectory(fFile);
      saveDir->cd();
    }
    ////////////////////////////////////////////////////////////////
    ///Finally save weights to disk
    void Weights::Save(){
      // cout<<"void Weights::Save() "<<fFile<<endl;
      //cout<<fIDTree<<" "<<fWTree<<endl;
      if(!fFile) {cout<<"Weights::Save() no file associated with "<<GetName()<<" so not saved"<<endl;return;}
      if(!fFile->IsWritable()) return;
      if(!fIDTree)return ;
      if(!fWTree)return ;
      fFile->cd();
      fWTree->Write();//Note can't just save whole object
      fIDTree->Write(); //As 1GB limit on object buffers in TFile
      Write();//save the rest (not trees) of the weights class
 
      delete fFile;fFile=nullptr;fWTree=nullptr;fIDTree=nullptr;

    }
    ///////////////////////////////////////////////////////////////
    ///Give file name and name (in .root file) of weights object to load weights
    ///into and empty weights object
    ///e.g. Weights* wts=new Weights();
    ///     wts->LoadSaved("path_to_/Weight_File.root","HSWeight");
    void Weights::LoadSaved(const TString& fname,const TString& wname){
      TDirectory* savedir=gDirectory;
      auto* wfile=new TFile(fname);
      if(!wfile) return;
  
      auto* file_wts=dynamic_cast<Weights*>(wfile->Get(wname));//read into memory
      if(!file_wts) return;
      fName=file_wts->GetName();
      fTitle=file_wts->GetTitle();

      savedir->cd();
      //Going to create trees in memoery so trun off warnings
      auto saveIgnore =  gErrorIgnoreLevel;
      gErrorIgnoreLevel=kFatal;
      
      TTree* tempTree=nullptr;
      tempTree=dynamic_cast<TTree*>(wfile->Get(wname+"_W"));
      fWTree=tempTree->CloneTree();
      delete tempTree;
      fWTree->SetDirectory(nullptr);
      fSpecies=file_wts->GetSpecies();
      fWVals.ResizeTo(fSpecies.size());
      for(UInt_t i=0;i<fSpecies.size();i++)
	fWTree->SetBranchAddress(GetSpeciesName(i),&fWVals[i]); 

      fIDName=file_wts->GetIDName();
      tempTree=dynamic_cast<TTree*>(wfile->Get(wname+"_ID"));
      fIDTree=tempTree->CloneTree();
      // fIDTree=(TTree*)file_wts->GetIDTree()->Clone();
      delete tempTree;
      fIDTree->SetDirectory(nullptr);
      fIDTree->SetBranchAddress("WID",&fID);
 
      fCurrEntry=0;
      fIsSorted=kFALSE;
      fN=fWTree->GetEntries();
      delete file_wts;file_wts=nullptr;  
      wfile->Close();
      delete wfile;wfile=nullptr;

      gErrorIgnoreLevel=saveIgnore;
    }
    void Weights::LoadSavedDisc(const TString& fname,const TString& wname){
      TDirectory* savedir=gDirectory;
      auto* wfile=new TFile(fname);
      if(!wfile) return;
  
      auto* file_wts=dynamic_cast<Weights*>(wfile->Get(wname));//read into memory
      if(!file_wts) return;
      fName=file_wts->GetName();
      fTitle=file_wts->GetTitle();

      savedir->cd();
      TTree* tempTree=nullptr;
      tempTree=dynamic_cast<TTree*>(wfile->Get(wname+"_W"));
      fWTree=tempTree;
      //  delete tempTree;
      //fWTree->SetDirectory(0);
      fSpecies=file_wts->GetSpecies();
      fWVals.ResizeTo(fSpecies.size());
      for(UInt_t i=0;i<fSpecies.size();i++)
	fWTree->SetBranchAddress(GetSpeciesName(i),&fWVals[i]); 

      fIDName=file_wts->GetIDName();
      tempTree=dynamic_cast<TTree*>(wfile->Get(wname+"_ID"));
      fIDTree=tempTree;
      // fIDTree=(TTree*)file_wts->GetIDTree()->Clone();
      //delete tempTree;
      //fIDTree->SetDirectory(0);
      fIDTree->SetBranchAddress("WID",&fID);
 
      fCurrEntry=0;
      fIsSorted=kFALSE;
      fN=fWTree->GetEntries();
      // delete file_wts;file_wts=nullptr;  
      //wfile->Close();
      //delete wfile;wfile=nullptr;
    }

    ////////////////////////////////////////////////////////////
    ///Given a tree selection weight events that pass with wgt
    ///and enter the weight into this object
    void Weights::WeightBySelection(TTree* tree,const TCut& cut,Double_t wgt){
      TDirectory *saveDir=gDirectory;
      if(fFile) fFile->cd();
      //Find events which pass cut
      tree->Draw(">>wlist",cut,"entrylist");
      TEntryList *elist =nullptr; 
      elist=dynamic_cast<TEntryList*>(gDirectory->Get("wlist"));
      elist->Print();
      Long64_t listEntries = elist->GetN();
      //Now only want ID branch
      tree->SetBranchStatus("*",false);
      tree->SetBranchStatus(fIDName,true);
      Double_t wID=0;
      tree->SetBranchAddress(fIDName,&wID);
  
      tree->SetEntryList(elist,"");
      //loop over events which passed cut
      for (Long64_t el = 0; el < listEntries; el++) {
	Long64_t entryNumber = tree->GetEntryNumber(el);
	tree->GetEntry(entryNumber);
	FillWeight(wID,wgt);
      }
      delete elist;elist=nullptr;
      tree->SetEntryList(nullptr);
  
      //turn all branches back on
      tree->ResetBranchAddresses();
      tree->SetBranchStatus("*",true);
      cout<<"Weights::WeightBySelection Added "<<listEntries<<" events with a weight of "<<wgt<<" from selection " <<cut<<endl;
      saveDir->cd();
    }


    ////////////////////////////////////////////////////////////
    ///Given a tree selection weight events that pass with wgt
    ///and enter the weight into this object. wgt is a branch in the tree
    void Weights::WeightBySelection(TTree* tree,const TCut& cut,const TString& wgt){
      TDirectory *saveDir=gDirectory;
      if(fFile) fFile->cd();
      //Find events which pass cut
      tree->Draw(">>wlist",cut,"entrylist");
      TEntryList *elist =nullptr; 
      elist=dynamic_cast<TEntryList*>(gDirectory->Get("wlist"));
      elist->Print();
      Long64_t listEntries = elist->GetN();
      //Now only want ID branch
      tree->SetBranchStatus("*",false);
      tree->SetBranchStatus(fIDName,true);
      tree->SetBranchStatus(wgt,true);
      Double_t wID=0;
      Double_t eventWeight;
      tree->SetBranchAddress(fIDName,&wID);
      tree->SetBranchAddress(wgt,&eventWeight);
  
      tree->SetEntryList(elist,"");
      //loop over events which passed cut
      for (Long64_t el = 0; el < listEntries; el++) {
	Long64_t entryNumber = tree->GetEntryNumber(el);
	tree->GetEntry(entryNumber);
	FillWeight(wID,eventWeight);
      }
      delete elist;elist=nullptr;
      tree->SetEntryList(nullptr);
  
      //turn all branches back on
      tree->ResetBranchAddresses();
      tree->SetBranchStatus("*",true);
      cout<<"Weights::WeightBySelection Added "<<listEntries<<" events with a weight taken from branch " << wgt << " from tree " << tree->GetName() << " according to selection " <<cut<<endl;
      saveDir->cd();
    }

    ////////////////////////////////////////////
    ///This creates a new file with a copy of the original tree + wname
    // filed_uptr  Weights::DFAddToTree(const TString& wname,const TString& outfname,const TString& tname,const TString& infname){
    //   //  ROOT::RDataFrame df(tname.Data(),fname.Data(),{GetIDName().Data()});
    //   Int_t isp=GetSpeciesID(wname);
    //   auto applyWeights = [this,&isp](double id ) {
    // 	GetEntryBinarySearch((Long64_t)id);
    // 	return (double)(GetWeight(isp));
    //   };
    //   ROOT::RDataFrame df(tname.Data(),infname.Data(),{GetIDName().Data()});
    //   // df.Define(wname,applyWeights);
    //   //  return DFdef_uptr(new DFdef_t(df.Define(wname.Data(),applyWeights)));
    //   df.Define(wname.Data(),applyWeights).Snapshot(tname.Data(),outfname.Data());

    //   return std::move(FiledTree::Read(tname.Data(),outfname.Data()));
    // }
    /////////////////////////////////////////////
    // void Weights::AddToTree(TString outfname,TString tname,TString infname){

    // }
    void Weights::AddToTree(TTree* tree){
      vector<TBranch*> branches;
      tree->SetBranchStatus("*",false);
      tree->SetBranchStatus(fIDName,true);

      const UInt_t Nsp=fSpecies.size();
      for(UInt_t i=0;i<Nsp;i++)
	branches.push_back(tree->Branch(GetSpeciesName(i),&fWVals[i]));
 

      auto id_leaf=tree->GetLeaf(fIDName);
      if(!id_leaf) {
	cout<<" ERROR Weights::AddToTree weights idname not found in tree : " <<fIDName<<endl;
	tree->Print();
      }
  
      auto Nentries=tree->GetEntries();
      for(Long64_t ient=0;ient<Nentries;ient++){
	tree->GetEntry(ient);
 
	GetEntryBinarySearch((Long64_t)id_leaf->GetValue());
 
	if(!GotEntry())
	  for(UInt_t ivec=0;ivec<Nsp;ivec++)
	    fWVals[ivec]=0;
 
	for(auto* br: branches)
	  br->Fill();
      }
 
      tree->ResetBranchAddresses();

    }
    void Weights::AddToTreeDisc(TTree* tree,const TString& fileName){
      TDirectory* saveDir=gDirectory;
      fBranchFile=TFile::Open(fileName,"recreate");

      vector<TBranch*> branches;
      tree->SetBranchStatus("*",false);
      tree->SetBranchStatus(fIDName,true);

      const UInt_t Nsp=fSpecies.size();
      for(UInt_t i=0;i<Nsp;i++){
	TBranch* branch=tree->Branch(GetSpeciesName(i),&fWVals[i]);
	branch->SetFile(fBranchFile);
	branches.push_back(branch);
      }

      auto id_leaf=tree->GetLeaf(fIDName);
      if(!id_leaf) {
	cout<<" ERROR Weights::AddToTree weights idname not found in tree : " <<fIDName<<endl;
	tree->Print();
      }
  
      auto Nentries=tree->GetEntries();
      for(Long64_t ient=0;ient<Nentries;ient++){
	tree->GetEntry(ient);
 
	GetEntryBinarySearch((Long64_t)id_leaf->GetValue());
 
	if(!GotEntry())
	  for(UInt_t ivec=0;ivec<Nsp;ivec++)
	    fWVals[ivec]=0;
 
	for(auto* br: branches)
	  br->Fill();
      }
 
      tree->ResetBranchAddresses();
      saveDir->cd();
    }

	////////////////////////////////////////////////////////////
	/// LC Jul 2018
	/// PP Jan 2020
	/// Importance Sampling
	/// Given a data tree and a simulation tree, output the weights required to adjust the simulated distribution 
	/// to match the data distribution for the given variable
	void Weights::ImportanceSampling(TTree* MCTree, TTree* dataTree, TH1* weightHist, TString var, Weights* MCWeights, TString MCWeightsSpecies, Weights* DataWeights, TString DataWeightsSpecies) {

		// create sim and data hists based on weightHist (empty hist with appropriate axis and binning)
		TH1* mcHist = (TH1*) weightHist->Clone();
		mcHist->SetName("mcHist");
		TH1* dataHist = (TH1*) weightHist->Clone();
		dataHist->SetName("dataHist");
		
		// fill the histograms from the trees with the quantity specified by var parameter, apply weights if specified
		if(MCWeights == nullptr){
			MCTree->Draw(var+">>mcHist","","goff");
		}
		else{
			MCWeights->Draw1DWithWeights(MCTree,mcHist,var,MCWeightsSpecies);
		}
		if(DataWeights == nullptr){
			dataTree->Draw(var+">>dataHist","","goff");
		}
		else{
			DataWeights->Draw1DWithWeights(dataTree,dataHist,var,DataWeightsSpecies);
		}
		
		// create hist with ratio of data to MC
		TH1* ratioHist = (TH1*) dataHist->Clone();
		ratioHist->Divide(mcHist);
		
		// loop around the MC tree filling weights
		Double_t wID=0;
		Double_t val=0;
		MCTree->SetBranchAddress(fIDName,&wID);
		MCTree->SetBranchAddress(var,&val);
		Int_t nentries = MCTree->GetEntries();
		for (int i=0; i<nentries; i++) {
			Double_t mcweight = 1;
			MCTree->GetEntry(i);
			if(!(MCWeights == nullptr)){
				MCWeights->GetEntryBinarySearch(wID);
				Int_t MCspeciesID = MCWeights->GetSpeciesID(MCWeightsSpecies);
				mcweight=MCWeights->GetWeight(MCspeciesID);
			}
			FillWeight(wID, 
				mcweight*ratioHist->GetBinContent( ratioHist->GetXaxis()->FindBin(val)));	
			//  cout << var << " value is " << val << " weight is " << ratioHist->GetBinContent(ratioHist->GetXaxis()->FindBin(val)) << endl;
			
		}
		MCTree->ResetBranchAddresses();
		//  cout << "nentries is " << nentries << endl;
	}
	///////////////////////////////////////////////////////////
	/// Draw a TH1 histogram with weights applied
	void  Weights::Draw1DWithWeights(TTree* tree, TH1* his, TString var, TString species){
		TLeaf *leafVar=tree->GetLeaf(var);
		TLeaf *leafID=tree->GetLeaf(fIDName);
		Int_t ispecies=0;
		if(!(species==TString("")))
			ispecies=fSpecies[species];
		
		cout << "HS::Weights::Draw1DWithWeights species:ispecies = " << species << ":" << ispecies << endl;
		for(Int_t i=0;i<tree->GetEntries();i++){
			tree->GetEntry(i);
			
			if(GetEntryBinarySearch(leafID->GetValue())){//find the weight for this event
				his->Fill(leafVar->GetValue(),GetWeight(ispecies));
			}
		}
	}


    //////////////////////////////////
  }

}
