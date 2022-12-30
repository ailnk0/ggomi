#pragma once

#include "DefineProxy.h"
#include "IProgressSite.h"

#include <vcclr.h>

namespace HpdfToOffice {
class PdfToOfficeLib;
}

namespace PdfToOfficeAppModule {
public
interface class IProgressSiteCli {
  void SetPercent(int percent);
};

public
ref class PdfToOfficeProxy {
 public:
  PdfToOfficeProxy();
  ~PdfToOfficeProxy();

 public:
  virtual void SetProgressSiteCli(IProgressSiteCli ^ progressSiteCli);
  virtual RES_CODE InitializeSolidFramework();
  virtual RES_CODE DoConversion(System::String ^ path,
                                System::String ^ password,
                                FILE_TYPE fileType,
                                IMG_TYPE imageType,
                                bool overwrite);

 private:
  HpdfToOffice::PdfToOfficeLib* lib = nullptr;
  HpdfToOffice::IProgressSite* m_ProgressSite = nullptr;
};

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
