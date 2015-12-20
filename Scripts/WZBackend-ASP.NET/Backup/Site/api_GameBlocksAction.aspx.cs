using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_GameBlocksAction : WOApiWebPage
{
    protected override void Execute()
    {
        if (LastIP != "67.86.56.36" && LastIP != "188.122.92.32" && LastIP != "188.122.92.31" && LastIP != "188.122.92.43") // gameblocks IP address (dev + production servers)
            throw new ApiExitException("bad ip: "+LastIP);

        string skey1 = web.Param("gbkey1", true);
        if (skey1 != "4eO66{%*c_mmpU,)9L.VTIq#t6ZFA(YE")
            throw new ApiExitException("bad key");

        //throw new ApiExitException("disabled");

        string customerID = web.Param("CustomerID", true);

        string action = web.Param("Action", true);
        if (action == "unban")
        {
            SqlCommand sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = "ADMIN_UnbanUser";
            sqcmd.Parameters.AddWithValue("@in_CustomerID", customerID);
            sqcmd.Parameters.AddWithValue("@in_UnbanReason", "GameBlocks: " + web.Param("UnbanReason", true));
          if (!CallWOApi(sqcmd))
                return;

            Response.Write("WO_0");
        }
        else if (action == "ban")
        {
            SqlCommand sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = "ADMIN_BanUser";
            sqcmd.Parameters.AddWithValue("@in_CustomerID", customerID);
            sqcmd.Parameters.AddWithValue("@in_BanReason", "GameBlocks: "+web.Param("BanReason", true));
            sqcmd.Parameters.AddWithValue("@in_OptPermaBan", 1);
            sqcmd.Parameters.AddWithValue("@in_OptBanFromGameblocks", 1);
            if (!CallWOApi(sqcmd))
                return;

            Response.Write("WO_0");
        }
        else
            throw new ApiExitException("no action");
    }
}
