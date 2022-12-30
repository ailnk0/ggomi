using System;
using System.ComponentModel;
using System.Reflection;
using System.Text;
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
            worker.WorkerReportsProgress = true;
            worker.WorkerSupportsCancellation = true;

            pdfToOffice = new PdfToOfficeProxy();

            GetModel().AppVersion = Assembly.GetExecutingAssembly().GetName().Version.ToString();
            GetModel().SaveDir = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
        }

        private void MainWindow_ContentRendered(object sender, EventArgs e)
        {
            string[] arrArg = Environment.GetCommandLineArgs();
            if (arrArg.Length >= 3)
            {
                WindowContextExecute(arrArg[1], arrArg[2]);
            }
        }

        private void WindowContextExecute(string filePath, string fileFormat)
        {
            string strFilePath = filePath;
            string strFileType = fileFormat.ToLower();

            if (strFileType == "xlsx")
            {
                GetModel().ConvFileType = FILE_TYPE.XLSX;
            }
            else if (strFileType == "pptx")
            {
                GetModel().ConvFileType = FILE_TYPE.PPTX;
            }
            else if (strFileType == "docx")
            {
                GetModel().ConvFileType = FILE_TYPE.DOCX;
            }
            else if (strFileType == "png")
            {
                GetModel().ConvFileType = FILE_TYPE.IMAGE;
                GetModel().ConvImgType = IMG_TYPE.PNG;
            }
            else if (strFileType == "jpg" || strFileType == "jpeg")
            {
                GetModel().ConvFileType = FILE_TYPE.IMAGE;
                GetModel().ConvImgType = IMG_TYPE.JPEG;
            }
            else if (strFileType == "gif")
            {
                GetModel().ConvFileType = FILE_TYPE.IMAGE;
                GetModel().ConvImgType = IMG_TYPE.GIF;
            }
            else
            {
                Console.WriteLine(Util.String.GetMsg(RES_CODE.PdfAError));
                return;
            }

            if (strFilePath == null)
            {
                Console.WriteLine(Util.String.GetMsg(RES_CODE.IOError));
                return;
            }

            GetModel().AppStatus = APP_STATUS.READY;

            string[] fileNames = { strFilePath };

            AddFileCommand.Execute(fileNames, this);
            ConvertCommand.Execute(null, this);
        }

        private void Worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            GetModel().AppStatus = APP_STATUS.COMPLETED;
        }

        private void Worker_DoWork(object sender, DoWorkEventArgs e)
        {
            MainViewModel model = (MainViewModel)e.Argument;

            foreach (Doc doc in model.Docs)
            {
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                    return;
                }

                if (doc.ConvStatus != CONV_STATUS.READY)
                {
                    continue;
                }

                doc.ConvStatus = CONV_STATUS.RUNNING;

                ProgressSiteCli progressSiteCli = new ProgressSiteCli(doc);
                pdfToOffice.SetProgressSiteCli(progressSiteCli);
                doc.ResCode = pdfToOffice.DoConversion(doc.FilePath, doc.Password, model.ConvFileType, model.ConvImgType, model.AllowOverwrite);

                if (doc.ResCode == RES_CODE.Success)
                {
                    doc.ConvStatus = CONV_STATUS.COMPLETED;
                }
                else
                {
                    doc.ConvStatus = CONV_STATUS.FAIL;

                    StringBuilder msg = new StringBuilder();
                    msg.AppendLine(doc.FilePath);
                    msg.AppendLine();
                    msg.AppendFormat("⚠ {0}", Util.String.GetMsg(doc.ResCode));
                    doc.Tooltip = msg.ToString();
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
            Util.Commands.Add(this, AboutCommand, OnAbout, CanAbout);
            Util.Commands.Add(this, ConfigCommand, OnConfig, CanConfig);
        }

        protected override void OnPreviewDrop(DragEventArgs e)
        {
            e.Handled = true;
            var fileNames = e.Data.GetData(DataFormats.FileDrop, true) as string[];
            AddFileCommand.Execute(fileNames, this);
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
        public static RoutedCommand AddFileCommand = new RoutedCommand("AddFileCommand", typeof(MenuItem));
        // 파일 제거 커맨드
        public static RoutedCommand RemoveFileCommand = new RoutedCommand("RemoveFileCommand", typeof(MenuItem));
        // 설정 커맨드
        public static RoutedCommand ConfigCommand = new RoutedCommand("ConfigCommand", typeof(MenuItem));
        // 정보 커맨드
        public static RoutedCommand AboutCommand = new RoutedCommand("AboutCommand", typeof(MenuItem));

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

            AddFileCommand.Execute(dialog.FileNames, this);
        }

        private void CanOpen(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().AppStatus == APP_STATUS.INIT || GetModel().AppStatus == APP_STATUS.READY)
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
            RES_CODE resCode = pdfToOffice.InitializeSolidFramework();
            if (resCode != RES_CODE.Success)
            {
                if (GetModel().ShowMsg)
                {
                    MessageBox.Show(Util.String.GetMsg(resCode));
                    return;
                }
            }

            GetModel().AppStatus = APP_STATUS.RUNNING;

            worker.RunWorkerAsync(GetModel());
        }

        private void CanConvert(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().AppStatus == APP_STATUS.READY)
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
            if (GetModel().AppStatus == APP_STATUS.RUNNING)
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
            e.CanExecute = true;
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
                GetModel().Docs.Add(doc);
            }
        }

        private void CanAddFile(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().AppStatus == APP_STATUS.INIT || GetModel().AppStatus == APP_STATUS.READY)
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
            // Keep selected items as temp
            Doc[] temp = new Doc[GetModel().SelectedItems.Count];
            GetModel().SelectedItems.CopyTo(temp, 0);

            // Unselect all to sync list and selected items after remove
            GetModel().SelectedItems.Clear();

            // Remove selected items
            foreach (Doc doc in temp)
            {
                GetModel().Docs.Remove(doc);
            }
        }

        private void CanRemoveFile(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().AppStatus == APP_STATUS.READY)
            {
                if (GetModel().SelectedItems != null && GetModel().SelectedItems.Count > 0)
                {
                    e.CanExecute = true;
                    return;
                }
            }

            e.CanExecute = false;
        }

        private void OnAbout(object sender, ExecutedRoutedEventArgs e)
        {
            AboutWindow aboutWin = new AboutWindow();
            aboutWin.DataContext = GetModel();
            aboutWin.ShowDialog();
        }

        private void CanAbout(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        private void OnConfig(object sender, ExecutedRoutedEventArgs e)
        {
            ConfigWindow confWin = new ConfigWindow();
            confWin.DataContext = GetModel();
            confWin.ShowDialog();
        }

        private void CanConfig(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().AppStatus != APP_STATUS.RUNNING)
            {
                e.CanExecute = true;
            }
            else
            {
                e.CanExecute = false;
            }
        }
        #endregion
    }
}
