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

public partial class api_AccUnlock : WOApiWebPage
{
    protected override void Execute()
    {
        string token = web.Param("token", true);

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ACCOUNT_SecurityUnlock";
        sqcmd.Parameters.AddWithValue("@in_token", token);

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("<!DOCTYPE html>\n");
        Response.Write("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
        Response.Write("<head>\n");
        Response.Write("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
        Response.Write("</head>\n");
        Response.Write("<body>\n");
        Response.Write("  <h3>\n");
        Response.Write("    <font face=\"Verdana\">Infestation: Survivor Stories Account Unlock</font>\n");
        Response.Write("  </h3>\n");
        Response.Write("  Your account is unlocked.\n");
        Response.Write("</body>\n");
        Response.Write("</html>\n");
    }
}
