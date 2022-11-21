using System;
using System.Collections.Generic;
using static PdfToOfficeApp.MainModel;

namespace PdfToOfficeApp
{
    public class MainViewModel : Notifier
    {
        public MainViewModel()
        {

        }

        private Format _selectedFormat;

        public Format SelectedFormat
        {
            get { return _selectedFormat; }
            set { _selectedFormat = value; }
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

        public int RunSample(string path)
        {
            PdfToOffice pdfToOffice = new PdfToOffice();

            return pdfToOffice.RunSample(path);
        }

        public Action CloseAction { get; set; }
    }
}
