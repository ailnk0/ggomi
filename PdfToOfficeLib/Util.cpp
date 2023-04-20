#include "pch.h"

#include "Util.h"

#include <fstream>
#include <sstream>

namespace HpdfToOffice {
namespace Util {

const String Path::SDK_PATH = L"./../../solidframework/Win32/SolidFrameworkNative.dll";
const String Path::SDK_LIC_PATH = L"./../../solidframework/license.xml";

bool Path::Exist(const String& path) {
  bool exist = false;
  std::ifstream buf(path);
  if (buf.is_open()) {
    exist = true;
    buf.close();
  }
  return exist;
}

String Path::ChangeExt(const String& path, const String& ext) {
  String buf = GetBeforeExt(path);
  buf.append(ext);
  return buf;
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

String Path::GetExt(const String& path) {
  String buf;
  auto pos = path.find_last_of(L".");
  if (pos != String::npos) {
    buf = path.substr(pos);
  }
  return buf;
}

String Path::GetBeforeExt(const String& path) {
  String buf;
  auto pos = path.find_last_of(L".");
  if (pos != String::npos) {
    buf = path.substr(0, pos);
  }
  return buf;
}

String Path::GetFileNameWithoutExt(const String& path) {
  String buf = GetFileName(path);
  auto pos = buf.find_last_of(L".");
  if (pos != String::npos) {
    buf = buf.substr(0, pos);
  }
  return buf;
}

String Path::GetAvailFileName(const String& path) {
  String beforeExt = GetBeforeExt(path);
  String ext = GetExt(path);

  int count = 0;
  String temp = path;
  while (Exist(temp)) {
    StringStream buf;
    buf << beforeExt;
    buf << L" (";
    buf << ++count;
    buf << L")";
    buf << ext;
    temp = buf.str();
  };
  return temp;
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
  String path = SDK_LIC_PATH;
  if (Exist(path)) {
    return path;
  }

  path = GetCurExeDir();
  path.append(SDK_LIC_PATH);
  if (Exist(path)) {
    return path;
  }

  return String();
}

}  // namespace Util
}  // namespace HpdfToOffice
