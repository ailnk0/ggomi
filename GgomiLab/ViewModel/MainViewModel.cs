using System;
using System.ComponentModel;

namespace GgomiLab
{
    public class MainViewModel : INotifyPropertyChanged, IDisposable
    {
        private DocList _docs;

        public DocList Docs
        {
            get { return _docs; }
        }

        private void Docs_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            OnPropertyChanged("FileName");
        }

        private AppStatus _status = AppStatus.Init;

        public AppStatus Status
        {
            get { return _status; }
            set
            {
                _status = value;
                OnPropertyChanged("Status");
            }
        }

        private bool _showMsg = true;

        public bool ShowMsg
        {
            get { return _showMsg; }
            set
            {
                _showMsg = value;
                OnPropertyChanged("ShowMsg");
            }
        }

        public MainViewModel()
        {
            _docs = new DocList();
            _docs.CollectionChanged += Docs_CollectionChanged;
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string info)
        {
            var handler = PropertyChanged;
            handler?.Invoke(this, new PropertyChangedEventArgs(info));
        }

        public void Dispose()
        {
            if (_docs != null)
            {
                _docs.CollectionChanged -= Docs_CollectionChanged;
                _docs = null;
            }
        }
    }
}
