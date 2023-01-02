#include "pch.h"

#include "PdfToImage.h"

using namespace SolidFramework::Platform;
using namespace SolidFramework::Converters::Plumbing;

namespace HpdfToOffice {
RES_CODE PdfToImage::Convert(const String& path, const String& password) {
  String filePath = path;

  String outPath;
  if (m_IsSaveToUserDir && !m_UserDir.empty()) {
    outPath = m_UserDir;
  } else {
    outPath = Util::Path::GetDirName(path);
  }

  RES_CODE status = RES_CODE::Success;
  try {
    auto pConverter =
        std::make_shared<SolidFramework::Converters::PdfToImageConverter>();
    m_Converter = pConverter;

    SolidFramework::Converters::Plumbing::ImageDocumentType imageType =
        SolidFramework::Converters::Plumbing::ImageDocumentType::Png;
    if (m_ImgType == IMG_TYPE::BMP)
      imageType = SolidFramework::Converters::Plumbing::ImageDocumentType::Bmp;
    else if (m_ImgType == IMG_TYPE::JPEG)
      imageType = SolidFramework::Converters::Plumbing::ImageDocumentType::Jpeg;
    else if (m_ImgType == IMG_TYPE::PNG)
      imageType = SolidFramework::Converters::Plumbing::ImageDocumentType::Png;
    else if (m_ImgType == IMG_TYPE::TIFF)
      imageType = SolidFramework::Converters::Plumbing::ImageDocumentType::Tiff;
    else if (m_ImgType == IMG_TYPE::GIF)
      imageType = SolidFramework::Converters::Plumbing::ImageDocumentType::Gif;

    pConverter->SetConversionType(ImageConversionType::ExtractPages);
    pConverter->SetOutputType(imageType);
    pConverter->AddSourceFile(filePath);
    pConverter->SetOutputDirectory(outPath);
    pConverter->SetOverwriteMode(
        SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
    pConverter->SetPassword(password);
    pConverter->OnProgress = &DoProgress;

    if (m_IsOverwrite) {
      pConverter->SetOverwriteMode(
          SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
    }

    pConverter->Convert();
    status = static_cast<RES_CODE>(pConverter->GetResults()[0]->GetStatus());
  } catch (const std::exception& /*e*/) {
    status = RES_CODE::Unknown;
  }
  return status;
}

void PdfToImage::SetImgType(IMG_TYPE imgType) {
  m_ImgType = imgType;
}
}  // namespace HpdfToOffice
