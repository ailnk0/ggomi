using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text;
using System.Windows.Data;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public class TooltipValueConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                var convStatus = (CONV_STATUS)values[0];
                var filePath = (string)values[1];
                var resCode = (RES_CODE)values[2];
                var outPath = (string)values[3];

                FileInfo info;
                var msg = new StringBuilder();

                switch (convStatus)
                {
                    case CONV_STATUS.FAIL:
                        msg.AppendLine(filePath);
                        msg.AppendLine();
                        msg.AppendFormat("⚠ {0}", Util.StringManager.GetMsg(resCode));
                        break;
                    case CONV_STATUS.COMPLETED:
                        info = new FileInfo(outPath);
                        msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_PATH"), outPath);
                        msg.AppendLine();
                        msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_SIZE"), Util.StringManager.BytesToString(info.Length));
                        msg.AppendLine();
                        msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_WRITE_TIME"), info.LastWriteTime);
                        break;
                    default:
                        info = new FileInfo(filePath);
                        msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_PATH"), filePath);
                        msg.AppendLine();
                        msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_SIZE"), Util.StringManager.BytesToString(info.Length));
                        msg.AppendLine();
                        msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_WRITE_TIME"), info.LastWriteTime);
                        break;
                }
                return msg.ToString();
            }
            catch (Exception ex)
            {
                Debug.Assert(false, ex.Message);
                return null;
            }
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
