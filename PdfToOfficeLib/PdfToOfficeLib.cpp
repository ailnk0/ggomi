// PdfToOfficeLib.cpp : Defines the functions for the static library.
//

#include "pch.h"

#include "PdfToOfficeLib.h"

#include <array>
#include <fstream>
#include <iostream>

using namespace SolidFramework::Platform;
using namespace SolidFramework::Converters::Plumbing;

namespace HpdfToOffice
{

IProgressSite* PdfToOfficeLib::m_ProgressSite = nullptr;

ErrorStatus PdfToOfficeLib::InitializeSolidFramework()
{
    SolidFramework::SupportPlatformIndependentPaths(true);

    try
    {
        String sdkDir = Util::Path::GetSdkDir();
        if (sdkDir.empty())
        {
            return ErrorStatus::InternalError;
        }
        if (!SolidFramework::Initialize(sdkDir))
        {
            return ErrorStatus::InternalError;
        }
    }
    catch (...)
    {
        return ErrorStatus::InternalError;
    }

    try
    {
        SolidFramework::License::Import(Util::Path::GetSdkLicPath());
    }
    catch (SolidFramework::InvalidLicenseException)
    {
        return ErrorStatus::InvalidLicense;
    }

    return ErrorStatus::Success;
}

void PdfToOfficeLib::DoProgress(SolidFramework::ProgressEventArgsPtr pProgressEventArgs)
{
    int progress = pProgressEventArgs->GetProgress();
    int maxProgress = pProgressEventArgs->GetMaxProgress();
    SolidFramework::Interop::SolidErrorCodes statusCode = pProgressEventArgs->GetStatusCode();
    std::wstring statusDesc = pProgressEventArgs->GetStatusDescription();

    double totalProgress = 0;
    if (statusDesc.find(L"PDFOCRInfoText") == 0)
    {
        totalProgress = progress / 4;
    }
    else if (statusDesc.find(L"PDFLoadingInfoText") == 0)
    {
        totalProgress = 25 + (progress / 4);
    }
    else if (statusDesc.find(L"PDFConvertingInfoText") == 0)
    {
        totalProgress = 50 + (progress / 4);
    }
    else if (statusDesc.find(L"WritingFileMessage") == 0)
    {
        totalProgress = 75 + (progress / 4);
    }

    std::wstring strDebug;
    strDebug.append(statusDesc.c_str());
    strDebug.append(L" : ");
    strDebug.append(std::to_wstring(progress));
    strDebug.append(L"%");

    std::wcout << strDebug << std::endl;

    if (m_ProgressSite) {
        m_ProgressSite->SetPercent(totalProgress);
    }
}

void PdfToOfficeLib::SetSite(IProgressSite* progressSite)
{
    m_ProgressSite = progressSite;
}

ErrorStatus PdfToOfficeLib::DoWordConversion(const String &path, const String &password)
{
    std::wstring filePath = path;
    std::wstring outPath = Util::Path::GetDirName(path);

    ErrorStatus status = ErrorStatus::Success;
    try
    {
        auto pWordConverter = std::make_shared<SolidFramework::Converters::PdfToWordConverter>();

        pWordConverter->SetReconstructionMode(SolidFramework::Converters::Plumbing::ReconstructionMode::Flowing);
        pWordConverter->SetOutputType(SolidFramework::Converters::Plumbing::WordDocumentType::DocX);
        pWordConverter->AddSourceFile(filePath);
        pWordConverter->SetOutputDirectory(outPath);
        pWordConverter->SetOverwriteMode(SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
        pWordConverter->SetPassword(password);
        pWordConverter->OnProgress = &DoProgress;

        pWordConverter->Convert();

        status = static_cast<ErrorStatus>(pWordConverter->GetResults()[0]->GetStatus());
    }
    catch (const std::exception & /*e*/)
    {
        status = ErrorStatus::Unknown;
    }

    return status;
}
} // namespace HpdfToOffice
