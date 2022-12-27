using System;
using System.ComponentModel;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public class Doc : INotifyPropertyChanged
    {
        private int _index;
        private string _filePath;
        private string _fileName;
        private string _tooltip;
        private CONV_STATUS _conversionStatus = CONV_STATUS.READY;
        private int _progressValue = 0;
        private RES_CODE _resCode = RES_CODE.Unknown;

        public Doc()
        {
        }

        public Doc(string filePath)
        {
            FilePath = filePath;
            Tooltip = filePath;
        }

        public int Index
        {
            get { return _index; }
            set
            {
                _index = value;
                OnPropertyChanged("Index");
            }
        }

        public string FilePath
        {
            get { return _filePath; }
            set
            {
                _filePath = value;
                try
                {
                    FileName = System.IO.Path.GetFileName(_filePath);
                }
                catch (Exception)
                {
                    FileName = _filePath;
                }
                OnPropertyChanged("FilePath");
                OnPropertyChanged("FileName");
            }
        }

        public string FileName
        {
            get { return _fileName; }
            set
            {
                _fileName = value;
                OnPropertyChanged("FileName");
            }
        }

        public string Tooltip
        {
            get { return _tooltip; }
            set
            {
                _tooltip = value;
                OnPropertyChanged("Tooltip");
            }
        }

        public CONV_STATUS ConversionStatus
        {
            get { return _conversionStatus; }
            set
            {
                _conversionStatus = value;
                OnPropertyChanged("ConversionStatus");
            }
        }

        public int ProgressValue
        {
            get { return _progressValue; }
            set
            {
                _progressValue = value;
                OnPropertyChanged("ProgressValue");
            }
        }

        public RES_CODE ResCode
        {
            get { return _resCode; }
            set
            {
                _resCode = value;
                OnPropertyChanged("ResCode");
            }
        }

        public override string ToString() => FileName;

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string info)
        {
            var handler = PropertyChanged;
            handler?.Invoke(this, new PropertyChangedEventArgs(info));
        }
    }
}
