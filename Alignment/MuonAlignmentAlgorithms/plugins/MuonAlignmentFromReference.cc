// -*- C++ -*-
//
// Package:    MuonAlignmentAlgorithms
// Class:      MuonAlignmentFromReference
// 
/**\class MuonAlignmentFromReference MuonAlignmentFromReference.cc Alignment/MuonAlignmentFromReference/interface/MuonAlignmentFromReference.h

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Jim Pivarski,,,
//         Created:  Sat Jan 24 16:20:28 CST 2009
//

#include "Alignment/CommonAlignmentAlgorithm/interface/AlignmentAlgorithmBase.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "Alignment/CommonAlignment/interface/Alignable.h"
#include "Alignment/CommonAlignment/interface/AlignableDetUnit.h"
#include "Alignment/MuonAlignment/interface/AlignableDTSuperLayer.h"
#include "Alignment/MuonAlignment/interface/AlignableDTChamber.h"
#include "Alignment/MuonAlignment/interface/AlignableDTStation.h"
#include "Alignment/MuonAlignment/interface/AlignableDTWheel.h"
#include "Alignment/MuonAlignment/interface/AlignableCSCChamber.h"
#include "Alignment/MuonAlignment/interface/AlignableCSCRing.h"
#include "Alignment/MuonAlignment/interface/AlignableCSCStation.h"
#include "Alignment/CommonAlignment/interface/AlignableNavigator.h"
#include "Alignment/MuonAlignment/interface/AlignableMuon.h"
#include "Alignment/CommonAlignmentAlgorithm/interface/AlignmentParameterStore.h"
#include "DataFormats/MuonDetId/interface/MuonSubdetId.h"
#include "DataFormats/MuonDetId/interface/DTChamberId.h"
#include "DataFormats/MuonDetId/interface/DTSuperLayerId.h"
#include "Geometry/CommonDetUnit/interface/GlobalTrackingGeometry.h"
#include "Geometry/Records/interface/GlobalTrackingGeometryRecord.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "TrackingTools/PatternTools/interface/Trajectory.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "PhysicsTools/UtilAlgos/interface/TFileService.h"
#include "TFile.h"

#include "Alignment/MuonAlignmentAlgorithms/interface/MuonResidualsFromTrack.h"
#include "Alignment/MuonAlignmentAlgorithms/interface/MuonResidualsPositionFitter.h"
#include "Alignment/MuonAlignmentAlgorithms/interface/MuonResidualsAngleFitter.h"

#include <map>
#include <sstream>
#include <fstream>

class MuonAlignmentFromReference : public AlignmentAlgorithmBase {
public:
  MuonAlignmentFromReference(const edm::ParameterSet& iConfig);
  ~MuonAlignmentFromReference();
  
  void initialize(const edm::EventSetup& iSetup, AlignableTracker* alignableTracker, AlignableMuon* alignableMuon, AlignmentParameterStore* alignmentParameterStore);
  void startNewLoop();
  void run(const edm::EventSetup& iSetup, const ConstTrajTrackPairCollection& trajtracks);
  void terminate();

private:
  std::string m_reference;
  double m_minTrackPt;
  double m_maxTrackPt;
  int m_minTrackerHits;
  double m_maxTrackerRedChi2;
  bool m_allowTIDTEC;
  int m_minDT13Hits;
  int m_minDT2Hits;
  int m_minCSCHits;
  double m_maxResidual;
  std::string m_writeTemporaryFile;
  std::vector<std::string> m_readTemporaryFiles;
  bool m_doAlignment;
  std::string m_residualsModel;
  bool m_DT13fitBfield;
  bool m_DT13fitZpos;
  bool m_DT13fitPhiz;
  bool m_DT13fitSlopeBfield;
  bool m_DT2fitBfield;
  bool m_DT2fitZpos;
  bool m_DT2fitPhiz;
  bool m_DT2fitSlopeBfield;
  bool m_CSCfitBfield;
  bool m_CSCfitZpos;
  bool m_CSCfitPhiz;
  bool m_CSCfitSlopeBfield;
  bool m_DTzFrom13;
  bool m_DTphizFrom13;
  std::string m_reportFileName;
  std::string m_rootDirectory;

  AlignableNavigator *m_alignableNavigator;
  AlignmentParameterStore *m_alignmentParameterStore;
  std::vector<Alignable*> m_alignables;
  std::map<Alignable*,MuonResidualsPositionFitter*> m_rphiFitters;
  std::map<Alignable*,MuonResidualsPositionFitter*> m_zFitters;
  std::map<Alignable*,MuonResidualsAngleFitter*> m_phixFitters;
  std::map<Alignable*,MuonResidualsAngleFitter*> m_phiyFitters;
  std::vector<unsigned int> m_indexOrder;
  std::vector<MuonResidualsFitter*> m_fitterOrder;
};

MuonAlignmentFromReference::MuonAlignmentFromReference(const edm::ParameterSet &iConfig)
  : AlignmentAlgorithmBase(iConfig)
  , m_reference(iConfig.getParameter<std::string>("reference"))
  , m_minTrackPt(iConfig.getParameter<double>("minTrackPt"))
  , m_maxTrackPt(iConfig.getParameter<double>("maxTrackPt"))
  , m_minTrackerHits(iConfig.getParameter<int>("minTrackerHits"))
  , m_maxTrackerRedChi2(iConfig.getParameter<double>("maxTrackerRedChi2"))
  , m_allowTIDTEC(iConfig.getParameter<bool>("allowTIDTEC"))
  , m_minDT13Hits(iConfig.getParameter<int>("minDT13Hits"))
  , m_minDT2Hits(iConfig.getParameter<int>("minDT2Hits"))
  , m_minCSCHits(iConfig.getParameter<int>("minCSCHits"))
  , m_maxResidual(iConfig.getParameter<double>("maxResidual"))
  , m_writeTemporaryFile(iConfig.getParameter<std::string>("writeTemporaryFile"))
  , m_readTemporaryFiles(iConfig.getParameter<std::vector<std::string> >("readTemporaryFiles"))
  , m_doAlignment(iConfig.getParameter<bool>("doAlignment"))
  , m_residualsModel(iConfig.getParameter<std::string>("residualsModel"))
  , m_DT13fitBfield(iConfig.getParameter<bool>("DT13fitBfield"))
  , m_DT13fitZpos(iConfig.getParameter<bool>("DT13fitZpos"))
  , m_DT13fitPhiz(iConfig.getParameter<bool>("DT13fitPhiz"))
  , m_DT13fitSlopeBfield(iConfig.getParameter<bool>("DT13fitSlopeBfield"))
  , m_DT2fitBfield(iConfig.getParameter<bool>("DT2fitBfield"))
  , m_DT2fitZpos(iConfig.getParameter<bool>("DT2fitZpos"))
  , m_DT2fitPhiz(iConfig.getParameter<bool>("DT2fitPhiz"))
  , m_DT2fitSlopeBfield(iConfig.getParameter<bool>("DT2fitSlopeBfield"))
  , m_CSCfitBfield(iConfig.getParameter<bool>("CSCfitBfield"))
  , m_CSCfitZpos(iConfig.getParameter<bool>("CSCfitZpos"))
  , m_CSCfitPhiz(iConfig.getParameter<bool>("CSCfitPhiz"))
  , m_CSCfitSlopeBfield(iConfig.getParameter<bool>("CSCfitSlopeBfield"))
  , m_DTzFrom13(iConfig.getParameter<bool>("DTzFrom13"))
  , m_DTphizFrom13(iConfig.getParameter<bool>("DTphizFrom13"))
  , m_reportFileName(iConfig.getParameter<std::string>("reportFileName"))
  , m_rootDirectory(iConfig.getParameter<std::string>("rootDirectory"))
{
  // alignment requires a TFile to provide plots to check the fit output
  // just filling the residuals lists does not
  // but we don't want to wait until the end of the job to find out that the TFile is missing
  if (m_doAlignment) {
    edm::Service<TFileService> tfileService;
    TFile &tfile = tfileService->file();
  }
}

MuonAlignmentFromReference::~MuonAlignmentFromReference() {
  delete m_alignableNavigator;
}

void MuonAlignmentFromReference::initialize(const edm::EventSetup& iSetup, AlignableTracker* alignableTracker, AlignableMuon* alignableMuon, AlignmentParameterStore* alignmentParameterStore) {
   if (alignableMuon == NULL) {
     throw cms::Exception("MuonAlignmentFromReference") << "doMuon must be set to True" << std::endl;
   }

   m_alignableNavigator = new AlignableNavigator(alignableMuon);
   m_alignmentParameterStore = alignmentParameterStore;
   m_alignables = m_alignmentParameterStore->alignables();

   int residualsModel;
   if (m_residualsModel == std::string("pureGaussian")) residualsModel = MuonResidualsFitter::kPureGaussian;
   else if (m_residualsModel == std::string("powerLawTails")) residualsModel = MuonResidualsFitter::kPowerLawTails;
   else throw cms::Exception("MuonAlignmentFromReference") << "unrecognized residualsModel: \"" << m_residualsModel << "\"" << std::endl;

   // set up the MuonResidualsFitters (which also collect residuals for fitting)
   m_rphiFitters.clear();
   m_zFitters.clear();
   m_phixFitters.clear();
   m_phiyFitters.clear();
   m_indexOrder.clear();
   m_fitterOrder.clear();
   for (std::vector<Alignable*>::const_iterator ali = m_alignables.begin();  ali != m_alignables.end();  ++ali) {
     std::vector<bool> selector = (*ali)->alignmentParameters()->selector();
     bool align_x = selector[0];
     bool align_y = selector[1];
     //     bool align_z = selector[2];
     bool align_phix = selector[3];
     //     bool align_phiy = selector[4];
     bool align_phiz = selector[5];

     if ((*ali)->alignableObjectId() == align::AlignableDTChamber) {
       m_rphiFitters[*ali] = new MuonResidualsPositionFitter(residualsModel, -1);
       if (!m_DT13fitBfield) m_rphiFitters[*ali]->fix(MuonResidualsPositionFitter::kBfield);
       if (!m_DT13fitZpos) m_rphiFitters[*ali]->fix(MuonResidualsPositionFitter::kZpos);
       if (!m_DT13fitPhiz) m_rphiFitters[*ali]->fix(MuonResidualsPositionFitter::kPhiz);
       m_indexOrder.push_back((*ali)->geomDetId().rawId()*4 + 0);
       m_fitterOrder.push_back(m_rphiFitters[*ali]);
       
       m_zFitters[*ali] = new MuonResidualsPositionFitter(residualsModel, -1);
       if (!m_DT2fitBfield) m_zFitters[*ali]->fix(MuonResidualsPositionFitter::kBfield);
       if (!m_DT2fitZpos) m_zFitters[*ali]->fix(MuonResidualsPositionFitter::kZpos);
       if (!m_DT2fitPhiz) m_zFitters[*ali]->fix(MuonResidualsPositionFitter::kPhiz);
       m_indexOrder.push_back((*ali)->geomDetId().rawId()*4 + 1);
       m_fitterOrder.push_back(m_zFitters[*ali]);

       m_phixFitters[*ali] = new MuonResidualsAngleFitter(residualsModel, -1);
       if (!m_DT2fitSlopeBfield) m_phixFitters[*ali]->fix(MuonResidualsAngleFitter::kBfield);
       m_indexOrder.push_back((*ali)->geomDetId().rawId()*4 + 2);
       m_fitterOrder.push_back(m_phixFitters[*ali]);

       m_phiyFitters[*ali] = new MuonResidualsAngleFitter(residualsModel, -1);
       if (!m_DT13fitSlopeBfield) m_phiyFitters[*ali]->fix(MuonResidualsAngleFitter::kBfield);
       m_indexOrder.push_back((*ali)->geomDetId().rawId()*4 + 3);
       m_fitterOrder.push_back(m_phiyFitters[*ali]);
     }

     else if ((*ali)->alignableObjectId() == align::AlignableCSCChamber) {
       if (align_x  &&  (!align_y  ||  !align_phiz)) throw cms::Exception("MuonAlignmentFromReference") << "CSCs are aligned in rphi, not x, so y and phiz must also be alignable" << std::endl;

       m_rphiFitters[*ali] = new MuonResidualsPositionFitter(residualsModel, -1);
       if (!m_CSCfitBfield) m_rphiFitters[*ali]->fix(MuonResidualsPositionFitter::kBfield);
       if (!m_CSCfitZpos) m_rphiFitters[*ali]->fix(MuonResidualsPositionFitter::kZpos);
       if (!m_CSCfitPhiz) m_rphiFitters[*ali]->fix(MuonResidualsPositionFitter::kPhiz);
       m_indexOrder.push_back((*ali)->geomDetId().rawId()*4 + 0);
       m_fitterOrder.push_back(m_rphiFitters[*ali]);

       m_phiyFitters[*ali] = new MuonResidualsAngleFitter(residualsModel, -1);
       if (!m_CSCfitSlopeBfield) m_phiyFitters[*ali]->fix(MuonResidualsAngleFitter::kBfield);
       m_indexOrder.push_back((*ali)->geomDetId().rawId()*4 + 1);
       m_fitterOrder.push_back(m_phiyFitters[*ali]);

       if (align_phix) {
	 throw cms::Exception("MuonAlignmentFromReference") << "CSCChambers can't be aligned in phix" << std::endl;
       }
     }

     else {
       throw cms::Exception("MuonAlignmentFromReference") << "only DTChambers and CSCChambers are alignable" << std::endl;
     }
   } // end loop over chambers chosen for alignment

   // deweight all chambers but the reference
   std::vector<Alignable*> all_DT_chambers = alignableMuon->DTChambers();
   std::vector<Alignable*> all_CSC_chambers = alignableMuon->CSCChambers();
   std::vector<Alignable*> deweight;

   for (std::vector<Alignable*>::const_iterator ali = all_DT_chambers.begin();  ali != all_DT_chambers.end();  ++ali) {
     DTChamberId id((*ali)->geomDetId().rawId());

     if (m_reference == std::string("tracker")) {
       deweight.push_back(*ali);
     }

     else if (m_reference == std::string("station1")) {
       if (id.station() > 1) {
	 deweight.push_back(*ali);
       }
     }

     else if (m_reference == std::string("station2")) {
       if (id.station() > 2) {
	 deweight.push_back(*ali);
       }
     }

     else if (m_reference == std::string("station3")) {
       if (id.station() > 3) {
	 deweight.push_back(*ali);
       }
     }

     else if (m_reference == std::string("wheel0")) {
       if (id.wheel() != 0) {
	 deweight.push_back(*ali);
       }
     }

     else if (m_reference == std::string("wheels0and1")) {
       if (id.wheel() != 0  &&  abs(id.wheel()) != 1) {
	 deweight.push_back(*ali);
       }
     }

     else if (m_reference == std::string("barrel")  ||  m_reference == std::string("me1")  ||  m_reference == std::string("me2")  ||  m_reference == std::string("me3")) {}
   }

   for (std::vector<Alignable*>::const_iterator ali = all_CSC_chambers.begin();  ali != all_CSC_chambers.end();  ++ali) {
     CSCDetId id((*ali)->geomDetId().rawId());

     if (m_reference == std::string("tracker")  ||  m_reference == std::string("wheel0")  ||  m_reference == std::string("wheels0and1")  ||  m_reference == std::string("barrel")) {
       deweight.push_back(*ali);
     }

     else if (m_reference == std::string("me1")) {
       if (id.station() != 1) {
	 deweight.push_back(*ali);
       }
     }

     else if (m_reference == std::string("me2")) {
       if (id.station() != 1  &&  id.station() != 2) {
	 deweight.push_back(*ali);
       }
     }

     else if (m_reference == std::string("me3")) {
       if (id.station() != 1  &&  id.station() != 2  &&  id.station() != 3) {
	 deweight.push_back(*ali);
       }
     }
   }

   alignmentParameterStore->setAlignmentPositionError(deweight, 1000., 0.);
}

void MuonAlignmentFromReference::startNewLoop() {}

void MuonAlignmentFromReference::run(const edm::EventSetup& iSetup, const ConstTrajTrackPairCollection& trajtracks) {
  edm::ESHandle<GlobalTrackingGeometry> globalGeometry;
  iSetup.get<GlobalTrackingGeometryRecord>().get(globalGeometry);

  for (ConstTrajTrackPairCollection::const_iterator trajtrack = trajtracks.begin();  trajtrack != trajtracks.end();  ++trajtrack) {
    const Trajectory* traj = (*trajtrack).first;
    const reco::Track* track = (*trajtrack).second;

    if (m_minTrackPt < track->pt()  &&  track->pt() < m_maxTrackPt) {
      double qoverpt = track->charge() / track->pt();
      MuonResidualsFromTrack muonResidualsFromTrack(globalGeometry, traj, m_alignableNavigator, m_maxResidual);

      if (muonResidualsFromTrack.trackerNumHits() >= m_minTrackerHits  &&  muonResidualsFromTrack.trackerRedChi2() < m_maxTrackerRedChi2  &&  (m_allowTIDTEC  ||  !muonResidualsFromTrack.contains_TIDTEC())) {
	std::vector<unsigned int> indexes = muonResidualsFromTrack.indexes();

	for (std::vector<unsigned int>::const_iterator index = indexes.begin();  index != indexes.end();  ++index) {
	  MuonChamberResidual *chamberResidual = muonResidualsFromTrack.chamberResidual(*index);

	  if (chamberResidual->chamberId().subdetId() == MuonSubdetId::DT  &&  (*index) % 2 == 0) {

	    if (chamberResidual->numHits() >= m_minDT13Hits) {
	      std::map<Alignable*,MuonResidualsPositionFitter*>::const_iterator rphiFitter = m_rphiFitters.find(chamberResidual->chamberAlignable());
	      std::map<Alignable*,MuonResidualsAngleFitter*>::const_iterator phiyFitter = m_phiyFitters.find(chamberResidual->chamberAlignable());

	      if (rphiFitter != m_rphiFitters.end()) {
		double *residdata = new double[MuonResidualsPositionFitter::kNData];
		residdata[MuonResidualsPositionFitter::kResidual] = chamberResidual->residual();
		residdata[MuonResidualsPositionFitter::kQoverPt] = qoverpt;
		residdata[MuonResidualsPositionFitter::kTrackAngle] = chamberResidual->trackdxdz();
		residdata[MuonResidualsPositionFitter::kTrackPosition] = chamberResidual->tracky();
		rphiFitter->second->fill(residdata);
		// the MuonResidualsPositionFitter will delete the array when it is destroyed
	      }
	      
	      if (phiyFitter != m_phiyFitters.end()) {
		double *residdata = new double[MuonResidualsAngleFitter::kNData];
		residdata[MuonResidualsAngleFitter::kResidual] = chamberResidual->resslope();
		residdata[MuonResidualsAngleFitter::kQoverPt] = qoverpt;
		phiyFitter->second->fill(residdata);
		// the MuonResidualsAngleFitter will delete the array when it is destroyed
	      }
	    }
	  } // end if DT13

	  else if (chamberResidual->chamberId().subdetId() == MuonSubdetId::DT  &&  (*index) % 2 == 1) {
	    if (chamberResidual->numHits() >= m_minDT2Hits) {
	      std::map<Alignable*,MuonResidualsPositionFitter*>::const_iterator zFitter = m_zFitters.find(chamberResidual->chamberAlignable());
	      std::map<Alignable*,MuonResidualsAngleFitter*>::const_iterator phixFitter = m_phixFitters.find(chamberResidual->chamberAlignable());

	      if (zFitter != m_zFitters.end()) {
		double *residdata = new double[MuonResidualsPositionFitter::kNData];
		residdata[MuonResidualsPositionFitter::kResidual] = chamberResidual->residual();
		residdata[MuonResidualsPositionFitter::kQoverPt] = qoverpt;
		residdata[MuonResidualsPositionFitter::kTrackAngle] = chamberResidual->trackdydz();
		residdata[MuonResidualsPositionFitter::kTrackPosition] = chamberResidual->trackx();
		zFitter->second->fill(residdata);
		// the MuonResidualsPositionFitter will delete the array when it is destroyed
	      }

	      if (phixFitter != m_phixFitters.end()) {
		double *residdata = new double[MuonResidualsAngleFitter::kNData];
		residdata[MuonResidualsAngleFitter::kResidual] = chamberResidual->resslope();
		residdata[MuonResidualsAngleFitter::kQoverPt] = qoverpt;
		phixFitter->second->fill(residdata);
		// the MuonResidualsAngleFitter will delete the array when it is destroyed
	      }
	    }
	  } // end if DT2

	  else if (chamberResidual->chamberId().subdetId() == MuonSubdetId::CSC) {

	    if (chamberResidual->numHits() >= m_minCSCHits) {
	      std::map<Alignable*,MuonResidualsPositionFitter*>::const_iterator rphiFitter = m_rphiFitters.find(chamberResidual->chamberAlignable());
	      std::map<Alignable*,MuonResidualsAngleFitter*>::const_iterator phiyFitter = m_phiyFitters.find(chamberResidual->chamberAlignable());

	      if (rphiFitter != m_rphiFitters.end()) {
		double *residdata = new double[MuonResidualsPositionFitter::kNData];
		residdata[MuonResidualsPositionFitter::kResidual] = chamberResidual->residual();
		residdata[MuonResidualsPositionFitter::kQoverPt] = qoverpt;
		residdata[MuonResidualsPositionFitter::kTrackAngle] = chamberResidual->trackdxdz();
		residdata[MuonResidualsPositionFitter::kTrackPosition] = chamberResidual->tracky();
		rphiFitter->second->fill(residdata);
		// the MuonResidualsPositionFitter will delete the array when it is destroyed
	      }

	      if (phiyFitter != m_phiyFitters.end()) {
		double *residdata = new double[MuonResidualsAngleFitter::kNData];
		residdata[MuonResidualsAngleFitter::kResidual] = chamberResidual->resslope();
		residdata[MuonResidualsAngleFitter::kQoverPt] = qoverpt;
		phiyFitter->second->fill(residdata);
		// the MuonResidualsAngleFitter will delete the array when it is destroyed
	      }
	    }
	  } // end if CSC

	} // end loop over chamberIds
      } // end if refit is okay
    } // end if track pT is within range
  } // end loop over tracks
}

void MuonAlignmentFromReference::terminate() {
  // collect temporary files
  if (m_readTemporaryFiles.size() != 0) {
    for (std::vector<std::string>::const_iterator fileName = m_readTemporaryFiles.begin();  fileName != m_readTemporaryFiles.end();  ++fileName) {
      FILE *file;
      int size;
      file = fopen(fileName->c_str(), "r");
      fread(&size, sizeof(int), 1, file);
      if (int(m_indexOrder.size()) != size) throw cms::Exception("MuonAlignmentFromReference") << "file \"" << *fileName << "\" has " << size << " fitters, but this job has " << m_indexOrder.size() << " fitters (probably corresponds to the wrong alignment job)" << std::endl;
      
      std::vector<unsigned int>::const_iterator index = m_indexOrder.begin();
      std::vector<MuonResidualsFitter*>::const_iterator fitter = m_fitterOrder.begin();
      for (int i = 0;  i < size;  ++i, ++index, ++fitter) {
	unsigned int index_toread;
	fread(&index_toread, sizeof(unsigned int), 1, file);
	if (*index != index_toread) throw cms::Exception("MuonAlignmentFromReference") << "file \"" << *fileName << "\" has index " << index_toread << " at position " << i << ", but this job is expecting " << *index << " (probably corresponds to the wrong alignment job)" << std::endl;
	(*fitter)->read(file, i);
      }

      fclose(file);
    }
  }

  // fit and align (time-consuming, so the user can turn it off if in
  // a residuals-gathering job)
  if (m_doAlignment) {
    edm::Service<TFileService> tfileService;
    TFileDirectory rootDirectory(m_rootDirectory == std::string("") ? *tfileService : tfileService->mkdir(m_rootDirectory));

    std::ofstream report;
    bool writeReport = (m_reportFileName != std::string(""));
    if (writeReport) {
      report.open(m_reportFileName.c_str());
      report << "reports = []" << std::endl;
      report << "class Report:" << std::endl
	     << "    def __init__(self, chamberId, postal_address, name):" << std::endl
	     << "        self.chamberId, self.postal_address, self.name = chamberId, postal_address, name" << std::endl
	     << "" << std::endl
	     << "    def rphiFit(self, position, zpos, phiz, bfield, sigma, gamma, redchi2):" << std::endl
	     << "        self.rphiFit_status = \"PASS\"" << std::endl
	     << "        self.rphiFit_position = position" << std::endl
	     << "        self.rphiFit_zpos = zpos" << std::endl
	     << "        self.rphiFit_phiz = phiz" << std::endl
	     << "        self.rphiFit_bfield = bfield" << std::endl
	     << "        self.rphiFit_sigma = sigma" << std::endl
	     << "        self.rphiFit_gamma = gamma" << std::endl
	     << "        self.rphiFit_redchi2 = redchi2" << std::endl
	     << "" << std::endl
	     << "    def zFit(self, position, zpos, phiz, bfield, sigma, gamma, redchi2):" << std::endl
	     << "        self.zFit_status = \"PASS\"" << std::endl
	     << "        self.zFit_position = position" << std::endl
	     << "        self.zFit_zpos = zpos" << std::endl
	     << "        self.zFit_phiz = phiz" << std::endl
	     << "        self.zFit_bfield = bfield" << std::endl
	     << "        self.zFit_sigma = sigma" << std::endl
	     << "        self.zFit_gamma = gamma" << std::endl
	     << "        self.zFit_redchi2 = redchi2" << std::endl
	     << "" << std::endl
	     << "    def phixFit(self, angle, bfield, sigma, gamma, redchi2):" << std::endl
	     << "        self.phixFit_status = \"PASS\"" << std::endl
	     << "        self.phixFit_angle = angle" << std::endl
	     << "        self.phixFit_bfield = bfield" << std::endl
	     << "        self.phixFit_sigma = sigma" << std::endl
	     << "        self.phixFit_gamma = gamma" << std::endl
	     << "        self.phixFit_redchi2 = redchi2" << std::endl
	     << "" << std::endl
	     << "    def phiyFit(self, angle, bfield, sigma, gamma, redchi2):" << std::endl
	     << "        self.phiyFit_status = \"PASS\"" << std::endl
	     << "        self.phiyFit_angle = angle" << std::endl
	     << "        self.phiyFit_bfield = bfield" << std::endl
	     << "        self.phiyFit_sigma = sigma" << std::endl
	     << "        self.phiyFit_gamma = gamma" << std::endl
	     << "        self.phiyFit_redchi2 = redchi2" << std::endl
	     << "" << std::endl
	     << "    def parameters(self, deltax, deltay, deltaz, deltaphix, deltaphiy, deltaphiz):" << std::endl
	     << "        self.deltax, self.deltay, self.deltaz, self.deltaphix, self.deltaphiy, self.deltaphiz = \\" << std::endl
	     << "                     deltax, deltay, deltaz, deltaphix, deltaphiy, deltaphiz" << std::endl
	     << "" << std::endl
	     << "    def errors(self, err2x, err2y, err2z):" << std::endl
	     << "        self.err2x, self.err2y, self.err2z = err2x, err2y, err2z" << std::endl;
    }

    for (std::vector<Alignable*>::const_iterator ali = m_alignables.begin();  ali != m_alignables.end();  ++ali) {
      std::vector<bool> selector = (*ali)->alignmentParameters()->selector();
      bool align_x = selector[0];
      bool align_y = selector[1];
      bool align_z = selector[2];
      bool align_phix = selector[3];
      bool align_phiy = selector[4];
      bool align_phiz = selector[5];
      int numParams = ((align_x ? 1 : 0) + (align_y ? 1 : 0) + (align_z ? 1 : 0) + (align_phix ? 1 : 0) + (align_phiy ? 1 : 0) + (align_phiz ? 1 : 0));

      // map from 0-5 to the index of params, above
      std::vector<int> paramIndex;
      int paramIndex_counter = -1;
      if (align_x) paramIndex_counter++;
      paramIndex.push_back(paramIndex_counter);
      if (align_y) paramIndex_counter++;
      paramIndex.push_back(paramIndex_counter);
      if (align_z) paramIndex_counter++;
      paramIndex.push_back(paramIndex_counter);
      if (align_phix) paramIndex_counter++;
      paramIndex.push_back(paramIndex_counter);
      if (align_phiy) paramIndex_counter++;
      paramIndex.push_back(paramIndex_counter);
      if (align_phiz) paramIndex_counter++;
      paramIndex.push_back(paramIndex_counter);

      // uncertainties will be infinite except for the aligned chambers in the aligned directions
      AlgebraicVector params(numParams);
      AlgebraicSymMatrix cov(numParams);
      for (int i = 0;  i < numParams;  i++) {
	for (int j = 0;  j < numParams;  j++) {
	  cov[i][j] = 0.;
	}
	params[i] = 0.;
      }
      // but the translational ones only, because that's all that's stored
      cov[paramIndex[0]][paramIndex[0]] = 1000.;
      cov[paramIndex[1]][paramIndex[1]] = 1000.;
      cov[paramIndex[2]][paramIndex[2]] = 1000.;

      std::map<Alignable*,MuonResidualsPositionFitter*>::const_iterator rphiFitter = m_rphiFitters.find(*ali);
      std::map<Alignable*,MuonResidualsPositionFitter*>::const_iterator zFitter = m_zFitters.find(*ali);
      std::map<Alignable*,MuonResidualsAngleFitter*>::const_iterator phixFitter = m_phixFitters.find(*ali);
      std::map<Alignable*,MuonResidualsAngleFitter*>::const_iterator phiyFitter = m_phiyFitters.find(*ali);
      
      DetId id = (*ali)->geomDetId();
      std::stringstream name;
      if (id.subdetId() == MuonSubdetId::DT) {
	DTChamberId chamberId(id.rawId());
	name << "MBwh";
	if (chamberId.wheel() == -2) name << "A";
	else if (chamberId.wheel() == -1) name << "B";
	else if (chamberId.wheel() ==  0) name << "C";
	else if (chamberId.wheel() == +1) name << "D";
	else if (chamberId.wheel() == +2) name << "E";
	name << "st" << chamberId.station() << "sec" << chamberId.sector();

	if (writeReport) {
	  report << "reports.append(Report(" << id.rawId() << ", (\"DT\", " << chamberId.wheel() << ", " << chamberId.station() << ", " << chamberId.sector() << "), \"" << name.str() << "\"))" << std::endl;
	}
      }
      else if (id.subdetId() == MuonSubdetId::CSC) {
	CSCDetId chamberId(id.rawId());
	name << "ME" << (chamberId.endcap() == 1 ? "p" : "m") << abs(chamberId.station()) << chamberId.ring() << "_" << chamberId.chamber();

	if (writeReport) {
	  report << "reports.append(Report(" << id.rawId() << ", (\"CSC\", " << (chamberId.endcap() == 1 ? 1 : -1)*abs(chamberId.station()) << ", " << chamberId.ring() << ", " << chamberId.chamber() << "), \"" << name.str() << "\"))" << std::endl;
	}
      }

      if (rphiFitter != m_rphiFitters.end()) {
	// the fit is verbose in std::cout anyway
	std::cout << "=============================================================================================" << std::endl;
	std::cout << "Fitting " << name.str() << " rphi" << std::endl;
	std::cout << "=============================================================================================" << std::endl;
	if (rphiFitter->second->fit()) {
	  std::stringstream name2;
	  name2 << name.str() << "_rphiFit";
	  rphiFitter->second->plot(name2.str(), &rootDirectory);
	  double redchi2 = rphiFitter->second->redchi2(name2.str(), &rootDirectory);

	  double position_value = rphiFitter->second->value(MuonResidualsPositionFitter::kPosition);
	  double position_error = rphiFitter->second->error(MuonResidualsPositionFitter::kPosition);
	  double position_uperr = rphiFitter->second->uperr(MuonResidualsPositionFitter::kPosition);
	  double position_downerr = rphiFitter->second->downerr(MuonResidualsPositionFitter::kPosition);

	  double zpos_value = rphiFitter->second->value(MuonResidualsPositionFitter::kZpos);
	  double zpos_error = rphiFitter->second->error(MuonResidualsPositionFitter::kZpos);
	  double zpos_uperr = rphiFitter->second->uperr(MuonResidualsPositionFitter::kZpos);
	  double zpos_downerr = rphiFitter->second->downerr(MuonResidualsPositionFitter::kZpos);

	  double phiz_value = rphiFitter->second->value(MuonResidualsPositionFitter::kPhiz);
	  double phiz_error = rphiFitter->second->error(MuonResidualsPositionFitter::kPhiz);
	  double phiz_uperr = rphiFitter->second->uperr(MuonResidualsPositionFitter::kPhiz);
	  double phiz_downerr = rphiFitter->second->downerr(MuonResidualsPositionFitter::kPhiz);

	  double bfield_value = rphiFitter->second->value(MuonResidualsPositionFitter::kBfield);
	  double bfield_error = rphiFitter->second->error(MuonResidualsPositionFitter::kBfield);
	  double bfield_uperr = rphiFitter->second->uperr(MuonResidualsPositionFitter::kBfield);
	  double bfield_downerr = rphiFitter->second->downerr(MuonResidualsPositionFitter::kBfield);

	  double sigma_value = rphiFitter->second->value(MuonResidualsPositionFitter::kSigma);
	  double sigma_error = rphiFitter->second->error(MuonResidualsPositionFitter::kSigma);
	  double sigma_uperr = rphiFitter->second->uperr(MuonResidualsPositionFitter::kSigma);
	  double sigma_downerr = rphiFitter->second->downerr(MuonResidualsPositionFitter::kSigma);

	  double gamma_value, gamma_error, gamma_uperr, gamma_downerr;
	  gamma_value = gamma_error = gamma_uperr = gamma_downerr = 0.;
	  if (rphiFitter->second->residualsModel() != MuonResidualsFitter::kPureGaussian) {
	    gamma_value = rphiFitter->second->value(MuonResidualsPositionFitter::kGamma);
	    gamma_error = rphiFitter->second->error(MuonResidualsPositionFitter::kGamma);
	    gamma_uperr = rphiFitter->second->uperr(MuonResidualsPositionFitter::kGamma);
	    gamma_downerr = rphiFitter->second->downerr(MuonResidualsPositionFitter::kGamma);
	  }

	  if (id.subdetId() == MuonSubdetId::DT) {
	    if (align_x) {
	      params[paramIndex[0]] = position_value;
	      cov[paramIndex[0]][paramIndex[0]] = 0.;   // local x-z is the global x-y plane; with an x alignment, this is now a good parameter
	      cov[paramIndex[2]][paramIndex[2]] = 0.;
	    }

	    if (align_z  &&  m_DTzFrom13) {
	      params[paramIndex[2]] = zpos_value;
	    }
	  
	    if (align_phiz  &&  m_DTphizFrom13) {
	      params[paramIndex[5]] = phiz_value;
	    }
	  } // end if DT

	  else {
	    if (align_x) {
	      GlobalPoint cscCenter = (*ali)->globalPosition();
	      double R = sqrt(cscCenter.x()*cscCenter.x() + cscCenter.y()*cscCenter.y());
	      double globalphi_correction = position_value / R;
	      
	      double localx_correction = R * sin(globalphi_correction);
	      double localy_correction = R * (cos(globalphi_correction) - 1.);
	      double phiz_correction = -globalphi_correction;

	      params[paramIndex[0]] = localx_correction;
	      params[paramIndex[1]] = localy_correction;
	      params[paramIndex[5]] = phiz_correction;

	      cov[paramIndex[0]][paramIndex[0]] = 0.;  // local x-y plane is the global x-y plane; with an rphi alignment, this is a now a good parameter
	      cov[paramIndex[1]][paramIndex[1]] = 0.;
	    }

	    if (align_z) {
	      params[paramIndex[2]] = zpos_value;
	    }

	    if (align_phiz) {
	      params[paramIndex[5]] += phiz_value;  // += not =    ...accumulated on top of whatever was needed for curvilinear rphi correction
	    }
	  } // end if CSC

	  if (writeReport) {
	    report << "reports[-1].rphiFit((" << position_value << ", " << position_error << ", " << position_uperr << ", " << position_downerr << "), \\" << std::endl
		   << "                    (" << zpos_value << ", " << zpos_error << ", " << zpos_uperr << ", " << zpos_downerr << "), \\" << std::endl
		   << "                    (" << phiz_value << ", " << phiz_error << ", " << phiz_uperr << ", " << phiz_downerr << "), \\" << std::endl
		   << "                    (" << bfield_value << ", " << bfield_error << ", " << bfield_uperr << ", " << bfield_downerr << "), \\" << std::endl
		   << "                    (" << sigma_value << ", " << sigma_error << ", " << sigma_uperr << ", " << sigma_downerr << "), \\" << std::endl;
	    if (rphiFitter->second->residualsModel() != MuonResidualsFitter::kPureGaussian) {
	      report << "                    (" << gamma_value << ", " << gamma_error << ", " << gamma_uperr << ", " << gamma_downerr << "), \\" << std::endl;
	    }
	    else {
	      report << "                    None, \\" << std::endl;
	    }
	    report << "                    " << redchi2 << ")" << std::endl;
	  } // end if writeReport
	}
	else if (writeReport) {
	  report << "reports[-1].rphiFit_status = \"FAIL\"" << std::endl;
	}
      }

      if (zFitter != m_zFitters.end()) {
	// the fit is verbose in std::cout anyway
	std::cout << "=============================================================================================" << std::endl;
	std::cout << "Fitting " << name.str() << " z" << std::endl;
	std::cout << "=============================================================================================" << std::endl;
	if (zFitter->second->fit()) {
	  std::stringstream name2;
	  name2 << name.str() << "_zFit";
	  zFitter->second->plot(name2.str(), &rootDirectory);
	  double redchi2 = zFitter->second->redchi2(name2.str(), &rootDirectory);

	  double position_value = zFitter->second->value(MuonResidualsPositionFitter::kPosition);
	  double position_error = zFitter->second->error(MuonResidualsPositionFitter::kPosition);
	  double position_uperr = zFitter->second->uperr(MuonResidualsPositionFitter::kPosition);
	  double position_downerr = zFitter->second->downerr(MuonResidualsPositionFitter::kPosition);

	  double zpos_value = zFitter->second->value(MuonResidualsPositionFitter::kZpos);
	  double zpos_error = zFitter->second->error(MuonResidualsPositionFitter::kZpos);
	  double zpos_uperr = zFitter->second->uperr(MuonResidualsPositionFitter::kZpos);
	  double zpos_downerr = zFitter->second->downerr(MuonResidualsPositionFitter::kZpos);

	  double phiz_value = zFitter->second->value(MuonResidualsPositionFitter::kPhiz);
	  double phiz_error = zFitter->second->error(MuonResidualsPositionFitter::kPhiz);
	  double phiz_uperr = zFitter->second->uperr(MuonResidualsPositionFitter::kPhiz);
	  double phiz_downerr = zFitter->second->downerr(MuonResidualsPositionFitter::kPhiz);

	  double bfield_value = zFitter->second->value(MuonResidualsPositionFitter::kBfield);
	  double bfield_error = zFitter->second->error(MuonResidualsPositionFitter::kBfield);
	  double bfield_uperr = zFitter->second->uperr(MuonResidualsPositionFitter::kBfield);
	  double bfield_downerr = zFitter->second->downerr(MuonResidualsPositionFitter::kBfield);

	  double sigma_value = zFitter->second->value(MuonResidualsPositionFitter::kSigma);
	  double sigma_error = zFitter->second->error(MuonResidualsPositionFitter::kSigma);
	  double sigma_uperr = zFitter->second->uperr(MuonResidualsPositionFitter::kSigma);
	  double sigma_downerr = zFitter->second->downerr(MuonResidualsPositionFitter::kSigma);

	  double gamma_value, gamma_error, gamma_uperr, gamma_downerr;
	  gamma_value = gamma_error = gamma_uperr = gamma_downerr = 0.;
	  if (zFitter->second->residualsModel() != MuonResidualsFitter::kPureGaussian) {
	    gamma_value = zFitter->second->value(MuonResidualsPositionFitter::kGamma);
	    gamma_error = zFitter->second->error(MuonResidualsPositionFitter::kGamma);
	    gamma_uperr = zFitter->second->uperr(MuonResidualsPositionFitter::kGamma);
	    gamma_downerr = zFitter->second->downerr(MuonResidualsPositionFitter::kGamma);
	  }

	  if (id.subdetId() == MuonSubdetId::DT) {
	    if (align_x) {
	      params[paramIndex[1]] = position_value;
	      cov[paramIndex[1]][paramIndex[1]] = 0.;   // local y is the global z direction: this is now a good parameter
	    }

	    if (align_z  &&  !m_DTzFrom13) {
	      params[paramIndex[2]] = zpos_value;
	    }
	  
	    if (align_phiz  &&  !m_DTphizFrom13) {
	      params[paramIndex[5]] = phiz_value;
	    }
	  } // end if DT

	  if (writeReport) {
	    report << "reports[-1].zFit((" << position_value << ", " << position_error << ", " << position_uperr << ", " << position_downerr << "), \\" << std::endl
		   << "                 (" << zpos_value << ", " << zpos_error << ", " << zpos_uperr << ", " << zpos_downerr << "), \\" << std::endl
		   << "                 (" << phiz_value << ", " << phiz_error << ", " << phiz_uperr << ", " << phiz_downerr << "), \\" << std::endl
		   << "                 (" << bfield_value << ", " << bfield_error << ", " << bfield_uperr << ", " << bfield_downerr << "), \\" << std::endl
		   << "                 (" << sigma_value << ", " << sigma_error << ", " << sigma_uperr << ", " << sigma_downerr << "), \\" << std::endl;
	    if (zFitter->second->residualsModel() != MuonResidualsFitter::kPureGaussian) {
	      report << "                 (" << gamma_value << ", " << gamma_error << ", " << gamma_uperr << ", " << gamma_downerr << "), \\" << std::endl;
	    }
	    else {
	      report << "                 None, \\" << std::endl;
	    }
	    report << "                 " << redchi2 << ")" << std::endl;
	  } // end if writeReport
	}
	else if (writeReport) {
	  report << "reports[-1].zFit_status = \"FAIL\"" << std::endl;
	}
      }

      if (phixFitter != m_phixFitters.end()) {
	// the fit is verbose in std::cout anyway
	std::cout << "=============================================================================================" << std::endl;
	std::cout << "Fitting " << name << " phix" << std::endl;
	std::cout << "=============================================================================================" << std::endl;
	if (phixFitter->second->fit()) {
	  std::stringstream name2;
	  name2 << name.str() << "_phixFit";
	  phixFitter->second->plot(name2.str(), &rootDirectory);
	  double redchi2 = phixFitter->second->redchi2(name2.str(), &rootDirectory);

	  double angle_value = phixFitter->second->value(MuonResidualsAngleFitter::kAngle);
	  double angle_error = phixFitter->second->error(MuonResidualsAngleFitter::kAngle);
	  double angle_uperr = phixFitter->second->uperr(MuonResidualsAngleFitter::kAngle);
	  double angle_downerr = phixFitter->second->downerr(MuonResidualsAngleFitter::kAngle);

	  double bfield_value = phixFitter->second->value(MuonResidualsAngleFitter::kBfield);
	  double bfield_error = phixFitter->second->error(MuonResidualsAngleFitter::kBfield);
	  double bfield_uperr = phixFitter->second->uperr(MuonResidualsAngleFitter::kBfield);
	  double bfield_downerr = phixFitter->second->downerr(MuonResidualsAngleFitter::kBfield);

	  double sigma_value = phixFitter->second->value(MuonResidualsAngleFitter::kSigma);
	  double sigma_error = phixFitter->second->error(MuonResidualsAngleFitter::kSigma);
	  double sigma_uperr = phixFitter->second->uperr(MuonResidualsAngleFitter::kSigma);
	  double sigma_downerr = phixFitter->second->downerr(MuonResidualsAngleFitter::kSigma);

	  double gamma_value, gamma_error, gamma_uperr, gamma_downerr;
	  gamma_value = gamma_error = gamma_uperr = gamma_downerr = 0.;
	  if (phixFitter->second->residualsModel() != MuonResidualsFitter::kPureGaussian) {
	    gamma_value = phixFitter->second->value(MuonResidualsAngleFitter::kGamma);
	    gamma_error = phixFitter->second->error(MuonResidualsAngleFitter::kGamma);
	    gamma_uperr = phixFitter->second->uperr(MuonResidualsAngleFitter::kGamma);
	    gamma_downerr = phixFitter->second->downerr(MuonResidualsAngleFitter::kGamma);
	  }

	  if (id.subdetId() == MuonSubdetId::DT) {
	    if (align_phix) {
	      params[paramIndex[3]] = angle_value;
	    }
	  } // end if DT

	  if (writeReport) {
	    report << "reports[-1].phixFit((" << angle_value << ", " << angle_error << ", " << angle_uperr << ", " << angle_downerr << "), \\" << std::endl
		   << "                    (" << bfield_value << ", " << bfield_error << ", " << bfield_uperr << ", " << bfield_downerr << "), \\" << std::endl
		   << "                    (" << sigma_value << ", " << sigma_error << ", " << sigma_uperr << ", " << sigma_downerr << "), \\" << std::endl;
	    if (phixFitter->second->residualsModel() != MuonResidualsFitter::kPureGaussian) {
	      report << "                    (" << gamma_value << ", " << gamma_error << ", " << gamma_uperr << ", " << gamma_downerr << "), \\" << std::endl;
	    }
	    else {
	      report << "                    None, \\" << std::endl;
	    }
	    report << "                    " << redchi2 << ")" << std::endl;
	  } // end if writeReport
	}
	else if (writeReport) {
	  report << "reports[-1].phizFit_status = \"FAIL\"" << std::endl;
	}
      }

      if (phiyFitter != m_phiyFitters.end()) {
	// the fit is verbose in std::cout anyway
	std::cout << "=============================================================================================" << std::endl;
	std::cout << "Fitting " << name << " phiy" << std::endl;
	std::cout << "=============================================================================================" << std::endl;
	if (phiyFitter->second->fit()) {
	  std::stringstream name2;
	  name2 << name.str() << "_phiyFit";
	  phiyFitter->second->plot(name2.str(), &rootDirectory);
	  double redchi2 = phiyFitter->second->redchi2(name2.str(), &rootDirectory);

	  double angle_value = phiyFitter->second->value(MuonResidualsAngleFitter::kAngle);
	  double angle_error = phiyFitter->second->error(MuonResidualsAngleFitter::kAngle);
	  double angle_uperr = phiyFitter->second->uperr(MuonResidualsAngleFitter::kAngle);
	  double angle_downerr = phiyFitter->second->downerr(MuonResidualsAngleFitter::kAngle);

	  double bfield_value = phiyFitter->second->value(MuonResidualsAngleFitter::kBfield);
	  double bfield_error = phiyFitter->second->error(MuonResidualsAngleFitter::kBfield);
	  double bfield_uperr = phiyFitter->second->uperr(MuonResidualsAngleFitter::kBfield);
	  double bfield_downerr = phiyFitter->second->downerr(MuonResidualsAngleFitter::kBfield);

	  double sigma_value = phiyFitter->second->value(MuonResidualsAngleFitter::kSigma);
	  double sigma_error = phiyFitter->second->error(MuonResidualsAngleFitter::kSigma);
	  double sigma_uperr = phiyFitter->second->uperr(MuonResidualsAngleFitter::kSigma);
	  double sigma_downerr = phiyFitter->second->downerr(MuonResidualsAngleFitter::kSigma);

	  double gamma_value, gamma_error, gamma_uperr, gamma_downerr;
	  gamma_value = gamma_error = gamma_uperr = gamma_downerr = 0.;
	  if (phiyFitter->second->residualsModel() != MuonResidualsFitter::kPureGaussian) {
	    gamma_value = phiyFitter->second->value(MuonResidualsAngleFitter::kGamma);
	    gamma_error = phiyFitter->second->error(MuonResidualsAngleFitter::kGamma);
	    gamma_uperr = phiyFitter->second->uperr(MuonResidualsAngleFitter::kGamma);
	    gamma_downerr = phiyFitter->second->downerr(MuonResidualsAngleFitter::kGamma);
	  }

	  if (id.subdetId() == MuonSubdetId::DT) {
	    if (align_phiy) {
	      params[paramIndex[4]] = angle_value;
	    }
	  } // end if DT

	  else {
	    if (align_phiy) {
	      params[paramIndex[4]] = angle_value;
	    }
	  } // end if CSC

	  if (writeReport) {
	    report << "reports[-1].phiyFit((" << angle_value << ", " << angle_error << ", " << angle_uperr << ", " << angle_downerr << "), \\" << std::endl
		   << "                    (" << bfield_value << ", " << bfield_error << ", " << bfield_uperr << ", " << bfield_downerr << "), \\" << std::endl
		   << "                    (" << sigma_value << ", " << sigma_error << ", " << sigma_uperr << ", " << sigma_downerr << "), \\" << std::endl;
	    if (phiyFitter->second->residualsModel() != MuonResidualsFitter::kPureGaussian) {
	      report << "                    (" << gamma_value << ", " << gamma_error << ", " << gamma_uperr << ", " << gamma_downerr << "), \\" << std::endl;
	    }
	    else {
	      report << "                    None, \\" << std::endl;
	    }
	    report << "                    " << redchi2 << ")" << std::endl;
	  } // end if writeReport
	}
	else if (writeReport) {
	  report << "reports[-1].phizFit_status = \"FAIL\"" << std::endl;
	}
      }

      if (writeReport) {
	report << "reports[-1].parameters(";
	if (align_x) report << params[paramIndex[0]] << ", ";
	else report << "None, ";
	if (align_y) report << params[paramIndex[1]] << ", ";
	else report << "None, ";
	if (align_z) report << params[paramIndex[2]] << ", ";
	else report << "None, ";
	if (align_phix) report << params[paramIndex[3]] << ", ";
	else report << "None, ";
	if (align_phiy) report << params[paramIndex[4]] << ", ";
	else report << "None, ";
	if (align_phiz) report << params[paramIndex[5]] << ")" << std::endl;
	else report << "None)" << std::endl;

	report << "reports[-1].errors(";
	if (align_x) report << cov[paramIndex[0]][paramIndex[0]] << ", ";
	else report << "None, ";
	if (align_y) report << cov[paramIndex[1]][paramIndex[1]] << ", ";
	else report << "None, ";
	if (align_z) report << cov[paramIndex[2]][paramIndex[2]] << ")" << std::endl;
	else report << "None)" << std::endl;

	report << std::endl;
      }

      AlignmentParameters *parnew = (*ali)->alignmentParameters()->cloneFromSelected(params, cov);
      (*ali)->setAlignmentParameters(parnew);
      m_alignmentParameterStore->applyParameters(*ali);
      (*ali)->alignmentParameters()->setValid(true);
    } // end loop over alignables

    if (writeReport) report.close();
  }

  // write out the pseudontuples for a later job to collect
  if (m_writeTemporaryFile != std::string("")) {
    FILE *file;
    file = fopen(m_writeTemporaryFile.c_str(), "w");
    int size = m_indexOrder.size();
    fwrite(&size, sizeof(int), 1, file);

    std::vector<unsigned int>::const_iterator index = m_indexOrder.begin();
    std::vector<MuonResidualsFitter*>::const_iterator fitter = m_fitterOrder.begin();
    for (int i = 0;  i < size;  ++i, ++index, ++fitter) {
      unsigned int index_towrite = *index;
      fwrite(&index_towrite, sizeof(unsigned int), 1, file);
      (*fitter)->write(file, i);
    }

    fclose(file);
  }
}

#include "Alignment/CommonAlignmentAlgorithm/interface/AlignmentAlgorithmPluginFactory.h"
DEFINE_EDM_PLUGIN(AlignmentAlgorithmPluginFactory, MuonAlignmentFromReference, "MuonAlignmentFromReference");
