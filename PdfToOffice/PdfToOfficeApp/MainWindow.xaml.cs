using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace PdfToOfficeApp
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        private static MainWindow instance;
        public MainViewModel vm;
        private ICollection<Doc> DocList_ItemsSource;
        public MainWindow()
        {
            InitializeComponent();
            instance = this;

            vm = new MainViewModel();
            this.DataContext = vm;
            if (vm.CloseAction == null)
                vm.CloseAction = new Action(() => this.Close());

            DocList_ItemsSource = IDC_DocList.ItemsSource as ICollection<Doc>;
        }

        public static MainWindow GetInstance()
        {
            return instance;
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);

            AddCommandHandlers(ApplicationCommands.Open, OnOpen, CanOpen);
            AddCommandHandlers(AddFileCommand, OnAddFile, CanAddFile);
            AddCommandHandlers(RemoveFileCommand, OnRemoveFile, CanRemoveFile);
            AddCommandHandlers(OpenFolderCommand, OnOpenFolder, CanOpenFolder);
            AddCommandHandlers(ConvertCommand, OnConvert, CanConvert);
            AddCommandHandlers(CancelCommand, OnCancel, CanCancel);
        }

        #region RoutedCommand
        // 파일 변환 커맨드
        public static RoutedCommand ConvertCommand = new RoutedCommand("ConvertCommand", typeof(Button));
        // 파일 추가 커맨드
        public static RoutedCommand AddFileCommand = new RoutedCommand("AddFileCommand", typeof(Button));
        // 파일 제거 커맨드
        public static RoutedCommand RemoveFileCommand = new RoutedCommand("RemoveFileCommand", typeof(Button));
        // 저장 경로 설정 커맨드
        public static RoutedCommand OpenFolderCommand = new RoutedCommand("OpenFolderCommand", typeof(Button));
        // 취소 커맨드
        public static RoutedCommand CancelCommand = new RoutedCommand("CancelCommand", typeof(Button));

        private void OnOpen(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog();
            dialog.Multiselect = true;
            bool? result = dialog.ShowDialog();
            if (result != true)
            {
                return;
            }

            DocListBox.AddDoc.Execute(dialog.FileNames, IDC_DocList);
        }

        private void CanOpen(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        private void OnConvert(object sender, ExecutedRoutedEventArgs e)
        {
            IDC_SelectFormat.Visibility = Visibility.Collapsed;
            IDC_Button_PrimaryConvert.Visibility = Visibility.Collapsed;
            IDC_Button_PrimaryPause.Visibility = Visibility.Visible;

            foreach (Doc doc in DocList_ItemsSource)
            {
                string path = doc.FilePath;
                int resultCode = vm.RunSample(path);
                if (resultCode != 0)
                {
                    MessageBox.Show(Utils.GetString("IDS_Msg_Failed"));
                }
            }

            IDC_Button_PrimaryPause.Visibility = Visibility.Collapsed;
            IDC_Button_PrimaryReset.Visibility = Visibility.Visible;
            MessageBox.Show("Conversion is done.");
        }

        private void CanConvert(object sender, CanExecuteRoutedEventArgs e)
        {
            // 목록에 파일 있는지 확인
            if (DocList_ItemsSource.Count == 0)
            {
                e.CanExecute = false;
                IDC_Button_PrimaryAdd.Visibility = Visibility.Visible;
                IDC_SelectFormat.Visibility = Visibility.Collapsed;
                IDC_Grid_SecondaryButtons.Visibility = Visibility.Collapsed;
                return;
            }
            else
            {
                IDC_Button_PrimaryConvert.Visibility = Visibility.Visible;
                IDC_SelectFormat.Visibility = Visibility.Visible;
                IDC_Grid_SecondaryButtons.Visibility = Visibility.Visible;
                IDC_Button_PrimaryAdd.Visibility = Visibility.Collapsed;

                if (SelectFormat.bRadioSelected)
                {
                    e.CanExecute = true;
                }
            }
        }

        // 파일 추가
        private void OnAddFile(object sender, ExecutedRoutedEventArgs e)
        {

        }

        private void CanAddFile(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        // 파일 제거
        private void OnRemoveFile(object sender, ExecutedRoutedEventArgs e)
        {

        }

        private void CanRemoveFile(object sender, CanExecuteRoutedEventArgs e)
        {

        }

        // 폴더 지정
        private void OnOpenFolder(object sender, ExecutedRoutedEventArgs e)
        {
            System.Windows.Forms.FolderBrowserDialog folderBrowserDialog = new System.Windows.Forms.FolderBrowserDialog();
            if (folderBrowserDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                vm.StrSavePath = folderBrowserDialog.SelectedPath;
            }
        }

        private void CanOpenFolder(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        // 취소
        private void OnCancel(object sender, ExecutedRoutedEventArgs e)
        {
            vm.CloseAction();
        }

        private void CanCancel(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        private void AddCommandHandlers(RoutedCommand command, ExecutedRoutedEventHandler execute, CanExecuteRoutedEventHandler canExecute)
        {
            CommandBindings.Add(
                new CommandBinding(command,
                    new ExecutedRoutedEventHandler(execute),
                    new CanExecuteRoutedEventHandler(canExecute)));
        }
        #endregion
    }
}
