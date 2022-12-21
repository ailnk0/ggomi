using System;
using System.Collections;
using PdfToOfficeApp.Misc;

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

        // 변환 형식
        private string _FormatName;
        public string FormatName
        {
            get { return _FormatName; }
            set
            {
                _FormatName = value;
                OnPropertyChanged("FormatName");
            }
        }

        // 변환 파일 저장 경로
        private string _StrSavePath;

        public string StrSavePath
        {
            get { return _StrSavePath; }
            set
            {
                _StrSavePath = value;
                OnPropertyChanged("StrSavePath");
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
