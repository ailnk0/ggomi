#include "pch.h"

#include "PdfToPptx.h"

namespace HpdfToOffice {

RES_CODE PdfToPptx::Convert(const String& path, const String& password) {
  String outPath = path;
  if (m_IsSaveToUserDir && !m_UserDir.empty()) {
    outPath = m_UserDir + L"\\" + Util::Path::GetFileName(outPath);
  }
  outPath = Util::Path::ChangeExt(outPath, L".pptx");
  outPath = Util::Path::GetAvailFileName(outPath);

  RES_CODE status = RES_CODE::Success;
  try {
    auto pConverter = std::make_shared<
        SolidFramework::Converters::PdfToPowerPointConverter>();
    m_Converter = pConverter;

    pConverter->OnProgress = &DoProgress;
    pConverter->SetPassword(password);

    status = static_cast<RES_CODE>(pConverter->Convert(path, outPath, true));

  } catch (const std::exception& /*e*/) {
    status = RES_CODE::Unknown;
  }
  return status;
}

}  // namespace HpdfToOffice
