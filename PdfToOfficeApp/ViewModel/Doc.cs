using System;
using System.IO;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public class Doc : Notifier
    {
        public Doc()
        {
        }

        public Doc(string filePath)
        {
            FilePath = filePath;
            Tooltip = filePath;
        }

        private string _FilePath;
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

        private string _FileName;
        public string FileName
        {
            get { return _FileName; }
            set
            {
                _FileName = value;
                OnPropertyChanged("FileName");
            }
        }

        private string _Tooltip;
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
    }
}
