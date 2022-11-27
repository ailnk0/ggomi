using System.Windows;
using System.Windows.Input;

namespace GgomiLab
{
    public class CommandUtil
    {
        public static void AddCommandHandler(UIElement ui, RoutedCommand command, ExecutedRoutedEventHandler execute, CanExecuteRoutedEventHandler canExecute)
        {
            if (ui == null)
            {
                return;
            }

            ui.CommandBindings.Add(new CommandBinding(command, new ExecutedRoutedEventHandler(execute), new CanExecuteRoutedEventHandler(canExecute)));
        }
    }
}
