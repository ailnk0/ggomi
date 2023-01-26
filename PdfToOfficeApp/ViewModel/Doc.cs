using System;
using System.IO;
using System.Text;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public class Doc : Notifier
    {
        public Doc()
        {
        }

        public Doc(FileInfo info)
        {
            FilePath = info.FullName;
            StringBuilder msg = new StringBuilder();
            msg.AppendFormat("{0}{1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_PATH"), FilePath);
            msg.AppendLine();
            msg.AppendFormat("{0}{1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_SIZE"), Util.StringManager.GetFileSize(info.Length));
            msg.AppendLine();
            msg.AppendFormat("{0}{1}", Util.StringManager.GetString("IDS_TOOLTIP_MSG_WRITE_TIME"), info.LastWriteTime);
            Tooltip = msg.ToString();
        }

        private string _FilePath = string.Empty;
        public string FilePath
        {
            get { return _FilePath; }
            set
            {
                _FilePath = value;
                try
                {
                    FileName = Path.GetFileName(_FilePath);
                }
                catch (Exception)
                {
                    FileName = _FilePath;
                }
                OnPropertyChanged("FilePath");
                OnPropertyChanged("FileName");
            }
        }

        private string _FileName = string.Empty;
        public string FileName
        {
            get { return _FileName; }
            set
            {
                _FileName = value;
                OnPropertyChanged("FileName");
            }
        }

        private string _Password = string.Empty;
        public string Password
        {
            get { return _Password; }
            set
            {
                _Password = value;
                OnPropertyChanged("Password");
            }
        }

        private string _Tooltip = string.Empty;
        public string Tooltip
        {
            get { return _Tooltip; }
            set
            {
                _Tooltip = value;
                OnPropertyChanged("Tooltip");
            }
        }

        private CONV_STATUS _ConvStatus = CONV_STATUS.READY;
        public CONV_STATUS ConvStatus
        {
            get { return _ConvStatus; }
            set
            {
                _ConvStatus = value;
                OnPropertyChanged("ConvStatus");
            }
        }

        private int _ProgressValue = 0;
        public int ProgressValue
        {
            get { return _ProgressValue; }
            set
            {
                _ProgressValue = value;
                OnPropertyChanged("ProgressValue");
            }
        }

        private RES_CODE _ResCode = RES_CODE.Unknown;
        public RES_CODE ResCode
        {
            get { return _ResCode; }
            set
            {
                _ResCode = value;
                OnPropertyChanged("ResCode");
            }
        }

        private string _OutPath = string.Empty;
        public string OutPath
        {
            get { return _OutPath; }
            set
            {
                _OutPath = value;
                OnPropertyChanged("OutPath");
            }
        }

        private string _FileSize = string.Empty;
        public string FileSize
        {
            get { return _FileSize; }
            set
            {
                _FileSize = value;
                OnPropertyChanged("FileSize");
            }
        }

        private string _WriteTime = string.Empty;
        public string WriteTime
        {
            get { return _WriteTime; }
            set
            {
                _WriteTime = value;
                OnPropertyChanged("WriteTime");
            }
        }
    }
}
