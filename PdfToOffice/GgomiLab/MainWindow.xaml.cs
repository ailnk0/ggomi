using Microsoft.Win32;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Input;

namespace GgomiLab
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            AddCommandBindings(ApplicationCommands.Open, OpenCommandHandler);
        }

        private void OpenCommandHandler(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog();
            bool? result = dialog.ShowDialog();
            if (result == true)
            {
                string fileName = dialog.FileName;
                ICollection<Doc> itemsSource = (ICollection<Doc>)IDC_DocList.ItemsSource;
                if (itemsSource != null)
                {
                    itemsSource.Add(new Doc(fileName));
                }
            }
        }

        private void AddCommandBindings(ICommand command, ExecutedRoutedEventHandler handler)
        {
            var commandBinding = new CommandBinding(command, handler);
            CommandBindings.Add(commandBinding);
        }
    }
}
