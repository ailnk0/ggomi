using System;
using System.Collections.Specialized;
using System.IO;
using System.Windows.Controls;
using System.Windows.Input;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public partial class DocListBox : ListBox
    {
        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);

            Loaded += DocListBox_Loaded;
        }

        private void DocListBox_Loaded(object sender, System.Windows.RoutedEventArgs e)
        {
            if (GetModel() == null)
            {
                return;
            }

            GetModel().SelectedItems = SelectedItems;
        }

        protected override void OnItemsChanged(NotifyCollectionChangedEventArgs e)
        {
            base.OnItemsChanged(e);

            if (GetModel() == null)
            {
                return;
            }

            if (GetModel().Docs.Count > 0)
            {
                GetModel().AppStatus = APP_STATUS.READY;
            }
            else
            {
                GetModel().AppStatus = APP_STATUS.INIT;
            }
        }

        protected override void OnMouseDoubleClick(MouseButtonEventArgs e)
        {
            if (GetModel().AppStatus == APP_STATUS.COMPLETED)
            {
                // TODO : 출력 파일 경로 및 파일명 -> 출력될 파일 명명한 부분 찾아서 바꾸기
                System.Diagnostics.Process process = new System.Diagnostics.Process();
                DocListBox temp = (DocListBox)e.Source;
                DocList docList = (DocList)temp.ItemsSource;
                Doc doc = docList[0];
                string strFileName = doc.FileName;
                string strFilePath = doc.FilePath;
                strFilePath = strFilePath.Replace(strFileName, "");
                strFileName = strFileName.Replace(".pdf", "");
                string strFileType;
                switch (GetModel().ConvFileType)
                {
                    case FILE_TYPE.DOCX:
                        strFileType = ".docx";
                        break;
                    case FILE_TYPE.PPTX:
                        strFileType = ".pptx";
                        break;
                    case FILE_TYPE.XLSX:
                        strFileType = ".xlsx";
                        break;
                    case FILE_TYPE.IMAGE:
                        strFileType = ".image";
                        break;
                    default:
                        strFileType = ".docx";
                        break;
                }

                strFileName += strFileType;

                if (GetModel().IsSaveToUserDir)
                    strFilePath = GetModel().UserDir + "\\" + strFileName;
                else
                    strFilePath += strFileName;

                try//if (File.Exists(strFilePath))
                {
                    process.StartInfo.FileName = strFilePath;
                    process.Start();
                }
                catch
                {

                }
            }
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
