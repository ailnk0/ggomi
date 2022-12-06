using System;
using System.ComponentModel;
using System.Windows.Controls;

namespace PdfToOfficeApp
{
    public class Doc : INotifyPropertyChanged
    {
        private string _filePath;
        private string _fileName;
        private int _progressValue = 0;

        public Doc()
        {
        }

        public Doc(string filePath)
        {
            FilePath = filePath;
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

        public int ProgressValue
        {
            get { return _progressValue; }
            set
            {
                _progressValue = value;
                OnPropertyChanged("ProgressValue");
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
