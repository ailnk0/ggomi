#include "pch.h"

#include "PdfToImage.h"

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

    SolidFramework::Converters::Plumbing::ImageDocumentType imageType;
    switch (m_ImgType) {
      case IMG_TYPE::BMP:
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Bmp;
        break;
      case IMG_TYPE::JPEG:
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Jpeg;
        break;
      case IMG_TYPE::PNG:
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Png;
        break;
      case IMG_TYPE::TIFF:
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Tiff;
        break;
      case IMG_TYPE::GIF:
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Gif;
        break;
      default:
        imageType =
            SolidFramework::Converters::Plumbing::ImageDocumentType::Png;
        break;
    }

    pConverter->SetConversionType(SolidFramework::Converters::Plumbing::
                                      ImageConversionType::ExtractPages);
    pConverter->SetOutputType(imageType);
    pConverter->AddSourceFile(filePath);
    pConverter->SetOutputDirectory(outPath);
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
