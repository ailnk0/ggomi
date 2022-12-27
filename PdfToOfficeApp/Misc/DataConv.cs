using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace PdfToOfficeApp
{
    public class DataConv : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value.Equals(parameter))
            {
                return true;
            }

            return false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            bool isCheck = (bool)value;
            if (isCheck)
            {
                return parameter;
            }
            else
            {
                return DependencyProperty.UnsetValue;
            }
        }
    }
}
