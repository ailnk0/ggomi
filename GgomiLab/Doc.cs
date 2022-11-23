using System;
using System.ComponentModel;

namespace GgomiLab
{
    public class Doc : INotifyPropertyChanged
    {
        private string _filePath;
        private string _fileName;

        public Doc()
        {
        }

        public Doc(string filePath)
        {
            _filePath = filePath;

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

        public event PropertyChangedEventHandler PropertyChanged;

        public override string ToString() => _fileName;

        protected void OnPropertyChanged(string info)
        {
            var handler = PropertyChanged;
            handler?.Invoke(this, new PropertyChangedEventArgs(info));
        }
    }
}
