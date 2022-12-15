using System;
using System.Windows;
using System.Windows.Input;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    namespace Util
    {
        public class Commands
        {
            public static void Add(FrameworkElement element, RoutedCommand command, ExecutedRoutedEventHandler execute, CanExecuteRoutedEventHandler canExecute)
            {
                element.CommandBindings.Add(
                    new CommandBinding(command,
                        new ExecutedRoutedEventHandler(execute),
                        new CanExecuteRoutedEventHandler(canExecute)));
            }
        }

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
                string msg;

                switch (status)
                {
                    case ErrorStatus.Canceled:
                        msg = GetString("IDS_ConvertUserCancelMessage");
                        break;

                    case ErrorStatus.Fail:
                    case ErrorStatus.OCRCanceled:
                    case ErrorStatus.CanceledExists:
                    case ErrorStatus.InvalidLicense:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_CONVERSION_FAIL");
                        break;

                    case ErrorStatus.FileHasCopyProtection:
                    case ErrorStatus.UnsupportedEncryptionHandler:
                    case ErrorStatus.MissingCertificate:
                    case ErrorStatus.WrongPassword:
                    case ErrorStatus.NoUserNoOwner:
                    case ErrorStatus.NoUserOwner:
                    case ErrorStatus.UserNoOwner:
                    case ErrorStatus.UserOwner:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_PASSWORD_PROTECT");
                        break;

                    case ErrorStatus.PdfAError:
                    case ErrorStatus.PdfAFatalError:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_UNSUPPORT_FORMAT");
                        break;

                    case ErrorStatus.BadData:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_BAD_FILE");
                        break;

                    case ErrorStatus.InternalError:
                    case ErrorStatus.UnavailableAction:
                    case ErrorStatus.InvalidPagesRange:
                    case ErrorStatus.NoBppConversion:
                    case ErrorStatus.NoGrayscale:
                    case ErrorStatus.PSDUnsupportedMode:
                    case ErrorStatus.Unknown:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_CONVERSION_ERROR");
                        break;

                    case ErrorStatus.NoTablesToExtract:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_NO_TABLES_TO_EXTRACT");
                        break;

                    case ErrorStatus.NoImagesToExtract:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_NO_IMAGES_TO_EXTRACT");
                        break;

                    case ErrorStatus.IOError:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_FILE_IO_ERROR");
                        break;

                    case ErrorStatus.IOFileLocked:
                    case ErrorStatus.AlreadyLoaded:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_FILE_LOCKED");
                        break;
                    case ErrorStatus.Success:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_CONVERSION_SUCCESS");
                        break;

                    default:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_CONVERSION_ERROR");
                        break;
                }

                return msg;
            }
        }
    }
}
