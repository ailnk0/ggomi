#include "pch.h"

#include "Util.h"

#include <fstream>

namespace HpdfToOffice {
namespace Util {
const String Path::SDK_PATH = L"HncPdfSdk/SolidFrameworkNative.dll";
const String Path::SDK_LIC_PATH = L"../../SolidFrameworkLicense/license.xml";

bool Path::Exist(const String& path) {
  bool exist = false;
  std::ifstream buf(path);
  if (buf.is_open()) {
    exist = true;
    buf.close();
  }
  return exist;
}

String Path::GetCurExePath() {
  wchar_t buf[MAX_PATH];
  GetModuleFileName(nullptr, buf, MAX_PATH);
  return buf;
}

String Path::GetCurExeDir() {
  String buf = GetCurExePath();
  buf = GetDirName(buf);
  return buf;
}

String Path::GetDirName(const String& path) {
  String buf;
  auto pos = path.find_last_of(L"\\/");
  if (pos != String::npos) {
    buf = path.substr(0, pos);
  }
  return buf;
}

String Path::GetFileName(const String& path) {
  String buf;
  auto pos = path.find_last_of(L"\\/");
  if (pos != String::npos) {
    buf = path.substr(pos + 1);
  }
  return buf;
}

String Path::GetSdkPath() {
  String path = SDK_PATH;
  if (Exist(path)) {
    return path;
  }

  path = GetCurExeDir();
  path.append(SDK_PATH);
  if (Exist(path)) {
    return path;
  }

  return String();
}

String Path::GetSdkDir() {
  return GetDirName(GetSdkPath());
}

String Path::GetSdkLicPath() {
  return SDK_LIC_PATH;
}
}  // namespace Util
}  // namespace HpdfToOffice
