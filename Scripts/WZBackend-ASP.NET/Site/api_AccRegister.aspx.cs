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

public partial class api_AccRegister : WOApiWebPage
{
    bool CheckIfHaveSteamAccount(string SteamID)
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SteamCheckAccount";
        sqcmd.Parameters.AddWithValue("@in_SteamID", SteamID);

        if (!CallWOApi(sqcmd))
            throw new ApiExitException("can't check for steam userid");

        reader.Read();
        int CustomerID = getInt("CustomerID");

        if (CustomerID > 0)
            return true;

        return false;
    }

    protected override void Execute()
    {
        string SerialKey = web.Param("serial");
        int ReferralID = 0;

        // if we got steam ticket, auth it and link it to account
        string SteamID = "";
        try
        {
            string ticket = web.Param("ticket");

            // get steam user id from auth ticket
            SteamApi api = new SteamApi();
            SteamID = api.GetSteamId(ticket);
            if (SteamID.Length == 0)
            {
                Response.Write("WO_5");
                Response.Write("steam: can't auth");
                return;
            }

            bool Have_Game = api.CheckAppOwnership(SteamID, "226700"); // base game
            if (!Have_Game)
            {
                Response.Write("WO_5");
                Response.Write("steam: does not own game");
                return;
            }

            // check if we already have linked account (to prevent multiple accounts registering)
            if(CheckIfHaveSteamAccount(SteamID))
            {
                Response.Write("WO_7");
                Response.Write("steam: already have account");
                return;
            }

            // 227662 DLC is a special DLC that says that user bought game from Steam and not converted user
            bool Have_SteamKey = api.CheckAppOwnership(SteamID, "227662");
            if (!Have_SteamKey)
            {
                Response.Write("WO_B");
                Response.Write("steam: does not own 227662");
                return;
            }

            // overwrite serial key with special key for steam
            SerialKey  = "STEAM-226700-FGHYT-AWRTS-HZRTA";
            ReferralID = 10;
        }
        catch { }


        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ACCOUNT_CREATE";
        sqcmd.Parameters.AddWithValue("@in_IP", LastIP);
        sqcmd.Parameters.AddWithValue("@in_EMail", web.Param("username")); // login name from updater
        sqcmd.Parameters.AddWithValue("@in_Password", web.Param("password"));
        sqcmd.Parameters.AddWithValue("@in_ReferralID", ReferralID);
        sqcmd.Parameters.AddWithValue("@in_SerialKey", SerialKey);
        sqcmd.Parameters.AddWithValue("@in_SerialEmail", web.Param("email"));   // email used in serial key purchase

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int CustomerID = getInt("CustomerID");

        Response.Write("WO_0");
        Response.Write(string.Format("{0}", CustomerID));

        // if we got steam id - link it to customer id
        if (SteamID.Length > 0)
        {
            sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = "WZ_SteamLinkAccount";
            sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
            sqcmd.Parameters.AddWithValue("@in_SteamID", SteamID);

            CallWOApi(sqcmd);
        }
    }
}
