using System;
using System.Globalization;
using System.IO;
using System.Text;
using System.Windows.Data;

namespace PdfToOfficeApp
{
    public class TooltipValueConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            // values[0] : doc.ConvStatus
            // values[1] : doc.FilePath
            // values[2] : doc.ResCode
            // values[3] : doc.OutPath

            FileInfo info;
            StringBuilder msg = new StringBuilder();
            try
            {
                if ((CONV_STATUS)values[0] == CONV_STATUS.FAIL)
                {
                    msg.AppendLine(values[1].ToString());
                    msg.AppendLine();
                    msg.AppendFormat("⚠ {0}", Util.StringManager.GetMsg((PdfToOfficeAppModule.RES_CODE)values[2]));
                }
                else if ((CONV_STATUS)values[0] == CONV_STATUS.COMPLETED)
                {
                    info = new FileInfo(values[3].ToString());
                    msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_PATH"), values[3]);
                    msg.AppendLine();
                    msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_SIZE"), Util.StringManager.BytesToString(info.Length));
                    msg.AppendLine();
                    msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_WRITE_TIME"), info.LastWriteTime);
                }
                else
                {
                    info = new FileInfo(values[1].ToString());
                    msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_PATH"), values[1]);
                    msg.AppendLine();
                    msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_SIZE"), Util.StringManager.BytesToString(info.Length));
                    msg.AppendLine();
                    msg.AppendFormat("{0} : {1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_WRITE_TIME"), info.LastWriteTime);
                }
            }
            catch
            {
                return null;
            }

            return msg.ToString();
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
