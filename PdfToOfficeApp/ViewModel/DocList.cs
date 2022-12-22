using System.Collections;
using System.Collections.ObjectModel;

namespace PdfToOfficeApp
{
    public class DocList : ObservableCollection<Doc>
    {
        public DocList()
        {
           
        }

        private FileFormat _selectedFileFormat;
        public FileFormat SelectedFileFormat
        {
            get { return _selectedFileFormat; }
            set { _selectedFileFormat = value; }
        }

        private FileFormat _imageFormat;
        public FileFormat ImageFormat
        {
            get { return _imageFormat; }
            set { _imageFormat = value; }
        }
    }
}
