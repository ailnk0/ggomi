using System;
using System.ComponentModel;
using System.Windows.Controls;

namespace PdfToOfficeApp
{
    public class Doc : INotifyPropertyChanged
    {
        private string _filePath;
        private string _fileName;
        private ProgressBar progressBarItem;
        private int _progressValue;
        public System.Windows.Visibility visibility { get; set; }

        public Doc()
        {
        }

        public Doc(string filePath)
        {
            _filePath = filePath;
            progressBarItem = new ProgressBar();
            _progressValue = 0;
            visibility = System.Windows.Visibility.Visible;
            try
            {
                _fileName = System.IO.Path.GetFileName(_filePath);
            }
            catch (Exception)
            {
                _fileName = filePath;
            }
        }

        public string FilePath
        {
            get { return _filePath; }
            set
            {
                _filePath = value;
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

        public ProgressBar ProgressBarItem
        {
            get { return progressBarItem; }
            set
            {
                progressBarItem = value;
                OnPropertyChanged("ProgressBarItem");
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

        public event PropertyChangedEventHandler PropertyChanged;

        public override string ToString() => _fileName;

        protected void OnPropertyChanged(string info)
        {
            var handler = PropertyChanged;
            handler?.Invoke(this, new PropertyChangedEventArgs(info));
        }
    }
}
