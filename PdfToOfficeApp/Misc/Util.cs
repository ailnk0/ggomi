﻿using System;
using System.Collections.ObjectModel;
using System.Linq;
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

        public class ResManager
        {
            public static Collection<ResourceDictionary> GetAppRes()
            {
                return Application.Current.Resources.MergedDictionaries;
            }
        }

        public class ThemeManager : ResManager
        {
            public enum THEME
            {
                NORMAL,
                DARK,
            }

            private static readonly Uri SRC_NORMAL = new Uri("Resources/Themes/Generic.xaml", UriKind.RelativeOrAbsolute);
            private static readonly Uri SRC_DARK = new Uri("Resources/Themes/Dark.xaml", UriKind.RelativeOrAbsolute);

            public static void Apply(THEME theme)
            {
                var themeRes = GetAppRes().FirstOrDefault(res => res.Source.OriginalString.StartsWith("Resources/Themes/"));
                if (themeRes == null)
                {
                    return;
                }

                switch (theme)
                {
                    case THEME.NORMAL:
                        themeRes.Source = SRC_NORMAL;
                        break;
                    case THEME.DARK:
                        themeRes.Source = SRC_DARK;
                        break;
                    default:
                        themeRes.Source = SRC_NORMAL;
                        break;
                }
            }
        }

        public class LangManager : ResManager
        {
            public enum LANG
            {
                KO_KR,
            }

            private static readonly Uri SRC_KO_KR = new Uri("Resources/Strings/ko-KR/pdftooffice_strings.xaml", UriKind.RelativeOrAbsolute);

            public static void Apply(LANG lang)
            {
                var langRes = GetAppRes().FirstOrDefault(res => res.Source.OriginalString.StartsWith("Resources/Strings/"));
                if (langRes == null)
                {
                    return;
                }

                switch (lang)
                {
                    case LANG.KO_KR:
                        langRes.Source = SRC_KO_KR;
                        break;

                    default:
                        langRes.Source = SRC_KO_KR;
                        break;
                }
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

            public static string GetMsg(RES_CODE status)
            {
                string msg;

                switch (status)
                {
                    case RES_CODE.Canceled:
                        msg = GetString("IDS_ConvertUserCancelMessage");
                        break;

                    case RES_CODE.Fail:
                    case RES_CODE.OCRCanceled:
                    case RES_CODE.CanceledExists:
                    case RES_CODE.InvalidLicense:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_CONVERSION_FAIL");
                        break;

                    case RES_CODE.FileHasCopyProtection:
                    case RES_CODE.UnsupportedEncryptionHandler:
                    case RES_CODE.MissingCertificate:
                    case RES_CODE.WrongPassword:
                    case RES_CODE.NoUserNoOwner:
                    case RES_CODE.NoUserOwner:
                    case RES_CODE.UserNoOwner:
                    case RES_CODE.UserOwner:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_PASSWORD_PROTECT");
                        break;

                    case RES_CODE.PdfAError:
                    case RES_CODE.PdfAFatalError:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_UNSUPPORT_FORMAT");
                        break;

                    case RES_CODE.BadData:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_BAD_FILE");
                        break;

                    case RES_CODE.InternalError:
                    case RES_CODE.UnavailableAction:
                    case RES_CODE.InvalidPagesRange:
                    case RES_CODE.NoBppConversion:
                    case RES_CODE.NoGrayscale:
                    case RES_CODE.PSDUnsupportedMode:
                    case RES_CODE.Unknown:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_CONVERSION_ERROR");
                        break;

                    case RES_CODE.NoTablesToExtract:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_NO_TABLES_TO_EXTRACT");
                        break;

                    case RES_CODE.NoImagesToExtract:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_NO_IMAGES_TO_EXTRACT");
                        break;

                    case RES_CODE.IOError:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_FILE_IO_ERROR");
                        break;

                    case RES_CODE.IOFileLocked:
                    case RES_CODE.AlreadyLoaded:
                        msg = GetString("IDS_HNC_PDFSDK_MSG_FILE_LOCKED");
                        break;
                    case RES_CODE.Success:
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
