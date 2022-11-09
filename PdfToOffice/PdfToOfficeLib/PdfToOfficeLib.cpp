// PdfToOfficeLib.cpp : Defines the functions for the static library.
//

#include "pch.h"

#include "PdfToOfficeLib.h"

#include <array>
#include <fstream>
#include <iostream>

using namespace SolidFramework::Platform;

bool PdfToOfficeLib::InitializeSolidFramework(const std::wstring &frameworkPath)
{
    // Open License File
    std::ifstream licenseFile("../../../SolidFrameworkLicense/license.txt", std::ios::binary);
    if (!licenseFile.is_open())
    {
        std::wcout << L"License file is not exist." << std::endl;
        return false;
    }

    // Read License
    std::array<char, 256> buffer;
    std::vector<std::wstring> license;
    license.resize(4);
    for (int i = 0; i < 4; i++)
    {
        licenseFile.getline(&buffer[0], 255);
        std::string temp = &buffer[0];
        license[i].assign(temp.begin(), temp.end());
    }

    // Initialize SolidFramework
    if (!SolidFramework::Initialize(frameworkPath))
    {
        std::wcout << L"Couldn't initialize SolidFramework from path " << frameworkPath << std::endl;
        return false;
    }

    // Import the license
    try
    {
        SolidFramework::License::Import(license[0], license[1], license[2], license[3]);
    }
    catch (SolidFramework::InvalidLicenseException &ex)
    {
        std::wcout << L"Couldn't import license due to exception: " << ex.what() << std::endl;
        return false;
    }
    return true;
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

bool PdfToOfficeLib::DoWordConversion(const std::wstring &fullPath, const std::wstring &password)
{
    std::wstring filePath = fullPath;
    std::wstring outPath = fullPath;

    size_t found = outPath.find_last_of(L"/\\");
    outPath = outPath.substr(0, found);

    bool bContinue = true;
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

        SolidFramework::Converters::Plumbing::ConversionStatus status = pWordConverter->GetResults()[0]->GetStatus();

        if (status != SolidFramework::Converters::Plumbing::ConversionStatus::Success)
        {
            std::wstring strError = L"ConversionStatus" + std::to_wstring((int)status);
            MessageBox(NULL, strError.c_str(), L"Exception", MB_ICONERROR | MB_TOPMOST);
            bContinue = false;
        }
    }
    catch (const std::exception &e)
    {
        std::string strE(e.what());
        std::wstring strError(strE.begin(), strE.end());
        MessageBox(NULL, strError.c_str(), L"Exception", MB_ICONERROR | MB_TOPMOST);
        bContinue = false;
    }

    return bContinue;
}

int PdfToOfficeLib::RunSamples(const std::wstring &path)
{
    // Allow SolidFramework to treat '\' and '/' as the platform-specific
    // directory separator
    SolidFramework::SupportPlatformIndependentPaths(true);

    // Set the path to the SolidFramework DLLs (varies depending on the platform)
    const std::wstring frameworkPath = L"HncPdfSdk\\";

    // Initialize SolidFramework and import the license
    if (!InitializeSolidFramework(frameworkPath))
    {
        std::wcout << L"SolidFramework initialization failed." << std::endl;
        return -1;
    }

    // Now you can start to use Solid Framework
    std::wcout << L"SolidFramework is initialized and ready to be used." << std::endl;

    DoWordConversion(path, L"");

    std::wcout << L"RunSamples is done." << std::endl;

    return 0;
}

int PdfToOfficeLib::RunSamples()
{
    return RunSamples(L"../sample/HOffice2022_Brochure_KR.pdf");
}
