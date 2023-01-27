using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
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
            private static readonly Uri SRC_KO_KR = new Uri("Resources/Strings/ko-KR/pdftooffice_strings.xaml", UriKind.RelativeOrAbsolute);
            private static readonly Uri SRC_EN_US = new Uri("Resources/Strings/en-US/pdftooffice_strings.xaml", UriKind.RelativeOrAbsolute);

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

                    case LANG.EN_US:
                        langRes.Source = SRC_EN_US;
                        break;

                    default:
                        langRes.Source = SRC_KO_KR;
                        break;
                }
            }
        }

        public class StringManager
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

            public static string BytesToString(long length)
            {
                try
                {
                    string strSize = "0Byte";
                    double dSize = 0;
                    if (length >= 8589900000)
                    {
                        dSize = length / (double)(1024 * 1024 * 1024);
                        strSize = Math.Round(dSize, 3) + "GB";
                    }
                    else if (length >= 1048576)
                    {
                        dSize = length / (double)(1024 * 1024);
                        strSize = Math.Round(dSize, 2) + "MB";
                    }
                    else if (length >= 1024)
                    {
                        dSize = length / (double)1024;
                        strSize = Math.Round(dSize, 2) + "KB";
                    }
                    else
                    {
                        strSize = length + "byte";
                    }
                    return strSize;
                }
                catch (Exception)
                {
                    return string.Empty;
                }
            }
        }

        public class PathManager
        {
            public static string GetAvailFileName(string path)
            {
                try
                {
                    string dir = Path.GetDirectoryName(path);
                    string name = Path.GetFileNameWithoutExtension(path);
                    string ext = Path.GetExtension(path);

                    int count = 0;
                    string tempPath = path;
                    var buf = new StringBuilder();
                    while (File.Exists(tempPath) || Directory.Exists(tempPath))
                    {
                        buf.Clear();
                        buf.Append(name);
                        buf.Append(" (");
                        buf.Append(++count);
                        buf.Append(")");
                        buf.Append(ext);

                        tempPath = Path.Combine(dir, buf.ToString());
                    };

                    return tempPath;
                }
                catch (Exception)
                {
                    return string.Empty;
                }
            }

            public static string GetOutPath(MainViewModel model, string sourcePath)
            {
                try
                {
                    string outPath = sourcePath;
                    if (model.IsSaveToUserDir && !string.IsNullOrEmpty(model.UserDir))
                    {
                        outPath = Path.Combine(model.UserDir, Path.GetFileName(outPath));
                        if(!Directory.Exists(model.UserDir))
                        {
                            Directory.CreateDirectory(model.UserDir);
                        }
                    }

                    string ext = null;
                    switch (model.ConvFileType)
                    {
                        case FILE_TYPE.DOCX:
                            ext = ".docx";
                            break;
                        case FILE_TYPE.PPTX:
                            ext = ".pptx";
                            break;
                        case FILE_TYPE.XLSX:
                            ext = ".xlsx";
                            break;
                        case FILE_TYPE.IMAGE:
                            switch (model.ConvImgType)
                            {
                                case IMG_TYPE.PNG:
                                    ext = ".png";
                                    break;
                                case IMG_TYPE.JPEG:
                                    ext = ".jpg";
                                    break;
                                case IMG_TYPE.GIF:
                                    ext = ".gif";
                                    break;
                                default:
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                    outPath = Path.ChangeExtension(outPath, ext);

                    if (!model.IsOverwrite)
                    {
                        outPath = GetAvailFileName(outPath);
                    }

                    return outPath;
                }
                catch (Exception)
                {
                    return string.Empty;
                }
            }
        }
    }
}
