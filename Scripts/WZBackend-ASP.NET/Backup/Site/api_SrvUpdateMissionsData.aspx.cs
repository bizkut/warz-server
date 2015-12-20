using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_SrvUpdateMissionsData : WOApiWebPage
{
    protected override void Execute()
    {
        //if (!WoCheckLoginSession()) -- this is server only data, shouldn't affect clients at all
        //    return;

        string skey1 = web.Param("skey1");
        if (skey1 != SERVER_API_KEY)
            throw new ApiExitException("bad key");

        string MissionsData = web.Param("md");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_UserUpdateMissionsData";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_MissionsData", MissionsData);

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }
}
