#pragma once

#include <vcclr.h>
#include "DefineProxy.h"
#include "IProgressSite.h"

namespace HpdfToOffice
{
class PdfToOfficeLib;
}

namespace PdfToOfficeAppModule
{
public interface class IProgressSiteCli
{
    void SetPercent(int percent);
};

public
ref class PdfToOfficeProxy
{
  public:
    PdfToOfficeProxy(IProgressSiteCli^ progressSiteCli);
    ~PdfToOfficeProxy();

  public:
    virtual ErrorStatus InitializeSolidFramework();
    virtual ErrorStatus DoWordConversion(System::String ^ path, System::String ^ password);
    System::String ^ DoProgressValue;
  private:
    HpdfToOffice::PdfToOfficeLib *lib = nullptr;

    HpdfToOffice::IProgressSite* m_ProgressSite;
};

class ProgressSiteProxy : public HpdfToOffice::IProgressSite
{
private:
    gcroot<IProgressSiteCli ^> m_Site;

public:
    ProgressSiteProxy(gcroot<IProgressSiteCli ^> site);
    virtual ~ProgressSiteProxy();

public:
    virtual void SetPercent(int percent);
};
} // namespace PdfToOfficeAppModule
