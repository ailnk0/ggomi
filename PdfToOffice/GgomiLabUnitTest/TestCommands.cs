using GgomiLab;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace GgomiLabUnitTest
{
    [TestClass]
    public class TestCommands
    {
        [TestMethod]
        public void TestAddDoc()
        {
            DocListBox docListBox = new DocListBox();
            docListBox.BeginInit();
            docListBox.ItemsSource = new ObservableCollection<Doc>();
            docListBox.EndInit();

            string[] fileNames = { "a.pdf", "b.pdf", "c.pdf" };
            DocListBox.AddDoc.Execute(fileNames, docListBox);

            var items = docListBox.ItemsSource as IList<Doc>;
            Assert.AreEqual(fileNames.Length, items.Count);
            for (int i = 0; i < fileNames.Length; i++)
            {
                Assert.AreEqual(fileNames[i], items[i].FilePath);
            }
        }
    }
}
