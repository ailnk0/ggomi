using System;
using System.Windows;

namespace PdfToOfficeApp
{
    public class Utils
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
    }
}
