using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public class ImgTypeDataConv : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if ((IMG_TYPE)value == (IMG_TYPE)parameter)
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
