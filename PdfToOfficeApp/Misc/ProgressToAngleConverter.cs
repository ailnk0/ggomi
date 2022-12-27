using System;
using System.Globalization;
using System.Windows.Controls;
using System.Windows.Data;

namespace PdfToOfficeApp
{
    public class ProgressToAngleConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            double progress = (double)values[0];
            ProgressBar bar = values[1] as ProgressBar;

            return 359.999 * (progress / (bar.Maximum - bar.Minimum));
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
