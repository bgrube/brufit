#!/usr/bin/env python3

import functools
import math

import ROOT


# always flush print() to reduce garbling of log files due to buffering
print = functools.partial(print, flush = True)


def plotDataHists(
  dataFileName,
  sigFileName,
  bkgFileName,
  pdfFileName = "data.pdf",
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
    # RF.SetUp().FactoryPDF("RooHSEventsHistPDF::Signal(Mmiss, smear_Sig[0], off_Sig[0], scale_Sig[1, 0.8, 1.2])")
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

  # get fitted yields
  fitResultFile = ROOT.TFile.Open(f"{outputDir}/ResultsHSMinuit2.root", "READ")
  fitResult = fitResultFile.Get("MinuitResult")
  fitPars   = fitResult.floatParsFinal()
  yieldVals = {"Yld_Signal": None, "Yld_BG": None}
  for fitParName in yieldVals:
    fitParIndex = fitPars.index(fitParName)
    if fitParIndex < 0:
      yieldVals[fitParName] = (0, 0)
    else:
      fitPar = fitPars[fitParIndex]
      yieldVals[fitParName] = (fitPar.getVal(), fitPar.getError())

  # plot fit result
  canvName = f"_Mmiss"
  canv = fitResultFile.Get(canvName)
  canv.SaveAs(f"{outputDir}/ResultsHSMinuit2.pdf")
  fitResultFile.Close()

  return yieldVals


def plotPulls(
  yieldVals,
  yieldValsTruth,
  outputDir,
):
  yieldNames = yieldVals[0].keys()
  for yieldName in yieldNames:
    canv = ROOT.TCanvas()
    histPulls = ROOT.TH1D(f"histPulls_{yieldName}", ";(#hat{Y} - Y_{True}) / #hat{#sigma}_{Y};Count", 50, -5, 5)
    for iYield, yieldVal in enumerate(yieldVals):
      if yieldVal[yieldName][1] == 0:
        continue
      pull = (yieldVal[yieldName][0] - yieldValsTruth[iYield][yieldName]) / yieldVal[yieldName][1]
      histPulls.Fill(pull)
    histPulls.Draw()
    canv.SaveAs(f"{outputDir}/pulls_{yieldName}.pdf")


if __name__ == "__main__":
  ROOT.gROOT.SetBatch(True)
  ROOT.gROOT.ProcessLine(".x ${BRUFIT}/macros/LoadBru.C")

  nmbSamples   = 500     # number of statistically independent samples to generate
  nmbEvents    = 100000  # defined in Model1.C
  fitTruth     = True    # if set, the true templates are used for the fit of every sample; the fit should hence match the data perfectly
  dataFileName = "Data.root"

  ROOT.gROOT.LoadMacro("Model1.C")
  if fitTruth:
    sigFileName  = f"Sig{dataFileName}"
    bkgFileName  = f"BG{dataFileName}"
  else:
    sigFileName  = f"SigTempl{dataFileName}"
    bkgFileName  = f"BGTempl{dataFileName}"
    ROOT.Model1(f"Templ{dataFileName}")  # generate independent data from which templates are taken; use same template for all fits

  yieldValsSig   = []
  yieldValsBkg   = []
  yieldVals      = []
  yieldValsTruth = []
  for iSample in range(nmbSamples):
    print(f"Performing fit {iSample + 1} of {nmbSamples}.")
    ROOT.Model1(dataFileName)  # generate data to fit; if fitTruth is True templates are taken from the same file
    # plotDataHists(dataFileName, sigFileName, bkgFileName, "data.pdf")

    dfData = ROOT.RDataFrame("MyModel", dataFileName)
    yieldValsTruth.append({
      "Yld_Signal": dfData.Filter("Sig == 1" ).Count().GetValue(),
      "Yld_BG"    : dfData.Filter("Sig == -1").Count().GetValue(),
    })
    assert math.isclose(yieldValsTruth[-1]["Yld_Signal"] + yieldValsTruth[-1]["Yld_BG"], nmbEvents)

    # fit signal with true template
    yieldValsSig.append(performFit(
      dataFileName = sigFileName,
      sigFileName  = sigFileName,
      bkgFileName  = None,
      useSig       = True,
      useBkg       = False,
      outputDir    = "outSig",
    ))
    # fit background with true template
    yieldValsBkg.append(performFit(
      dataFileName = bkgFileName,
      sigFileName  = None,
      bkgFileName  = bkgFileName,
      useSig       = False,
      useBkg       = True,
      outputDir    = "outBkg",
    ))
    # fit total distribution with true templates
    yieldVals.append(performFit(
      dataFileName = dataFileName,
      sigFileName  = sigFileName,
      bkgFileName  = bkgFileName,
      useSig       = True,
      useBkg       = True,
      outputDir    = "out",
    ))
    print()

  plotPulls(yieldValsSig, yieldValsTruth, outputDir = "outSig")
  plotPulls(yieldValsBkg, yieldValsTruth, outputDir = "outBkg")
  plotPulls(yieldVals,    yieldValsTruth, outputDir = "out")

  print("Finished successfully")
