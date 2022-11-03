using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using PdfToOfficeApp;

namespace PdfToOfficeUnitTest
{
    [TestClass]
    public class PdfToOfficeUnitTest
    {
        [TestMethod]
        public void TestMethod1()
        {
            PdfToOffice pdfToOffice = new PdfToOffice();
            int result = pdfToOffice.RunSample();

            Assert.AreEqual(0, result);
        }
    }
}
