﻿// PdfToOffice.cpp : Defines the functions for the static library.
//

#include "pch.h"

#include "PdfToOffice.h"

#include <array>
#include <fstream>
#include <iostream>

namespace HpdfToOffice {

IProgressSite* PdfToOffice::s_ProgressSite = nullptr;

RES_CODE PdfToOffice::Init() {
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

void PdfToOffice::Cancel() {
  if (!m_Converter) {
    return;
  }
  m_Converter->Cancel();
  m_Converter->ClearSourceFiles();
}

bool PdfToOffice::GetOverwrite() {
  return m_IsOverwrite;
}

void PdfToOffice::SetOverwrite(bool overwrite) {
  m_IsOverwrite = overwrite;
}

void PdfToOffice::DoProgress(
    SolidFramework::ProgressEventArgsPtr pProgressEventArgs) {
  int progress = pProgressEventArgs->GetProgress();
  int maxProgress = pProgressEventArgs->GetMaxProgress();
  String statusDesc = pProgressEventArgs->GetStatusDescription();

  int totalProgress = 0;
  if (statusDesc == L"PDFOCRInfoText") {
    totalProgress = (25 * progress / maxProgress);
  } else if (statusDesc == L"PDFLoadingInfoText") {
    totalProgress = 25 + (25 * progress / maxProgress);
  } else if (statusDesc == L"PDFConvertingInfoText") {
    totalProgress = 50 + (25 * progress / maxProgress);
  } else if (statusDesc == L"WritingFileMessage") {
    totalProgress = 75 + (25 * progress / maxProgress);
  } else if (statusDesc == L"PageInfo") {
    totalProgress = 100 * progress / maxProgress;
  }

  if (s_ProgressSite) {
    s_ProgressSite->SetPercent(totalProgress);
  }
}

void PdfToOffice::SetSite(IProgressSite* progressSite) {
  s_ProgressSite = progressSite;
}

}  // namespace HpdfToOffice
