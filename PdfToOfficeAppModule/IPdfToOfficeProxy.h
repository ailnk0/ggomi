#pragma once

#include "DefineProxy.h"
#include "IProgressSiteCli.h"

namespace PdfToOfficeAppModule {

public
interface class IPdfToOfficeProxy {
 public:
  virtual RES_CODE Init() = 0;
  virtual RES_CODE Convert(System::String ^ path,
                           System::String ^ password) = 0;
  virtual void Cancel() = 0;
  virtual void SetOverwrite(bool overwrite) = 0;
  virtual void SetSaveToUserDir(bool isSaveToUserDir) = 0;
  virtual void SetUserDir(System::String ^ path) = 0;
  virtual System::String ^ GetOutPath() = 0;
  virtual void SetProgressSiteCli(IProgressSiteCli ^ progressSiteCli) = 0;
};

}  // namespace PdfToOfficeAppModule
