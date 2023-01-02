#pragma once

#include "IProgressSite.h"

#include <vcclr.h>

namespace PdfToOfficeAppModule {

class ProgressSiteProxy : public HpdfToOffice::IProgressSite {
 private:
  gcroot<IProgressSiteCli ^> m_Site;

 public:
  ProgressSiteProxy(gcroot<IProgressSiteCli ^> site);
  virtual ~ProgressSiteProxy();

 public:
  virtual void SetPercent(int percent);
};

}  // namespace PdfToOfficeAppModule
