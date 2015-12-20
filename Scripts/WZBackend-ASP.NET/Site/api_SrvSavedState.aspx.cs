using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_SrvSavedState : WOApiWebPage
{
    static byte[] GetBytes(string str)
    {
        //byte[] bytes = new byte[str.Length * sizeof(char)];
        //System.Buffer.BlockCopy(str.ToCharArray(), 0, bytes, 0, bytes.Length);

        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(str);
        return bytes;
    }

    static string GetString(byte[] bytes)
    {
        //char[] chars = new char[bytes.Length / sizeof(char)];
        //System.Buffer.BlockCopy(bytes, 0, chars, 0, bytes.Length);
        //return new string(chars);

        return System.Text.Encoding.UTF8.GetString(bytes);
    }

    void GetSavedState()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_SavedStateGet";
        sqcmd.Parameters.AddWithValue("@in_GameServerID", web.Param("GameSID"));

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        GResponse.Write("WO_0");
        if (getInt("HaveData") == 0)
        {
            // return empty xml
            GResponse.Write("<?xml version=\"1.0\"?>");
            return;
        }

        GResponse.Write(GetString(reader["SavedState"] as byte[]));
    }

    void SetSavedState()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_SavedStateSet";
        sqcmd.Parameters.AddWithValue("@in_GameServerID", web.Param("GameSID"));
        sqcmd.Parameters.AddWithValue("@in_SavedState", GetBytes(web.Param("state")));

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    protected override void Execute()
    {
        string skey1 = web.Param("skey1");
        if (skey1 != SERVER_API_KEY)
            throw new ApiExitException("bad key");

        string func = web.Param("func");
        if (func == "get")
            GetSavedState();
        else if (func == "set")
            SetSavedState();
        else
            throw new ApiExitException("bad func");
    }
}
