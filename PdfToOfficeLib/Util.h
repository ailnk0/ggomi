#pragma once

#include "Define.h"

namespace HpdfToOffice {
namespace Util {

class Path {
 public:
  static const String SDK_PATH;
  static const String SDK_LIC_PATH;

 public:
  static bool Exist(const String& path);
  static String GetCurExePath();
  static String GetCurExeDir();
  static String GetDirName(const String& path);
  static String GetFileName(const String& path);
  static String GetSdkPath();
  static String GetSdkDir();
  static String GetSdkLicPath();
};

}  // namespace Util
}  // namespace HpdfToOffice
