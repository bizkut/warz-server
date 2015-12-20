using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;
using System.IO;
using System.IO.Compression;

public partial class api_SrvUploadLogFile : WOApiWebPage
{
    public string PICTURES_DIR = "C:\\inetpub\\wwwroot\\WarZ\\intrawz\\pictures";
    public string CRASH_DIR = "C:\\inetpub\\WZ_ServerCrashes";

    public class CAttachedFile
    {
        public string FileName = "";
        public byte[] data = null;

        public bool IsValid()
        {
            return data != null;
        }

        public CAttachedFile(WOApiWebPage page, string file, bool compressed)
        {
            string str1 = file + "File";
            string str2 = file + "Size";

            HttpPostedFile httpFile = page.Request.Files[str1];
            if (httpFile == null)
                return;

            FileName = httpFile.FileName;

            int httpSize = Convert.ToInt32(page.web.Param(str2));
            data = new Byte[httpSize];

            if (compressed)
            {
                GZipStream GZip = new GZipStream(httpFile.InputStream, CompressionMode.Decompress);
                GZip.Read(data, 0, httpSize);
                GZip.Close();
            }
            else
            {
                using (BinaryReader br = new BinaryReader(httpFile.InputStream))
                {
                    data = br.ReadBytes(httpSize);
                }
            }
        }
    };

    public static void CopyStream(Stream input, Stream output) 
    { 
        byte[] buffer = new byte[32768]; 
        int read; 
        while ((read = input.Read(buffer, 0, buffer.Length)) > 0) 
        { 
            output.Write(buffer, 0, read); 
        } 
    }
    
    void UploadDump(CAttachedFile logFile, CAttachedFile dumpFile)
    {
        // save log there
        string fname = string.Format("{0}\\{1}", CRASH_DIR, logFile.FileName);
        FileStream fs = new FileStream(fname, System.IO.FileMode.Create, System.IO.FileAccess.Write);
        fs.Write(logFile.data, 0, logFile.data.Length);
        fs.Close();

        // save dump there
        fname = string.Format("{0}\\{1}", CRASH_DIR, dumpFile.FileName);
        fs = new FileStream(fname, System.IO.FileMode.Create, System.IO.FileAccess.Write);
        fs.Write(dumpFile.data, 0, dumpFile.data.Length);
        fs.Close();
    }

    void UploadScreenshot(CAttachedFile jpgFile)
    {
        // fname should be in form JPG_ServerID_CustomerID_CharID_6497d172.jpg
        string[] parts = jpgFile.FileName.Split('_');
        int ServerID = Convert.ToInt32(parts[1]);
        int CustomerID = Convert.ToInt32(parts[2]);
        int CharID = Convert.ToInt32(parts[3]);

        // create directory based on customer id
        string dir = string.Format("{0}\\{1}", PICTURES_DIR, CustomerID);
        System.IO.Directory.CreateDirectory(dir);

        // save jpg there
        string fname = string.Format("{0}\\{1}_{2}_{3}_{4}-{5:00}{6:00}-{7:00}{8:00}.jpg",
            dir, 
            CustomerID, CharID, ServerID,
            DateTime.Now.Year, DateTime.Now.Month, DateTime.Now.Day,
            DateTime.Now.Hour, DateTime.Now.Minute);

        FileStream fs = new FileStream(fname, System.IO.FileMode.Create, System.IO.FileAccess.Write);
        fs.Write(jpgFile.data, 0, jpgFile.data.Length);
        fs.Close();
    }

    protected override void Execute()
    {
        string skey1 = web.Param("skey1");
        if (skey1 != SERVER_API_KEY)
            throw new ApiExitException("bad key");

        CAttachedFile lfile = new CAttachedFile(this, "log", true);
        CAttachedFile dfile = new CAttachedFile(this, "dmp", true);
        CAttachedFile jfile = new CAttachedFile(this, "jpg", false);

        if (lfile.IsValid() && dfile.IsValid())
        {
            UploadDump(lfile, dfile);
        }

        if (jfile.IsValid())
        {
            UploadScreenshot(jfile);
        }
        else if(!lfile.IsValid())
        {
            // no log uploading yet
            throw new ApiExitException("no log/dump files");
        }

        Response.Write("WO_0");
    }
}
