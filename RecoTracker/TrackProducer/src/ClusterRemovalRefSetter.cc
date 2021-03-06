#include "RecoTracker/TrackProducer/interface/ClusterRemovalRefSetter.h"
#include "DataFormats/SiPixelDetId/interface/PixelSubdetector.h"

ClusterRemovalRefSetter::ClusterRemovalRefSetter(const edm::Event &iEvent, const edm::InputTag& tag) {
    edm::Handle<reco::ClusterRemovalInfo> hCRI;
    iEvent.getByLabel(tag, hCRI);
    cri_ = &*hCRI; 

    //std::cout << "Rekeying PIXEL ProdID " << cri_->pixelNewRefProd().id() << " => " << cri_->pixelRefProd().id() << std::endl;
    //std::cout << "Rekeying STRIP ProdID " << cri_->stripNewRefProd().id() << " => " << cri_->stripRefProd().id() << std::endl;
}

void ClusterRemovalRefSetter::reKey(TrackingRecHit *hit) const {
    if (!hit->isValid()) return;
    DetId detid = hit->geographicalId(); 
    uint32_t subdet = detid.subdetId();
    if ((subdet == PixelSubdetector::PixelBarrel) || (subdet == PixelSubdetector::PixelEndcap)) {
        if (!cri_->hasPixel()) return;
        reKeyPixel(reinterpret_cast<SiPixelRecHit *>(hit)->omniCluster());
    } else {
        if (!cri_->hasStrip()) return;
        const std::type_info &type = typeid(*hit);
        if (type == typeid(SiStripRecHit2D)) {
            reKeyStrip(reinterpret_cast<SiStripRecHit2D *>(hit)->omniCluster());
	} else if (type == typeid(SiStripRecHit1D)) {
            reKeyStrip(reinterpret_cast<SiStripRecHit1D *>(hit)->omniCluster());
        } else if (type == typeid(SiStripMatchedRecHit2D)) {
            SiStripMatchedRecHit2D *mhit = reinterpret_cast<SiStripMatchedRecHit2D *>(hit);
            reKeyStrip(mhit->monoClusterRef());
            reKeyStrip(mhit->stereoClusterRef());
        } else if (type == typeid(ProjectedSiStripRecHit2D)) {
            ProjectedSiStripRecHit2D *phit = reinterpret_cast<ProjectedSiStripRecHit2D *>(hit);
            reKeyStrip(phit->originalHit().omniCluster());
        } else throw cms::Exception("Unknown RecHit Type") << "RecHit of type " << type.name() << " not supported. (use c++filt to demangle the name)";
    }
}


void ClusterRemovalRefSetter::reKeyPixel(OmniClusterRef& newRef) const {
  // "newRef" as it refs to the "new"(=cleaned) collection, instead of the old one
  using reco::ClusterRemovalInfo;
  const ClusterRemovalInfo::Indices &indices = cri_->pixelIndices();
  
  if (newRef.id()  != cri_->pixelNewRefProd().id()) {
    throw cms::Exception("Inconsistent Data") << "ClusterRemovalRefSetter: " << 
      "Existing pixel cluster refers to product ID " << newRef.id() << 
      " while the ClusterRemovalInfo expects as *new* cluster collection the ID " << cri_->pixelNewRefProd().id() << "\n";
  }
  size_t newIndex = newRef.key();
  assert(newIndex < indices.size());
  size_t oldIndex = indices[newIndex];
  ClusterPixelRef oldRef(cri_->pixelRefProd(), oldIndex);
  newRef = OmniClusterRef(oldRef);
}


void ClusterRemovalRefSetter::reKeyStrip(OmniClusterRef& newRef) const {
  // "newRef" as it refs to the "new"(=cleaned) collection, instead of the old one
  using reco::ClusterRemovalInfo;
  const ClusterRemovalInfo::Indices &indices = cri_->stripIndices();
  
  if (newRef.id() != cri_->stripNewRefProd().id()) {   // this is a cfg error in the tracking configuration, much more likely
    throw cms::Exception("Inconsistent Data") << "ClusterRemovalRefSetter: " << 
      "Existing strip cluster refers to product ID " << newRef.id() << 
      " while the ClusterRemovalInfo expects as *new* cluster collection the ID " << cri_->stripNewRefProd().id() << "\n";
  }
  
  size_t newIndex = newRef.key();
  assert(newIndex < indices.size());
  size_t oldIndex = indices[newIndex];
  ClusterStripRef oldRef(cri_->stripRefProd(), oldIndex);
  newRef = OmniClusterRef(oldRef);
}


