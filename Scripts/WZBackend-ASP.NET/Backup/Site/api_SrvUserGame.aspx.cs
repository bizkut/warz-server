using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_SrvUserGame : WOApiWebPage
{
    void JoinedGame()
    {
        // check for optional server hop position change
        string GamePos = "";
        int IsServerHop = 0;
        try
        {
            GamePos = web.Param("gSH");
            IsServerHop = 1;
        }
        catch { }

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_UserJoinedGame3";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_GameMapId", web.Param("g1"));
        sqcmd.Parameters.AddWithValue("@in_GameServerId", web.Param("g2"));
        sqcmd.Parameters.AddWithValue("@in_IsServerHop", IsServerHop);
        sqcmd.Parameters.AddWithValue("@in_GamePos", GamePos);
        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void LeftGame()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_UserLeftGame";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_GameMapId", web.Param("g1"));
        sqcmd.Parameters.AddWithValue("@in_GameServerId", web.Param("g2"));
        sqcmd.Parameters.AddWithValue("@in_TimePlayed", web.Param("s1"));
        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void PingGame()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_UserPingGame";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    protected override void Execute()
    {
        // disconnect if we don't have correct credentials
        if (!WoCheckLoginSession())
            return;

        string skey1 = web.Param("skey1");
        if (skey1 != SERVER_API_KEY)
            throw new ApiExitException("bad key");

        string func = web.Param("func");
        if (func == "join")
            JoinedGame();
        else if (func == "leave")
            LeftGame();
        else if (func == "ping")
            PingGame();
        else
            throw new ApiExitException("bad func");
    }
}
