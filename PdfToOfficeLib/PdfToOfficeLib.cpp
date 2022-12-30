// PdfToOfficeLib.cpp : Defines the functions for the static library.
//

#include "pch.h"

#include "PdfToOfficeLib.h"

#include <array>
#include <fstream>
#include <iostream>

using namespace SolidFramework::Platform;
using namespace SolidFramework::Converters::Plumbing;

namespace HpdfToOffice {

IProgressSite* PdfToOfficeLib::m_ProgressSite = nullptr;

RES_CODE PdfToOfficeLib::InitializeSolidFramework() {
  SolidFramework::SupportPlatformIndependentPaths(true);

  try {
    String sdkDir = Util::Path::GetSdkDir();
    if (sdkDir.empty()) {
      return RES_CODE::InternalError;
    }
    if (!SolidFramework::Initialize(sdkDir)) {
      return RES_CODE::InternalError;
    }
  } catch (...) {
    return RES_CODE::InternalError;
  }

  try {
    SolidFramework::License::Import(Util::Path::GetSdkLicPath());
  } catch (SolidFramework::InvalidLicenseException) {
    return RES_CODE::InvalidLicense;
  }

  return RES_CODE::Success;
}

void PdfToOfficeLib::DoProgress(
    SolidFramework::ProgressEventArgsPtr pProgressEventArgs) {
  int progress = pProgressEventArgs->GetProgress();
  String statusDesc = pProgressEventArgs->GetStatusDescription();

  int totalProgress = 0;
  if (statusDesc.find(L"PDFOCRInfoText") == 0) {
    totalProgress = progress / 4;
  } else if (statusDesc.find(L"PDFLoadingInfoText") == 0) {
    totalProgress = 25 + (progress / 4);
  } else if (statusDesc.find(L"PDFConvertingInfoText") == 0) {
    totalProgress = 50 + (progress / 4);
  } else if (statusDesc.find(L"WritingFileMessage") == 0) {
    totalProgress = 75 + (progress / 4);
  }

  if (m_ProgressSite) {
    m_ProgressSite->SetPercent(totalProgress);
  }
}

void PdfToOfficeLib::SetSite(IProgressSite* progressSite) {
  m_ProgressSite = progressSite;
}

RES_CODE PdfToOfficeLib::DoConversion(const String& path,
                                      const String& password,
                                      FILE_TYPE fileFormat,
                                      IMG_TYPE imageFormat,
                                      bool overwrite) {
  String filePath = path;
  String outPath = Util::Path::GetDirName(path);

  RES_CODE status = RES_CODE::Success;
  try {
    if (fileFormat == FILE_TYPE::XLSX) {
      auto pConverter =
          std::make_shared<SolidFramework::Converters::PdfToExcelConverter>();
      pConverter->SetOutputType(
          SolidFramework::Converters::Plumbing::ExcelDocumentType::XlsX);
      pConverter->AddSourceFile(filePath);
      pConverter->SetOutputDirectory(outPath);
      pConverter->SetPassword(password);
      pConverter->OnProgress = &DoProgress;

      if (overwrite) {
        pConverter->SetOverwriteMode(
            SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
      }

      pConverter->Convert();
      status = static_cast<RES_CODE>(pConverter->GetResults()[0]->GetStatus());

    } else if (fileFormat == FILE_TYPE::DOCX) {
      auto pWordConverter =
          std::make_shared<SolidFramework::Converters::PdfToWordConverter>();

      pWordConverter->SetReconstructionMode(
          SolidFramework::Converters::Plumbing::ReconstructionMode::Flowing);
      pWordConverter->SetOutputType(
          SolidFramework::Converters::Plumbing::WordDocumentType::DocX);
      pWordConverter->AddSourceFile(filePath);
      pWordConverter->SetOutputDirectory(outPath);
      pWordConverter->SetOverwriteMode(
          SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
      pWordConverter->SetPassword(password);
      pWordConverter->OnProgress = &DoProgress;

      pWordConverter->Convert();
      status =
          static_cast<RES_CODE>(pWordConverter->GetResults()[0]->GetStatus());

    } else if (fileFormat == FILE_TYPE::PPTX) {
      auto pWordConverter = std::make_shared<
          SolidFramework::Converters::PdfToPowerPointConverter>();

      pWordConverter->AddSourceFile(filePath);
      pWordConverter->SetOutputDirectory(outPath);
      pWordConverter->SetOverwriteMode(
          SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
      pWordConverter->SetPassword(password);
      pWordConverter->OnProgress = &DoProgress;

      pWordConverter->Convert();
      status =
          static_cast<RES_CODE>(pWordConverter->GetResults()[0]->GetStatus());

    } else if (fileFormat == FILE_TYPE::IMAGE) {
      auto pImageConverter =
          std::make_shared<SolidFramework::Converters::PdfToImageConverter>();

      SolidFramework::Converters::Plumbing::ImageDocumentType imageType =
          SolidFramework::Converters::Plumbing::ImageDocumentType::Png;
      if (imageFormat == IMG_TYPE::BMP)
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Bmp;
      else if (imageFormat == IMG_TYPE::JPEG)
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Jpeg;
      else if (imageFormat == IMG_TYPE::PNG)
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Png;
      else if (imageFormat == IMG_TYPE::TIFF)
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Tiff;
      else if (imageFormat == IMG_TYPE::GIF)
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Gif;

      pImageConverter->SetConversionType(ImageConversionType::ExtractPages);
      pImageConverter->SetOutputType(imageType);
      pImageConverter->AddSourceFile(filePath);
      pImageConverter->SetOutputDirectory(outPath);
      pImageConverter->SetOverwriteMode(
          SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
      pImageConverter->SetPassword(password);
      pImageConverter->OnProgress = &DoProgress;

      pImageConverter->Convert();
      status =
          static_cast<RES_CODE>(pImageConverter->GetResults()[0]->GetStatus());

    } else {
    }

    //// TODO : 취소 기능 분리
    // pWordConverter->Cancel();
    // pWordConverter->ClearSourceFiles();

  } catch (const std::exception& /*e*/) {
    status = RES_CODE::Unknown;
  }

  return status;
}
}  // namespace HpdfToOffice
