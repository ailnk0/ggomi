#pragma once

#include "DefineProxy.h"

namespace HpdfToOffice
{
class PdfToOfficeLib;
}

namespace PdfToOfficeAppModule
{

public
ref class PdfToOfficeAppModule
{
  public:
    PdfToOfficeAppModule();
    ~PdfToOfficeAppModule();

  public:
    virtual ErrorStatus InitializeSolidFramework();
    virtual ErrorStatus DoWordConversion(System::String ^ path, System::String ^ password);

  private:
    HpdfToOffice::PdfToOfficeLib *lib = nullptr;
};

} // namespace PdfToOfficeAppModule
