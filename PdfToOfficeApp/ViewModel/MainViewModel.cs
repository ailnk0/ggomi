using System;
using System.Collections;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public class MainViewModel : Notifier, IDisposable
    {
        public MainViewModel()
        {
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

        private DocList _Docs = new DocList();
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

        private FILE_TYPE _ConvFileType = FILE_TYPE.DOCX;
        public FILE_TYPE ConvFileType
        {
            get { return _ConvFileType; }
            set
            {
                _ConvFileType = value;
                OnPropertyChanged("ConvFileType");
            }
        }

        private IMG_TYPE _ConvImgType = IMG_TYPE.PNG;
        public IMG_TYPE ConvImgType
        {
            get { return _ConvImgType; }
            set
            {
                _ConvImgType = value;
                OnPropertyChanged("ConvImgType");
            }
        }

        private APP_STATUS _AppStatus = APP_STATUS.INIT;
        public APP_STATUS AppStatus
        {
            get { return _AppStatus; }
            set
            {
                _AppStatus = value;
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

        private string _AppVersion = string.Empty;
        public string AppVersion
        {
            get { return _AppVersion; }
            set
            {
                _AppVersion = value;
                OnPropertyChanged("AppVersion");
            }
        }

        private string _UserDir = string.Empty;
        public string UserDir
        {
            get { return _UserDir; }
            set
            {
                _UserDir = value;
                OnPropertyChanged("UserDir");
            }
        }

        private bool _IsSaveToUserDir = false;
        public bool IsSaveToUserDir
        {
            get { return _IsSaveToUserDir; }
            set
            {
                _IsSaveToUserDir = value;
                OnPropertyChanged("IsSaveToUserDir");
            }
        }

        private bool _AllowOverwrite = true;
        public bool AllowOverwrite
        {
            get { return _AllowOverwrite; }
            set
            {
                _AllowOverwrite = value;
                OnPropertyChanged("AllowOverwrite");
            }
        }

        private bool _AllowDarkTheme = false;
        public bool AllowDarkTheme
        {
            get { return _AllowDarkTheme; }
            set
            {
                _AllowDarkTheme = value;

                if (value)
                {
                    Util.ThemeManager.Apply(THEME.DARK);
                }
                else
                {
                    Util.ThemeManager.Apply(THEME.NORMAL);
                }

                OnPropertyChanged("AllowDarkTheme");
            }
        }

        private LANG _lang = LANG.KO_KR;
        public LANG Lang
        {
            get { return _lang; }
            set
            {
                _lang = value;

                Util.LangManager.Apply(_lang);

                OnPropertyChanged("Lang");
            }
        }
    }
}
