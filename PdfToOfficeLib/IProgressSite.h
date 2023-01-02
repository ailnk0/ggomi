#pragma once

namespace HpdfToOffice {

class IProgressSite {
 public:
  virtual void SetPercent(int percent) = 0;
};

}  // namespace HpdfToOffice
