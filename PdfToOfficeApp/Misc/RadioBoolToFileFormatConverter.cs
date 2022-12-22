﻿using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace PdfToOfficeApp
{
    public class RadioBoolToFileFormatConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string param = parameter as string;

            if (param == null)
                return DependencyProperty.UnsetValue;

            if (Enum.IsDefined(value.GetType(), value) == false)
                return DependencyProperty.UnsetValue;

            object paramValue = Enum.Parse(value.GetType(), param);
            return param.Equals(value);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string param = parameter as string;
            if (param == null)
                return DependencyProperty.UnsetValue;

            return Enum.Parse(targetType, param);
        }
    }
}
