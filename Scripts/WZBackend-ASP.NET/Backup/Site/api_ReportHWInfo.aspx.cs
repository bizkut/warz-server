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

public partial class api_ReportHWInfo : WOApiWebPage
{
    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ReportHWInfo";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@r00", web.Param("r00"));
        sqcmd.Parameters.AddWithValue("@r10", web.Param("r10"));
        sqcmd.Parameters.AddWithValue("@r11", web.Param("r11"));
        sqcmd.Parameters.AddWithValue("@r12", web.Param("r12"));
        sqcmd.Parameters.AddWithValue("@r13", web.Param("r13"));
        sqcmd.Parameters.AddWithValue("@r20", web.Param("r20"));
        sqcmd.Parameters.AddWithValue("@r21", web.Param("r21"));
        sqcmd.Parameters.AddWithValue("@r22", web.Param("r22"));
        sqcmd.Parameters.AddWithValue("@r23", web.Param("r23"));
        sqcmd.Parameters.AddWithValue("@r24", web.Param("r24"));
        sqcmd.Parameters.AddWithValue("@r25", web.Param("r25"));
        sqcmd.Parameters.AddWithValue("@r26", web.Param("r26"));
        sqcmd.Parameters.AddWithValue("@r30", web.Param("r30"));

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }
}
