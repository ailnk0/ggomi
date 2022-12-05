#include "pch.h"

#include "PdfToOfficeProxy.h"

#include "IProgressSite.h"
#include "PdfToOfficeLib.h"

#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

namespace PdfToOfficeAppModule
{

PdfToOfficeProxy::PdfToOfficeProxy(IProgressSiteCli^ progressSiteCli)
{
    lib = new HpdfToOffice::PdfToOfficeLib();

    ProgressSiteProxy* progressSiteProxy = new ProgressSiteProxy(progressSiteCli); // C#의 ProgressSiteCli 클래스 객체를 파라미터로 전달
    m_ProgressSite = static_cast<HpdfToOffice::IProgressSite*>(progressSiteProxy);

    HpdfToOffice::PdfToOfficeLib::SetSite(m_ProgressSite);
}

PdfToOfficeProxy::~PdfToOfficeProxy()
{
    if (lib)
    {
        delete lib;
        lib = nullptr;
	}

    if (m_ProgressSite != nullptr) {
        delete m_ProgressSite;
        m_ProgressSite = nullptr;
    }
}

ErrorStatus PdfToOfficeProxy::InitializeSolidFramework()
{
    try
    {
        return static_cast<ErrorStatus>(lib->InitializeSolidFramework());
    }
    catch (...)
    {
        return static_cast<ErrorStatus>(HpdfToOffice::ErrorStatus::Unknown);
    }
}

ErrorStatus PdfToOfficeProxy::DoWordConversion(System::String ^ path, System::String ^ password)
{
    try
    {
        return static_cast<ErrorStatus>(
            lib->DoWordConversion(msclr::interop::marshal_as<HpdfToOffice::String>(path),
                                                      msclr::interop::marshal_as<HpdfToOffice::String>(password)));
    }
    catch (...)
    {
        return static_cast<ErrorStatus>(HpdfToOffice::ErrorStatus::Unknown);
    }
}

ProgressSiteProxy::ProgressSiteProxy(gcroot<IProgressSiteCli^> site) {
    m_Site = site;
}

ProgressSiteProxy::~ProgressSiteProxy() {
    if (static_cast<IProgressSiteCli^>(m_Site) != nullptr) {
        delete m_Site;
        m_Site = nullptr;
    }
}

void ProgressSiteProxy::SetPercent(int percent) {
    m_Site->SetPercent(percent); // C#에 구현되어 있는 SetPercent메서드가 호출됨
}
} // namespace PdfToOfficeAppModule
