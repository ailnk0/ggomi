using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Microsoft.Win32;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window, IDisposable
    {
        private PdfToOfficeProxy pdfToOffice;
        private BackgroundWorker worker = new BackgroundWorker();
        private ProgressSiteCli progressSiteCli { get; set; }

        public MainWindow()
        {
            InitializeComponent();
        }

        public void Dispose()
        {
            if (pdfToOffice != null)
            {
                pdfToOffice.Dispose();
                pdfToOffice = null;
            }

            if (worker != null)
            {
                worker.Dispose();
                worker = null;
            }

            ContentRendered -= MainWindow_ContentRendered;
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);
            AddCommandHandlers();

            ContentRendered += MainWindow_ContentRendered;

            worker.DoWork += Worker_DoWork;
            worker.RunWorkerCompleted += Worker_RunWorkerCompleted;
            worker.ProgressChanged += Worker_ProgressChanged;
            worker.WorkerReportsProgress = true;
            worker.WorkerSupportsCancellation = true;
        }

        private void Worker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {

        }

        // 작업 완료
        private void Worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            string strResult;
            if (e.Error != null)
            {
                strResult = Util.String.GetMsg(ErrorStatus.Unknown);
            }
            else if (e.Cancelled)
            {
                strResult = Util.String.GetMsg(ErrorStatus.Canceled);
            }
            else
            {
                strResult = "";
                foreach (var doc in GetModel().Docs)
                {
                    if (doc.FileErrorStatus != ErrorStatus.Success)
                    {
                        strResult += string.Format("{0} : " + Util.String.GetMsg(doc.FileErrorStatus) + "\n", doc.FileName);
                    }
                }
                if (strResult == "")
                    strResult = Util.String.GetMsg(ErrorStatus.Success);
            }

            GetModel().Status = AppStatus.Completed;

            if (GetModel().ShowMsg)
            {
                MessageBox.Show(strResult);
            }

            // TODO : 변환 작업 완료되면 변환 버튼을 돌아가기 버튼으로 바꾸기
        }

        private void Worker_DoWork(object sender, DoWorkEventArgs e)
        {
            // TODO : 변환 작업 중일 때는 변환 버튼을 정지 버튼으로 바꾸기
            foreach (Doc doc in (DocList)e.Argument)
            {
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                    return;
                }

                progressSiteCli = new ProgressSiteCli(doc);
                pdfToOffice.SetProgressSiteCli(progressSiteCli);

                string path = doc.FilePath;

                doc.ConversionStatus = FileConversionStatus.Running;

                ErrorStatus status = pdfToOffice.DoWordConversion(path, "");
                doc.FileErrorStatus = status;

                if (status == ErrorStatus.Success)
                {
                    doc.ConversionStatus = FileConversionStatus.Completed;
                }
                else
                {
                    doc.ConversionStatus = FileConversionStatus.Fail;
                }
            }
        }

        private void AddCommandHandlers()
        {
            Util.Commands.Add(this, ApplicationCommands.Open, OnOpen, CanOpen);
            Util.Commands.Add(this, AddFileCommand, OnAddFile, CanAddFile);
            Util.Commands.Add(this, RemoveFileCommand, OnRemoveFile, CanRemoveFile);
            Util.Commands.Add(this, ConvertCommand, OnConvert, CanConvert);
            Util.Commands.Add(this, StopCommand, OnStop, CanStop);
            Util.Commands.Add(this, ResetCommand, OnReset, CanReset);
        }

        private void MainWindow_ContentRendered(object sender, EventArgs e)
        {
            string[] arrArg = Environment.GetCommandLineArgs();
            if (arrArg.Length >= 3)
            {
                WindowContextExecute(arrArg[1], arrArg[2]);
            }
        }

        protected override void OnPreviewDrop(DragEventArgs e)
        {
            e.Handled = true;
            var fileNames = e.Data.GetData(DataFormats.FileDrop, true) as string[];
            AddFileCommand.Execute(fileNames, this);
        }

        private void WindowContextExecute(string filePath, string fileFormat)
        {
            string strFilePath = filePath;
            string strFileFormat = fileFormat;

            // TODO : Binding으로 바꾸기
            if (strFileFormat == "HWP")
                IDC_SelectFormat.IDC_RadioButton_HWP.IsChecked = true;
            else if (strFileFormat == "XSLX")
                IDC_SelectFormat.IDC_RadioButton_XLSX.IsChecked = true;
            else if (strFileFormat == "PPTX")
                IDC_SelectFormat.IDC_RadioButton_PPTX.IsChecked = true;
            else if (strFileFormat == "DOCX")
                IDC_SelectFormat.IDC_RadioButton_DOCX.IsChecked = true;
            else if (strFileFormat == "IMAGE")
                IDC_SelectFormat.IDC_RadioButton_Image.IsChecked = true;

            if (strFilePath == null)
            {
                return;
            }

            GetModel().Status = AppStatus.Ready;

            string[] fileNames = { strFilePath };
            AddFileCommand.Execute(fileNames, this);
            ConvertCommand.Execute(GetModel().Docs, IDC_Button_PrimaryConvert);
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }

        #region RoutedCommand
        // 파일 변환 커맨드
        public static RoutedCommand ConvertCommand = new RoutedCommand("ConvertCommand", typeof(Button));
        // 파일 변환 중지 커맨드
        public static RoutedCommand StopCommand = new RoutedCommand("StopCommand", typeof(Button));
        // 앱 초기화 커맨드
        public static RoutedCommand ResetCommand = new RoutedCommand("ResetCommand", typeof(Button));
        // 파일 추가 커맨드
        public static RoutedCommand AddFileCommand = new RoutedCommand("AddFileCommand", typeof(Button));
        // 파일 제거 커맨드
        public static RoutedCommand RemoveFileCommand = new RoutedCommand("RemoveFileCommand", typeof(Button));

        private void OnOpen(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog
            {
                Multiselect = true
            };
            bool? result = dialog.ShowDialog();
            if (result != true)
            {
                return;
            }

            AddFileCommand.Execute(dialog.FileNames, IDC_DocListBox);
        }

        private void CanOpen(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().Status == AppStatus.Init || GetModel().Status == AppStatus.Ready)
            {
                e.CanExecute = true;
            }
            else
            {
                e.CanExecute = false;
            }
        }

        private void OnConvert(object sender, ExecutedRoutedEventArgs e)
        {
            pdfToOffice = new PdfToOfficeProxy();

            ErrorStatus result = pdfToOffice.InitializeSolidFramework();
            if (result != ErrorStatus.Success)
            {
                if (GetModel().ShowMsg)
                {
                    string strErr = string.Format("Error Code : {0}", result.ToString());
                    MessageBox.Show(strErr);
                    return;
                }
            }

            GetModel().Status = AppStatus.Running;
            // TODO : 변환 진행 창으로 변경 (SelectFormat 없애고 DocList 확장)
            worker.RunWorkerAsync(GetModel().Docs);
        }

        private void CanConvert(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().Status == AppStatus.Ready)
            {
                e.CanExecute = true;
                return;
            }

            e.CanExecute = false;
        }

        private void OnStop(object sender, ExecutedRoutedEventArgs e)
        {
            if (worker != null && worker.IsBusy)
            {
                worker.CancelAsync();
            }
        }

        private void CanStop(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().Status == AppStatus.Running)
            {
                e.CanExecute = true;
                return;
            }

            e.CanExecute = false;
        }

        private void OnReset(object sender, ExecutedRoutedEventArgs e)
        {
            GetModel().Docs.Clear();
        }

        private void CanReset(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().Docs.Count > 0 && worker != null && !worker.IsBusy)
            {
                e.CanExecute = true;
                return;
            }

            e.CanExecute = false;
        }

        // 파일 추가
        private void OnAddFile(object sender, ExecutedRoutedEventArgs e)
        {
            if (!(e.Parameter is string[] fileNames))
            {
                return;
            }

            foreach (var file in fileNames)
            {
                Doc doc = new Doc(file);
                doc.Index = GetModel().Docs.Count;
                GetModel().Docs.Add(doc);
            }
        }

        private void CanAddFile(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().Status == AppStatus.Init || GetModel().Status == AppStatus.Ready)
            {
                e.CanExecute = true;
            }
            else
            {
                e.CanExecute = false;
            }
        }

        // 파일 제거
        private void OnRemoveFile(object sender, ExecutedRoutedEventArgs e)
        {
            foreach (Doc doc in GetModel().Docs.ListSelectedItems)
            {
                GetModel().Docs.Remove(doc);
            }
        }

        private void CanRemoveFile(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().Status == AppStatus.Ready)
            {
                if (GetModel().Docs.ListSelectedItems != null && GetModel().Docs.ListSelectedItems.Count > 0)
                {
                    e.CanExecute = true;
                    return;
                }
            }

            e.CanExecute = false;
        }
        #endregion
    }
}
