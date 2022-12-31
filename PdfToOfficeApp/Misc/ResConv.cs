using System;
using System.Globalization;
using System.Windows.Data;

namespace PdfToOfficeApp
{
    public class ResConv : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            LANG lang = (LANG)value;
            switch (lang)
            {
                case LANG.KO_KR:
                    return Util.String.GetString("IDS_OPT_LANG_KO_KR");
                case LANG.EN_US:
                    return Util.String.GetString("IDS_OPT_LANG_EN_US");
                default:
                    return Util.String.GetString("IDS_OPT_LANG_KO_KR");
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
