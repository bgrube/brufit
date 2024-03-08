#!/usr/bin/env python3


import ROOT


def plotDataHists(
  dataFileName,
  sigFileName,
  bkgFileName,
):
  histDef = (";m_{miss} [GeV]", 100, 0, 10)
  dfData = ROOT.RDataFrame("MyModel", dataFileName)
  histData    = dfData.Histo1D(ROOT.RDF.TH1DModel("mMissData", *histDef), "Mmiss")
  histDataSig = dfData.Filter("Sig == 1").Histo1D(ROOT.RDF.TH1DModel("mMissDataSig", *histDef), "Mmiss")
  histDataBkg = dfData.Filter("Sig == -1").Histo1D(ROOT.RDF.TH1DModel("mMissDataBkg", *histDef), "Mmiss")
  dfTemplSig = ROOT.RDataFrame("MyModel", sigFileName)
  histTemplSig = dfTemplSig.Histo1D(ROOT.RDF.TH1DModel("mMissTemplSig", *histDef), "Mmiss")
  dfTemplBkg = ROOT.RDataFrame("MyModel", bkgFileName)
  histTemplBkg = dfTemplBkg.Histo1D(ROOT.RDF.TH1DModel("mMissTemplBkg", *histDef), "Mmiss")

  # draw histograms
  pdfFileName = "data.pdf"
  hists = (histData, histDataSig, histTemplSig, histDataBkg, histTemplBkg)
  canv = ROOT.TCanvas()
  ROOT.gPad.Print(pdfFileName + "[")
  for hist in hists:
    hist.Draw()
    canv.Print(pdfFileName)
  ROOT.gPad.Print(pdfFileName + "]")


def performFit(
  dataFileName,
  sigFileName,
  bkgFileName,
  useSig,
  useBkg,
  outputDir,
):
  # create the sPlot fit manager and set the output directory for fit results, plots, and weights
  RF = ROOT.sPlot()
  RF.SetUp().SetOutDir(outputDir)
  # set RooFit options
  # RF.SetUp().AddFitOption(ROOT.RooFit.PrintLevel(2))
  # RF.SetUp().AddFitOption(ROOT.RooFit.Timer(True))      # prints CPU and wall time of fit steps
  # RF.SetUp().AddFitOption(ROOT.RooFit.BatchMode(True))  # computes a batch of likelihood values at a time, uses faster math functions and possibly auto vectorization
  # RF.SetUp().AddFitOption(ROOT.RooFit.NumCPU(25))       # parallelizes calculation of likelihood using the given number of cores

  # define `Mmiss` as fit variable, this is also the name of the branch in the tree; set the fit range to [0, 10]
  RF.SetUp().LoadVariable("Mmiss[0, 10]")

  # define `fgID` as name of the event-ID variable
  # input tree should have a double branch with this name containing a unique event ID number
  RF.SetUp().SetIDBranchName("fgID")

  if useSig:
    # define signal PDF `Signal` to be a histogram created from the `Mmiss` tree branch
    # the histogram shape is
    #   * convoluted with a Gaussian with width `smear_Sig`; initial width 0, allowed range [0, 20]
    #   * shifted by an offset `off_Sig`; initial offset 0, allowed range [-2, +2]
    #   * scaled along the x axis by a factor `scale_Sig`; initial scale 1, allowed range [0.8, 1.2]
    # RF.SetUp().FactoryPDF("RooHSEventsHistPDF::Signal(Mmiss, smear_Sig[0, 0, 20], off_Sig[0, -2, 2], scale_Sig[1, 0.8, 1.2])")
    RF.SetUp().FactoryPDF("RooHSEventsHistPDF::Signal(Mmiss, smear_Sig[0], off_Sig[0], scale_Sig[1])")
    # load data from which histogram PDFs are constructed
    RF.LoadSimulated("MyModel", sigFileName, "Signal")  # tree name, file name, PDF name
    RF.SetUp().LoadSpeciesPDF("Signal", 1)

  if useBkg:
    # define background PDF `BG` in the same way as above
    # RF.SetUp().FactoryPDF("RooHSEventsHistPDF::BG(Mmiss, smear_Bkg[0, 0, 5], off_Bkg[0, 0, 0], scale_Bkg[1.0, 0.8, 1.2])")
    RF.SetUp().FactoryPDF("RooHSEventsHistPDF::BG(Mmiss, smear_Bkg[0], off_Bkg[0], scale_Bkg[1])")
    # load data from which histogram PDFs are constructed
    RF.LoadSimulated("MyModel", bkgFileName, "BG")
    RF.SetUp().LoadSpeciesPDF("BG", 1)

  # load data to be fitted
  RF.LoadData("MyModel", dataFileName)  # tree name, file name

  # perform fit and plot fit result
  ROOT.Here.Go(RF)

  # plot fit result
  fitResultFile = ROOT.TFile.Open(f"{outputDir}/ResultsHSMinuit2.root", "READ")
  canvName = f"_Mmiss"
  canv = fitResultFile.Get(canvName)
  canv.SaveAs(f"{outputDir}/ResultsHSMinuit2.pdf")
  fitResultFile.Close()


if __name__ == "__main__":
  ROOT.gROOT.SetBatch(True)
  ROOT.gROOT.ProcessLine(".x ${BRUFIT}/macros/LoadBru.C")

  dataFileName = "Data.root"
  sigFileName  = "SigData.root"
  bkgFileName  = "BGData.root"

  # plotDataHists(dataFileName, sigFileName, bkgFileName)

  # fit signal with true template
  performFit(
    dataFileName = sigFileName,
    sigFileName  = sigFileName,
    bkgFileName  = None,
    useSig       = True,
    useBkg       = False,
    outputDir    = "outSig",
  )
  # fit background with true template
  performFit(
    dataFileName = bkgFileName,
    sigFileName  = None,
    bkgFileName  = bkgFileName,
    useSig       = False,
    useBkg       = True,
    outputDir    = "outBkg",
  )
  # fit total distribution with true templates
  performFit(
    dataFileName = dataFileName,
    sigFileName  = sigFileName,
    bkgFileName  = bkgFileName,
    useSig       = True,
    useBkg       = True,
    outputDir    = "out",
  )

  print("Finished successfully")
