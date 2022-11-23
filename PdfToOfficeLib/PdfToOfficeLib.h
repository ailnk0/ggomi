#pragma once

#include "SolidFramework.h"
#include "Define.h"

namespace HpdfToOffice
{

class PdfToOfficeLib
{
  public:
    PdfToOfficeLib() = default;

  public:
    virtual ErrorStatus InitializeSolidFramework();
    virtual ErrorStatus DoWordConversion(const String &path, const String &password);

  public:
    static void DoProgress(SolidFramework::ProgressEventArgsPtr pProgressEventArgs);
};

} // namespace HpdfToOffice
