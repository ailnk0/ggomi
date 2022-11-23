using PdfToOfficeAppModule;
using System;
using System.Windows;

namespace PdfToOfficeApp
{
    namespace Util
    {
        public class String
        {
            public static string GetString(string id)
            {
                try
                {
                    return Application.Current.FindResource(id) as string;
                }
                catch (Exception)
                {
                    return string.Empty;
                }
            }

            public static string GetMsg(ErrorStatus status)
            {
                switch (status)
                {
                    case ErrorStatus.Success:
                        return GetString("IDS_Msg_Success");
                    case ErrorStatus.InvalidLicense:
                        return GetString("IDS_Msg_InvalidLicense");
                    case ErrorStatus.InternalError:
                        return GetString("IDS_Msg_InternalError");
                    case ErrorStatus.IOError:
                        return GetString("IDS_Msg_IOError");
                    case ErrorStatus.IOFileLocked:
                        return GetString("IDS_Msg_IOFileLocked");
                    default:
                        return GetString("IDS_Msg_Unknown");
                }
            }
        }
    }
}
