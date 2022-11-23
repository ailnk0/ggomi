using System.ComponentModel;

namespace PdfToOfficeApp
{
    public class MainModel
    {
        // 변환 형식 콤보박스 
        public class Format
        {
            public string Name { get; set; }
        }

        // 목록에 출력할 파일 정보
        public class FileInformation : Notifier
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
        }
    }

    public class Notifier : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string property)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(property));
            }
        }
    }
}
