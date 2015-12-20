using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;
using System.Configuration;

public partial class api_AccCheckKey : WOApiWebPage
{
    protected override void Execute()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "[BreezeNet].[dbo].BN_WarZ_SerialCheck";
        sqcmd.Parameters.AddWithValue("@in_EMail", web.Param("email"));
        sqcmd.Parameters.AddWithValue("@in_SerialKey", web.Param("serial"));

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int CheckCode  = getInt("CheckCode");
        int SerialType = getInt("SerialType");
        string CheckMsg = getString("CheckMsg");

        Response.Write("WO_0");
        Response.Write(string.Format("{0} {1} :{2}", CheckCode, SerialType, CheckMsg));
    }
}
