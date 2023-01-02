#pragma once

#include "DefineProxy.h"
#include "IPdfToOffice.h"
#include "IPdfToOfficeProxy.h"
#include "IProgressSite.h"
#include "IProgressSiteCli.h"

namespace PdfToOfficeAppModule {

public
ref class PdfToOfficeProxy : public IPdfToOfficeProxy {
 public:
  PdfToOfficeProxy();
  ~PdfToOfficeProxy();

 public:
  virtual RES_CODE InitializeSolidFramework();
  virtual RES_CODE Convert(System::String ^ path, System::String ^ password);
  virtual void Cancel();
  virtual void SetOverwrite(bool overwrite);
  virtual void SetSaveToUserDir(bool isSaveToUserDir);
  virtual void SetUserDir(System::String ^ path);
  virtual void SetProgressSiteCli(IProgressSiteCli ^ progressSiteCli);

 protected:
  HpdfToOffice::IPdfToOffice* lib = nullptr;
  HpdfToOffice::IProgressSite* m_ProgressSite = nullptr;
};

}  // namespace PdfToOfficeAppModule
