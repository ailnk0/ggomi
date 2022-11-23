#include "pch.h"

#include "Util.h"

namespace HpdfToOffice
{
namespace Util
{

const String Path::SDK_PATH = L"/HncPdfSdk/";
const String Path::SDK_LIC_PATH = L"/../../SolidFrameworkLicense/license.xml";

String Path::GetCurExePath()
{
    wchar_t buf[MAX_PATH];
    GetModuleFileName(nullptr, buf, MAX_PATH);
    return buf;
}

String Path::GetCurDir()
{
    String buf = GetCurExePath();
    buf = GetDirName(buf);
    return buf;
}

String Path::GetDirName(const String &path)
{
    String buf;
    auto pos = path.find_last_of(L"\\/");
    if (pos != String::npos)
    {
        buf = path.substr(0, pos);
    }
    return buf;
}

String Path::GetFileName(const String &path)
{
    String buf;
    auto pos = path.find_last_of(L"\\/");
    if (pos != String::npos)
    {
        buf = path.substr(pos + 1);
    }
    return buf;
}

String Path::GetSdkPath()
{
    String buf = GetCurDir();
    buf.append(SDK_PATH);
    return buf;
}

String Path::GetSdkLicPath()
{
    String buf = GetCurDir();
    buf.append(SDK_LIC_PATH);
    return buf;
}

} // namespace Util
} // namespace HpdfToOffice
