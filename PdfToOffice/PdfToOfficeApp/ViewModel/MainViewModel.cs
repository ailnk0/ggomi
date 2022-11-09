using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using static PdfToOfficeApp.MainModel;

namespace PdfToOfficeApp
{
    public class MainViewModel : Notifier
    {
        public MainViewModel()
        {
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
            fileInformations = new ObservableCollection<FileInformation>();
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

        public int RunSample(string path)
        {
            PdfToOffice pdfToOffice = new PdfToOffice();
            return pdfToOffice.RunSample(path);
        }

        public Action CloseAction { get; set; }
    }
}
