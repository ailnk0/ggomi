using System;
using System.Collections;

namespace PdfToOfficeApp
{
    public class MainViewModel : Notifier, IDisposable
    {
        public MainViewModel()
        {
            _Docs = new DocList();
            _Docs.CollectionChanged += _Docs_CollectionChanged;
            _imageFormat = FileFormat.JPEG;
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

        private FileFormat _selectedFileFormat;
        public FileFormat SelectedFileFormat
        {
            get { return _selectedFileFormat; }
            set { _selectedFileFormat = value;
                OnPropertyChanged("SelectedFileFormat");
            }
        }

        private FileFormat _imageFormat;
        public FileFormat ImageFormat
        {
            get { return _imageFormat; }
            set { _imageFormat = value;
                OnPropertyChanged("ImageFormat");
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

        // 이미지 형식 체크
        private bool _IsImage;
        public bool IsImage
        {
            get { return _IsImage; }
            set
            {
                _IsImage = value;
                OnPropertyChanged("IsImage");
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
