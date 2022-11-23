using GgomiLab;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;

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

        [TestMethod]
        public void TestConvert()
        {
            MainWindow mw = new MainWindow();

            Assert.AreEqual(false, MainWindow.Convert.CanExecute(null, mw));

            var DocListData = mw.DataContext as IList<Doc>;
            DocListData.Add(new Doc("a.pdf"));
            DocListData.Add(new Doc("b.pdf"));
            DocListData.Add(new Doc("c.pdf"));

            Assert.AreEqual(true, MainWindow.Convert.CanExecute(null, mw));

            MainWindow.Convert.Execute(false, mw);

            string convertedDocPath = "../sample/HOffice2022_Brochure_KR.docx";
            bool isConvertSuccess = File.Exists(convertedDocPath);
            if (isConvertSuccess)
            {
                File.Delete(convertedDocPath);
            }

            Assert.AreEqual(true, isConvertSuccess);
        }
    }
}
