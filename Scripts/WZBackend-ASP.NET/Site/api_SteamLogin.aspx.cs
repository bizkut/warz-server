using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_SteamLogin : WOApiWebPage
{
    bool ActivateDLC(int CustomerID, int AppID, string apiAnswer, string SteamID)
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SteamActivateDLC";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
        sqcmd.Parameters.AddWithValue("@in_AppID", AppID);
        sqcmd.Parameters.AddWithValue("@in_APIAnswer", apiAnswer);
        sqcmd.Parameters.AddWithValue("@in_AuthSteamUserID", SteamID);
        
        if (!CallWOApi(sqcmd))
            return true;

        reader.Read();
        string DLCResult = getString("DLCResult");
        if (DLCResult.Length > 0)
        {
            Response.Write("DLC:" + DLCResult);
            return true;
        }

        // already own it
        return false;
    }

    protected override void Execute()
    {
        string ticket = web.Param("ticket");

        // get steam user id from auth ticket
        SteamApi api = new SteamApi();
        string SteamID = api.GetSteamId(ticket);
        if(SteamID.Length == 0)
        {
            Response.Write("WO_5");
            Response.Write(api.lastData_);
            return;
        }

        string apiAnswer = "";
        bool Have_Game = false;
        int Have_227660 = 0; // old 15$
        int Have_227661 = 0; // old 50$
        int Have_267590 = 0; // 12-2013 15$
        int Have_267591 = 0; // 12-2013 50$
        SteamXML.appownershipAppsApp[] apps = api.GetPublisherAppOwnership(SteamID, ref apiAnswer);
        if (apps != null)
        {
            foreach (SteamXML.appownershipAppsApp app in apps)
            {
                if (app.ownsapp == false)
                    continue;
                if (app.permanent == false && SteamID != "76561198043252597") // ignore that flag for denis steam acc (i have game as developer)
                    continue;
                switch (app.appid)
                {
                    case 226700: Have_Game = true; break;  // base game
                    case 227660: Have_227660 = app.appid; break; // 15$
                    case 227661: Have_227661 = app.appid; break; // 50$
                    case 267590: Have_267590 = app.appid; break; // new 15$
                    case 267591: Have_267591 = app.appid; break; // new 50$
                }
            }
        }

        if (!Have_Game)
        {
            // special 1001 code for running under steam but without game.
            Response.Write("WO_0");
            Response.Write(string.Format("{0} {1} {2}",
                0, 0, 1001));
            return;
        }

        string countryIP = "";
        if (!String.IsNullOrEmpty(Request["HTTP_CF_IPCOUNTRY"]))
            countryIP = Request["HTTP_CF_IPCOUNTRY"];            

        // try to login user based on his steam id
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SteamLogin";
        sqcmd.Parameters.AddWithValue("@in_IP", LastIP);
        sqcmd.Parameters.AddWithValue("@in_SteamID", SteamID);
        sqcmd.Parameters.AddWithValue("@in_Country", countryIP);

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int CustomerID = getInt("CustomerID");
        int AccountStatus = getInt("AccountStatus");
        int SessionID = 0;
        int IsDeveloper = 0;

        if (CustomerID > 0)
        {
            SessionID = getInt("SessionID");
            IsDeveloper = getInt("IsDeveloper");
        }

        Response.Write("WO_0");
        Response.Write(string.Format("{0} {1} {2}",
            CustomerID, SessionID, AccountStatus));

        // DLC logic
        // because of totally retarded steam setup our bundles will look like this:
        //  base: 226700
        //  15$:  226700 & 227660
        //  50$:  226700 & 227660 & 227661

        // activate 1st DLC ONLY if we don't have second
        if (CustomerID > 0 && Have_227660 > 0 && Have_227661 == 0)
        {
            if (ActivateDLC(CustomerID, Have_227660, apiAnswer, SteamID))
                return;
        }
        if (CustomerID > 0 && Have_227660 > 0 && Have_227661 > 0)
        {
            if (ActivateDLC(CustomerID, Have_227661, apiAnswer, SteamID))
                return;
        }

        // NEW 12-2013 packages
        // activate 1st DLC ONLY if we don't have second
        if (CustomerID > 0 && Have_267590 > 0 && Have_267591 == 0)
        {
            if (ActivateDLC(CustomerID, Have_267590, apiAnswer, SteamID))
                return;
        }

        if (CustomerID > 0 && Have_267590 > 0 && Have_267591 > 0)
        {
            if (ActivateDLC(CustomerID, Have_267591, apiAnswer, SteamID))
                return;
        }

        return;
    }
}
