#pragma once

#include "DefineProxy.h"
#include "IProgressSiteCli.h"
#include "IProgressSite.h"

#include <vcclr.h>

namespace HpdfToOffice {
class PdfToOffice;
}

namespace PdfToOfficeAppModule {
public
ref class PdfToOfficeProxy {
 public:
  PdfToOfficeProxy();
  ~PdfToOfficeProxy();

 public:
  virtual void SetProgressSiteCli(IProgressSiteCli ^ progressSiteCli);
  virtual RES_CODE InitializeSolidFramework();
  virtual RES_CODE Convert(System::String ^ path, System::String ^ password);
  virtual void Cancel();
  virtual void SetIsSaveToUserDir(bool allow);
  virtual void SetUserDir(System::String ^ path);
  virtual void SetOverwrite(bool allow);

 protected:
  HpdfToOffice::PdfToOffice* lib = nullptr;
  HpdfToOffice::IProgressSite* m_ProgressSite = nullptr;
};
}  // namespace PdfToOfficeAppModule
