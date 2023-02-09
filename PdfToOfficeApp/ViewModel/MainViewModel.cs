using System;
using System.Collections;
using System.Reflection;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public class MainViewModel : Notifier, IDisposable
    {
        private bool disposed = false;

        public MainViewModel()
        {
        }

        ~MainViewModel()
        {
            Dispose();
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposed)
                return;

            if (disposing)
            {
                if (_Docs != null)
                {
                    _Docs.CollectionChanged -= _Docs_CollectionChanged;
                    _Docs = null;
                }
            }

            disposed = true;
        }

        public void Init()
        {
            Docs = new DocList();
            if (_Docs != null)
            {
                _Docs.CollectionChanged += _Docs_CollectionChanged;
            }

            SelectedItems = null;
            ConvFileType = FILE_TYPE.DOCX;
            ConvImgType = IMG_TYPE.PNG;
            AppStatus = APP_STATUS.INIT;
            AppVersion = Assembly.GetExecutingAssembly().GetName().Version.ToString();
            UserDir = Properties.Settings.Default.UserDir;
            IsSaveToUserDir = Properties.Settings.Default.IsSaveToUserDir;
            IsOverwrite = Properties.Settings.Default.IsOverwrite;
            IsDarkTheme = Properties.Settings.Default.IsDarkTheme;
            Lang = Properties.Settings.Default.Lang;

            if (string.IsNullOrEmpty(UserDir))
            {
                UserDir = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
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

        public ADD_TYPE AddType
        {
            get { return Properties.Settings.Default.AddType; }
            set
            {
                Properties.Settings.Default.AddType = value;
                Properties.Settings.Default.Save();

                OnPropertyChanged("AddType");
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

        public string UserDir
        {
            get { return Properties.Settings.Default.UserDir; }
            set
            {
                Properties.Settings.Default.UserDir = value;
                Properties.Settings.Default.Save();

                OnPropertyChanged("UserDir");
            }
        }

        public bool IsSaveToUserDir
        {
            get { return Properties.Settings.Default.IsSaveToUserDir; }
            set
            {
                Properties.Settings.Default.IsSaveToUserDir = value;
                Properties.Settings.Default.Save();

                OnPropertyChanged("IsSaveToUserDir");
            }
        }

        public bool IsOverwrite
        {
            get { return Properties.Settings.Default.IsOverwrite; }
            set
            {
                Properties.Settings.Default.IsOverwrite = value;
                Properties.Settings.Default.Save();

                OnPropertyChanged("IsOverwrite");
            }
        }

        public bool IsDarkTheme
        {
            get { return Properties.Settings.Default.IsDarkTheme; }
            set
            {
                Properties.Settings.Default.IsDarkTheme = value;
                Properties.Settings.Default.Save();

                if (value)
                {
                    Util.ThemeManager.Apply(THEME.DARK);
                }
                else
                {
                    Util.ThemeManager.Apply(THEME.NORMAL);
                }

                OnPropertyChanged("IsDarkTheme");
            }
        }

        public LANG Lang
        {
            get { return Properties.Settings.Default.Lang; }
            set
            {
                Properties.Settings.Default.Lang = value;
                Properties.Settings.Default.Save();

                Util.LangManager.Apply(value);

                OnPropertyChanged("Lang");
            }
        }
    }
}
