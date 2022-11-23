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

ErrorStatus PdfToOfficeLib::InitializeSolidFramework()
{
    SolidFramework::SupportPlatformIndependentPaths(true);

    if (!SolidFramework::Initialize(Util::Path::GetSdkPath()))
    {
        return ErrorStatus::InternalError;
    }

    try
    {
        SolidFramework::License::Import(Util::Path::GetSdkLicPath());
    }
    catch (SolidFramework::InvalidLicenseException /*&ex*/)
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

    std::wstring strDebug;
    strDebug.append(statusDesc.c_str());
    strDebug.append(L" : ");
    strDebug.append(std::to_wstring(progress));
    strDebug.append(L"%");

    std::wcout << strDebug << std::endl;
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
