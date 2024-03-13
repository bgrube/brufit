#!/usr/bin/env python3


import ROOT


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

  # define `MissingMassSquared_Measured` as fit variable, this is also the name of the branch in the tree; set the fit range to [0, 10]
  RF.SetUp().LoadVariable("MissingMassSquared_Measured[-0.25, 3.75]")

  # define `ComboID` as name of the event-ID variable
  # input tree should have a double branch with this name containing a unique event ID number
  RF.SetUp().SetIDBranchName("ComboID")

  if useSig:
    # define signal PDF `Signal` to be a histogram created from the `MissingMassSquared_Measured` tree branch
    # the histogram shape is
    #   * convoluted with a Gaussian with width `smear_Sig`; initial width 0, allowed range [0, 20]
    #   * shifted by an offset `off_Sig`; initial offset 0, allowed range [-2, +2]
    #   * scaled along the x axis by a factor `scale_Sig`; initial scale 1, allowed range [0.8, 1.2]
    # RF.SetUp().FactoryPDF("RooHSEventsHistPDF::Signal(MissingMassSquared_Measured, smear_Sig[0, 0, 20], off_Sig[0, -2, 2], scale_Sig[1, 0.8, 1.2])")
    RF.SetUp().FactoryPDF("RooHSEventsHistPDF::Signal(MissingMassSquared_Measured, smear_Sig[0], off_Sig[0], scale_Sig[1])")
    # load data from which histogram PDFs are constructed
    RF.LoadSimulated("pippippimpimpmiss", sigFileName, "Signal")  # tree name, file name, PDF name
    RF.SetUp().LoadSpeciesPDF("Signal", 1)

  if useBkg:
    # define background PDF `BG` in the same way as above
    # RF.SetUp().FactoryPDF("RooHSEventsHistPDF::BG(MissingMassSquared_Measured, smear_Bkg[0, 0, 5], off_Bkg[0, 0, 0], scale_Bkg[1.0, 0.8, 1.2])")
    RF.SetUp().FactoryPDF("RooHSEventsHistPDF::BG(MissingMassSquared_Measured, smear_Bkg[0], off_Bkg[0], scale_Bkg[1])")
    # load data from which histogram PDFs are constructed
    RF.LoadSimulated("pippippimpimpmiss", bkgFileName, "BG")
    RF.SetUp().LoadSpeciesPDF("BG", 1)

  # load data to be fitted
  RF.LoadData("pippippimpimpmiss", dataFileName)  # tree name, file name

  # perform fit and plot fit result
  ROOT.Here.Go(RF)

  # plot fit result
  fitResultFile = ROOT.TFile.Open(f"{outputDir}/ResultsHSMinuit2.root", "READ")
  canvName = f"_MissingMassSquared_Measured"
  canv = fitResultFile.Get(canvName)
  canv.SaveAs(f"{outputDir}/ResultsHSMinuit2.pdf")
  fitResultFile.Close()


if __name__ == "__main__":
  ROOT.gROOT.SetBatch(True)
  ROOT.gROOT.ProcessLine(".x ${BRUFIT}/macros/LoadBru.C")

  bggenFileName = "/w/halld-scshelf2101/bgrube/ProtonTrackEfficiency/ReactionEfficiency/data/MCbggen/2017_01-ver03_goodToF/pippippimpimpmiss_flatTree.MCbggen_2017_01-ver03_goodToF.root.brufit"
  dataFileName  = bggenFileName
  # create separate signal and background files
  sigFileName   = "SigData.root"
  bkgFileName   = "BGData.root"
  dfData = ROOT.RDataFrame("pippippimpimpmiss", dataFileName)
  columnNames = dfData.GetColumnNames()
  dfData.Filter("(IsSignal == 1)").Snapshot("pippippimpimpmiss", sigFileName, columnNames)
  dfData.Filter("(IsSignal == 0)").Snapshot("pippippimpimpmiss", bkgFileName, columnNames)

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
