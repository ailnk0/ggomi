using System;
using System.Collections;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public class MainViewModel : Notifier, IDisposable
    {
        public MainViewModel()
        {
            _Docs = new DocList();
            _Docs.CollectionChanged += _Docs_CollectionChanged;
        }

        public void Dispose()
        {
            if (_Docs != null)
            {
                _Docs.CollectionChanged -= _Docs_CollectionChanged;
                _Docs = null;
            }
        }

        private void _Docs_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            OnPropertyChanged("Docs");
        }

        private DocList _Docs;
        public DocList Docs
        {
            get { return _Docs; }
            set
            {
                _Docs = value;
                OnPropertyChanged("Docs");
            }
        }

        private IList _SelectedItems;
        public IList SelectedItems
        {
            get { return _SelectedItems; }
            set
            {
                _SelectedItems = value;
                OnPropertyChanged("SelectedItems");
            }
        }

        private FILE_TYPE _convFileType = FILE_TYPE.XLSX;
        public FILE_TYPE ConvFileType
        {
            get { return _convFileType; }
            set
            {
                _convFileType = value;
                OnPropertyChanged("ConvFileType");
            }
        }

        private IMG_TYPE _convImgType = IMG_TYPE.PNG;
        public IMG_TYPE ConvImgType
        {
            get { return _convImgType; }
            set
            {
                _convImgType = value;
                OnPropertyChanged("ConvImgType");
            }
        }

        private APP_STATUS _appStatus = APP_STATUS.INIT;
        public APP_STATUS AppStatus
        {
            get { return _appStatus; }
            set
            {
                _appStatus = value;
                OnPropertyChanged("AppStatus");
            }
        }

        private bool _ShowMsg = true;
        public bool ShowMsg
        {
            get { return _ShowMsg; }
            set
            {
                _ShowMsg = value;
                OnPropertyChanged("ShowMsg");
            }
        }
    }
}
