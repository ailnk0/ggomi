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

  ErrorStatus PdfToOfficeLib::DoWordConversion(const String &path, const String &password, const String &format)
  {
    std::wstring filePath = path;
    std::wstring outPath = Util::Path::GetDirName(path);
    std::wstring fileFormat = format;

    ErrorStatus status = ErrorStatus::Success;
    try
    {
      if (fileFormat == L"")
        status = ErrorStatus::Fail;
      else if (fileFormat == L"XLSX")
      {
        auto pConverter = std::make_shared<SolidFramework::Converters::PdfToExcelConverter>();
        pConverter->SetOutputType(SolidFramework::Converters::Plumbing::ExcelDocumentType::XlsX);
        pConverter->AddSourceFile(filePath);
        pConverter->SetOutputDirectory(outPath);
        pConverter->SetOverwriteMode(SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
        pConverter->SetPassword(password);
        pConverter->OnProgress = &DoProgress;

        pConverter->Convert();
        status = static_cast<ErrorStatus>(pConverter->GetResults()[0]->GetStatus());
      }
      else if (fileFormat == L"DOCX")
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
      else if (fileFormat == L"PPTX")
      {
        auto pWordConverter = std::make_shared<SolidFramework::Converters::PdfToPowerPointConverter>();

        pWordConverter->AddSourceFile(filePath);
        pWordConverter->SetOutputDirectory(outPath);
        pWordConverter->SetOverwriteMode(SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
        pWordConverter->SetPassword(password);
        pWordConverter->OnProgress = &DoProgress;

        pWordConverter->Convert();
        status = static_cast<ErrorStatus>(pWordConverter->GetResults()[0]->GetStatus());
      }
      else if (fileFormat == L"BMP" || fileFormat == L"JPEG" || fileFormat == L"PNG" || fileFormat == L"TIFF" || fileFormat == L"GIF")
      {
        auto pImageConverter = std::make_shared<SolidFramework::Converters::PdfToImageConverter>();

        SolidFramework::Converters::Plumbing::ImageDocumentType imageType;
        if (fileFormat == L"BMP")
          imageType = SolidFramework::Converters::Plumbing::ImageDocumentType::Bmp;
        else if (fileFormat == L"JPEG")
          imageType = SolidFramework::Converters::Plumbing::ImageDocumentType::Jpeg;
        else if (fileFormat == L"PNG")
          imageType = SolidFramework::Converters::Plumbing::ImageDocumentType::Png;
        else if (fileFormat == L"TIFF")
          imageType = SolidFramework::Converters::Plumbing::ImageDocumentType::Tiff;
        else if (fileFormat == L"GIF")
          imageType = SolidFramework::Converters::Plumbing::ImageDocumentType::Gif;

        pImageConverter->SetOutputType(imageType);
        pImageConverter->AddSourceFile(filePath);
        pImageConverter->SetOutputDirectory(outPath);
        pImageConverter->SetOverwriteMode(SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
        pImageConverter->SetPassword(password);
        pImageConverter->OnProgress = &DoProgress;

        pImageConverter->Convert();
        status = static_cast<ErrorStatus>(pImageConverter->GetResults()[0]->GetStatus());
      }
           
      //// TODO : 취소 기능 분리
      //pWordConverter->Cancel();
      //pWordConverter->ClearSourceFiles();
    }
    catch (const std::exception & /*e*/)
    {
      status = ErrorStatus::Unknown;
    }

    return status;
  }
} // namespace HpdfToOffice
