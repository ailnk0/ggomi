using System.Collections.ObjectModel;

namespace GgomiLab
{
    public class DocList : ObservableCollection<Doc>
    {
        public DocList()
        {
            Add(new Doc("C:/First PDF.pdf"));
            Add(new Doc("C:/Second PDF.pdf"));
            Add(new Doc("C:/Third PDF.pdf"));
            Add(new Doc("C:/Fourth PDF.pdf"));
            Add(new Doc("C:/Fifth PDF.pdf"));
        }
    }
}
