#pragma once

#include <TTree.h>
#include <TString.h>

#include <utility>

namespace HS{
  namespace FIT{

    using strings_t = std::vector<TString>;

    class BootStrapper {
      
    public:
      BootStrapper(Int_t N);
      BootStrapper() =default;
      BootStrapper(const BootStrapper&)=default;
      BootStrapper(BootStrapper&&)=default;
      virtual ~BootStrapper()=default;
      BootStrapper& operator=(const BootStrapper& other)=default;
      BootStrapper& operator=(BootStrapper&& other) = default;


      void DivideData(const TString& tname,const TString& fname);
      void DivideData(TTree* tree);

      TTree* BootStrapTree(TTree* tree) const;

      strings_t GetFileNames(){return fFileNames;}
      void SetOutDir(TString outdir){fOutDir=std::move(outdir);}

      Int_t GetN(){return fNBoots;}

      Int_t GetGroup(Int_t i){ return (int)std::round(i/fNBoots);}
      Int_t GetBootID(Int_t i) {return i%fNBoots;}
    private :
      strings_t fFileNames;

      TString fOutDir;
      Int_t fNBoots{};
      
    };
      
  }//namespace FIT
}//namesapce HS
