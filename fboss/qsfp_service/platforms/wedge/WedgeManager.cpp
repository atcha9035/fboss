#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

#include <folly/gen/Base.h>

#include "fboss/lib/usb/UsbError.h"
#include "fboss/qsfp_service/sff/QsfpModule.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeQsfp.h"

namespace facebook { namespace fboss {
WedgeManager::WedgeManager(){
}

void WedgeManager::initTransceiverMap(){
  // If we can't get access to the USB devices, don't bother to
  // create the QSFP objects;  this is likely to be a permanent
  // error.
  try {
    wedgeI2CBusLock_ = folly::make_unique<WedgeI2CBusLock>(getI2CBus());
  } catch (const LibusbError& ex) {
    LOG(ERROR) << "failed to initialize USB to I2C interface";
    return;
  }

  // Wedge port 0 is the CPU port, so the first port associated with
  // a QSFP+ is port 1.  We start the transceiver IDs with 0, though.
  for (int idx = 0; idx < getNumQsfpModules(); idx++) {
    std::unique_ptr<WedgeQsfp> qsfpImpl =
      folly::make_unique<WedgeQsfp>(idx, wedgeI2CBusLock_.get());
    std::unique_ptr<QsfpModule> qsfp =
      folly::make_unique<QsfpModule>(std::move(qsfpImpl));
    transceivers_.emplace(TransceiverID(idx), move(qsfp));
    LOG(INFO) << "making QSFP for " << idx;
  }
}

void WedgeManager::getTransceiversInfo(std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) |
      folly::gen::appendTo(*ids);
  }

  for (const auto& i : *ids) {
    TransceiverInfo transInfo;
    transceivers_[TransceiverID(i)]->getTransceiverInfo(transInfo);
    info[i] = transInfo;
  }
}

std::unique_ptr<BaseWedgeI2CBus> WedgeManager::getI2CBus() {
  return folly::make_unique<WedgeI2CBus>();
}
}} // facebook::fboss