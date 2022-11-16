using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace GgomiLab
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static readonly DependencyProperty StatusProperty =
            DependencyProperty.Register("Status", typeof(AppStatus), typeof(MainWindow), new PropertyMetadata(AppStatus.Init));

        public AppStatus Status
        {
            get => (AppStatus)GetValue(StatusProperty);
            set => SetValue(StatusProperty, value);
        }

        private IList<Doc> DocListData;

        public MainWindow()
        {
            InitializeComponent();
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);

            DocListData = Resources["doclistbox-data"] as IList<Doc>;

            AddCommandHandlers(ApplicationCommands.Open, OnOpen, CanOpen);
            AddCommandHandlers(Convert, OnConvert, CanConvert);
            AddCommandHandlers(AddDoc, OnAddDoc, CanAddDoc);
        }

        protected override void OnPreviewDrop(DragEventArgs e)
        {
            e.Handled = true;
            var fileNames = e.Data.GetData(DataFormats.FileDrop, true) as string[];
            AddDoc.Execute(fileNames, this);
        }

        public static RoutedCommand AddDoc = new RoutedCommand("AddDoc", typeof(DocListBox));

        private void OnAddDoc(object sender, ExecutedRoutedEventArgs e)
        {
            var fileNames = e.Parameter as string[];
            if (fileNames == null)
            {
                return;
            }

            var itemsSource = DocListData as ICollection<Doc>;
            foreach (var file in fileNames)
            {
                itemsSource.Add(new Doc(file));
            }
        }

        private void CanAddDoc(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        public static RoutedCommand Convert = new RoutedCommand("Convert", typeof(Button));

        private void OnOpen(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog();
            dialog.Multiselect = true;
            bool? result = dialog.ShowDialog();
            if (result != true)
            {
                return;
            }

            AddDoc.Execute(dialog.FileNames, IDC_DocList);
        }

        private void CanOpen(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        private void OnConvert(object sender, ExecutedRoutedEventArgs e)
        {
            var appModule = new PdfToOfficeAppModule.PdfToOfficeAppModule();

            int code = appModule.RunSample();

            bool? showMessage = e.Parameter as bool?;
            if (showMessage != false)
            {
                MessageBox.Show(string.Format("[{0}] {1}", code, Utils.GetErrorMessage(code)));
            }
        }

        private void CanConvert(object sender, CanExecuteRoutedEventArgs e)
        {
            if (Status == AppStatus.Init)
            {
                e.CanExecute = false;
                return;
            }

            e.CanExecute = true;
        }

        public void AddCommandHandlers(RoutedCommand command, ExecutedRoutedEventHandler execute, CanExecuteRoutedEventHandler canExecute)
        {
            CommandBindings.Add(
                new CommandBinding(command,
                    new ExecutedRoutedEventHandler(execute),
                    new CanExecuteRoutedEventHandler(canExecute)));
        }
    }
}
