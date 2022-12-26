#pragma once

#include "IProgressSite.h"
#include "SolidFramework.h"
#include "Define.h"

namespace HpdfToOffice
{
class PdfToOfficeLib
{
private:
    static IProgressSite* m_ProgressSite;

  public:
    PdfToOfficeLib() = default;

  public:
    virtual ErrorStatus InitializeSolidFramework();
    virtual ErrorStatus DoConversion(const String &path, const String &password, const String &fileFormat);
  public:
    static void DoProgress(SolidFramework::ProgressEventArgsPtr pProgressEventArgs);
    static void SetSite(IProgressSite* progressSite);
};
} // namespace HpdfToOffice
