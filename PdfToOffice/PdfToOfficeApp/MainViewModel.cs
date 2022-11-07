using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Forms;
using System.Security.Cryptography;
using System.Data.SqlClient;
using System.Collections.ObjectModel;
using System.Reflection.Emit;
using System.IO;
using static PdfToOfficeApp.MainViewModel;
using System.Reflection;
using System.Data.Common;
using System.Runtime.InteropServices;
using static PdfToOfficeApp.MainModel;

namespace PdfToOfficeApp
{
    public class MainViewModel : INotifyPropertyChanged
    {
        public MainViewModel()
        {
            fileInformations = new ObservableCollection<FileInformation>();
            Init();
        }
        // 변환 형식 콤보박스 아이템
        private IList<Format> _formats = new List<Format>
        {
            new Format{Name = "한글 문서(*.hwp)"},
            new Format{Name = "워드 문서(*.docx)"},
            new Format{Name = "파워포인트 문서(*.pptx)"},
            new Format{Name = "엑셀 문서(*.xlsx)"},
            new Format{Name = "PNG 형식(*.png)"}
        };

        public IList<Format> Formats
        {
            get { return _formats; }
        }

        private Format _selectedFormat;
        public Format SelectedFormat
        {
            get { return _selectedFormat; }
            set { _selectedFormat = value; }
        }

        private void Init()
        {
            SelectedFormat = Formats.FirstOrDefault();
        }

        #region 변수
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
        private ObservableCollection<FileInformation> fileInformations;
        public ObservableCollection<FileInformation> FileInformations
        {
            get { return fileInformations; }
            set
            {
                fileInformations = value;
                OnPropertyChanged("FileInformations");
            }
        }
        private FileInformation seletedFileInfo;
        public FileInformation SeletedFileInfo
        {
            get { return seletedFileInfo; }
            set
            {
                seletedFileInfo = value;
                OnPropertyChanged("SeletedFileInfo");
            }
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string property)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(property));
            }
        }

        #endregion

        #region Command

        private ICommand testCommand;
        public ICommand TestCommand
        {
            get { return (this.testCommand) ?? (this.testCommand = new MainCommand(test)); }
        }


        // 파일 추가 버튼
        private ICommand addFileCommand;
        public ICommand AddFileCommand
        {
            get { return (this.addFileCommand) ?? (this.addFileCommand = new MainCommand(AddFile)); }
        }

        // 파일 제거 버튼
        private ICommand removeFileCommand;
        public ICommand RemoveFileCommand
        {
            get { return (this.removeFileCommand) ?? (this.removeFileCommand = new MainCommand(RemoveFile)); }
        }

        // 저장 경로 설정 버튼
        private ICommand openFolderCommand;
        public ICommand OpenFolderCommand
        {
            get { return (this.openFolderCommand) ?? (this.openFolderCommand = new MainCommand(OpenFolder)); }
        }

        // 변환하기 버튼
        private ICommand okCommand;
        public ICommand OkCommand
        {
            get { return (this.okCommand) ?? (this.okCommand = new MainCommand(Ok)); }
        }

        // 취소 버튼
        private ICommand cancelCommand;
        public ICommand CancelCommand
        {
            get { return (this.cancelCommand) ?? (this.cancelCommand = new MainCommand(Cancel)); }
        }
        #endregion

        private void test()
        {
            FileInformation fileInformation = new FileInformation();
            fileInformation.StrFileName = "name";
            fileInformation.StrFilePath = "path";
            fileInformation.StrFileSize = "Bytes";
            FileInformations.Add(fileInformation);
        }

        // 파일 추가
        private void AddFile()
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.DefaultExt = ".pdf";
            openFileDialog.Filter = "PDF 문서 (*.pdf)|*.pdf";
            openFileDialog.Multiselect = true;

            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                foreach (string file in openFileDialog.FileNames)
                {
                    FileInfo fileInfo = new FileInfo(file);

                    FileInformation fileInformation = new FileInformation();

                    fileInformation.StrFileName = fileInfo.Name;
                    fileInformation.StrFilePath = fileInfo.DirectoryName;
                    fileInformation.StrFileSize = fileInfo.Length.ToString() + " Bytes";
                    FileInformations.Add(fileInformation);
                }
            }
        }

        // 파일 삭제
        private void RemoveFile()
        {
            
            if(seletedFileInfo != null)
            {
                FileInformations.Remove(seletedFileInfo);
            }
        }


        // 저장 경로 설정
        private void OpenFolder()
        {
            FolderBrowserDialog folderBrowserDialog = new FolderBrowserDialog();
            if (folderBrowserDialog.ShowDialog() == DialogResult.OK)
            {
                StrSavePath = folderBrowserDialog.SelectedPath;
            }
        }

        // 변환하기
        private void Ok()
        {
            int resultCode = RunSample();
            if (resultCode == 0)
            {
                MessageBox.Show("Conversion is done.");
            }
            else
            {
                MessageBox.Show("Conversion is failed.");
            }
        }

        public int RunSample()
        {
            PdfToOffice pdfToOffice = new PdfToOffice();
            return pdfToOffice.RunSample();
        }

        public Action CloseAction { get; set; }
        // 취소
        private void Cancel()
        {
            CloseAction();
        }
    
        // 목록에 출력할 파일 정보
        public class FileInformation : INotifyPropertyChanged
        {
            // 파일 이름
            private string _StrFileName;
            public string StrFileName
            {
                get { return _StrFileName; }
                set
                {
                    _StrFileName = value;
                    OnPropertyChanged("StrFileName");
                }
            }

            // 파일 쪽수
            private int _NFilePage;
            public int NFilePage
            {
                get { return _NFilePage; }
                set
                {
                    _NFilePage = value;
                    OnPropertyChanged("NFilePage");
                }
            }

            // 파일 위치
            private string _StrFilePath;
            public string StrFilePath
            {
                get { return _StrFilePath; }
                set
                {
                    _StrFilePath = value;
                    OnPropertyChanged("StrFilePath");
                }
            }

            // 파일 크기
            private string _StrFileSize;
            public string StrFileSize
            {
                get { return _StrFileSize; }
                set
                {
                    _StrFileSize = value;
                    OnPropertyChanged("StrFileSize");
                }
            }

            // 파일 변환 범위
            private string _StrFileRange;
            public string StrFileRange
            {
                get { return _StrFileRange; }
                set
                {
                    _StrFileRange = value;
                    OnPropertyChanged("StrFileRange");
                }
            }

            public event PropertyChangedEventHandler PropertyChanged;

            protected void OnPropertyChanged(string propertyName)
            {
                if (PropertyChanged != null)
                {
                    PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
                }
            }
        }

        
    }
}
