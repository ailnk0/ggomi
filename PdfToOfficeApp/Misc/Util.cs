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
                    case ErrorStatus.Fail:
                        return GetString("IDS_Msg_Fail");
                    default:
                        return GetString("IDS_Msg_Unknown");
                }
            }
        }
    }
}
