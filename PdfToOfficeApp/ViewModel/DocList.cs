using System.Collections;
using System.Collections.ObjectModel;

namespace PdfToOfficeApp
{
    public class DocList : ObservableCollection<Doc>
    {
        private IList listSelectedItems;
        public DocList()
        {
        }

        public IList ListSelectedItems
        {
            get { return listSelectedItems; }
            set { listSelectedItems = value; }
        }
    }
}
