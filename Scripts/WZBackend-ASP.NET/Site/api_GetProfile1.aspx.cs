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

public partial class api_GetProfile1 : WOApiWebPage
{
    void AddChars(ref StringBuilder xml, bool FromServer)
    {
        // move to next select, where data should be
        reader.NextResult();

        xml.Append("<chars>\n");
        while (reader.Read())
        {
            DateTime DeathTime = Convert.ToDateTime(reader["DeathUtcTime"]);
            xml.Append("<c ");
            xml.Append(xml_attr("CharID", reader["CharID"]));
            xml.Append(xml_attr("Gamertag", reader["Gamertag"]));
            xml.Append(xml_attr("Alive", reader["Alive"]));
            xml.Append(xml_attr("DeathTime", ToUnixTime(DeathTime).ToString()));
            xml.Append(xml_attr("SecToRevive", reader["SecToRevive"]));
            xml.Append(xml_attr("SecFromLastGame", reader["SecFromLastUpdate"])); // var name Game<->Update is not a typo
            xml.Append(xml_attr("Hardcore", reader["Hardcore"]));
            xml.Append(xml_attr("XP", reader["XP"]));
            xml.Append(xml_attr("SpendXP", reader["SpendXP"]));
            xml.Append(xml_attr("Skills", reader["Skills"].ToString().TrimEnd()));
            xml.Append(xml_attr("TimePlayed", reader["TimePlayed"]));
            xml.Append(xml_attr("GameMapId", reader["GameMapId"]));
            xml.Append(xml_attr("GameServerId", reader["GameServerId"]));
            xml.Append(xml_attr("GamePos", reader["GamePos"]));
            xml.Append(xml_attr("GamePos2", reader["GamePos2"]));
            xml.Append(xml_attr("GamePos3", reader["GamePos3"]));
            xml.Append(xml_attr("GamePos4", reader["GamePos4"]));
            xml.Append(xml_attr("GamePos5", reader["GamePos5"]));
            xml.Append(xml_attr("GamePos6", reader["GamePos6"]));
            xml.Append(xml_attr("GamePos7", reader["GamePos7"]));
            xml.Append(xml_attr("GamePos8", reader["GamePos8"]));
            xml.Append(xml_attr("GameFlags", reader["GameFlags"]));
            xml.Append(xml_attr("Health", reader["Health"]));
            xml.Append(xml_attr("Hunger", reader["Food"]));
            xml.Append(xml_attr("Thirst", reader["Water"]));
            xml.Append(xml_attr("Toxic", reader["Toxic"]));
            xml.Append(xml_attr("Reputation", reader["Reputation"]));
            xml.Append(xml_attr("HeroItemID", reader["HeroItemID"]));
            xml.Append(xml_attr("HeadIdx", reader["HeadIdx"]));
            xml.Append(xml_attr("BodyIdx", reader["BodyIdx"]));
            xml.Append(xml_attr("LegsIdx", reader["LegsIdx"]));
            xml.Append(xml_attr("BackpackSize", reader["BackpackSize"]));
            xml.Append(xml_attr("BackpackID", reader["BackpackID"]));
            xml.Append(xml_attr("ClanID", reader["ClanID"]));
            xml.Append(xml_attr("ClanRank", reader["ClanRank"]));
            xml.Append(xml_attr("ClanTag", reader["ClanTag"]));
            xml.Append(xml_attr("ClanTagColor", reader["ClanTagColor"]));

            xml.Append(xml_attr("attm1", reader["Attachment1"]));
            xml.Append(xml_attr("attm2", reader["Attachment2"]));

            // generic trackable stats
            xml.Append(xml_attr("ts00", reader["Stat00"]));
            xml.Append(xml_attr("ts01", reader["Stat01"]));
            xml.Append(xml_attr("ts02", reader["Stat02"]));
            xml.Append(xml_attr("ts03", reader["Stat03"]));
            xml.Append(xml_attr("ts04", reader["Stat04"]));
            xml.Append(xml_attr("ts05", reader["Stat05"]));
            xml.Append(">");     // <c closure

            // generic char data
            string CharData = reader["CharData"].ToString();
            if (CharData.Length > 0)
            {
                xml.Append("<CharData>\n");
                xml.Append(CharData);
                xml.Append("</CharData>\n");
            }

            // missions data
            string MissionsData = reader["MissionsData"].ToString();
            if (MissionsData.Length > 0) //@TODO-FOR-DEBUG: && FromServer
            {
                xml.Append("<MissionsData>\n");
                xml.Append(MissionsData);
                xml.Append("</MissionsData>\n");
            }

            xml.Append("</c>");
        }
        xml.Append("</chars>\n");
    }

    void AddInventory(ref StringBuilder xml)
    {
        // move to next select, where data should be
        reader.NextResult();

        xml.Append("<inventory>\n");
        while (reader.Read())
        {
            Int64 InvID = Convert.ToInt64(reader["InventoryID"]);
            int ItemID = Convert.ToInt32(reader["ItemID"]);
            int Quantity = Convert.ToInt32(reader["Quantity"]);
            int Var1 = Convert.ToInt32(reader["Var1"]);
            int Var2 = Convert.ToInt32(reader["Var2"]);
            int Var3 = Convert.ToInt32(reader["Var3"]);

            xml.Append("<i ");
            xml.Append(xml_attr("id", InvID));
            xml.Append(xml_attr("itm", ItemID));
            xml.Append(xml_attr("qt", Quantity));
            if (Var1 >= 0) xml.Append(xml_attr("v1", Var1));
            if (Var2 >= 0) xml.Append(xml_attr("v2", Var2));
            if (Var3 >= 0) xml.Append(xml_attr("v3", Var3));

            xml.Append("/>");
        }
        xml.Append("</inventory>\n");
    }

    void AddBackpacks(ref StringBuilder xml)
    {
        // move to next select, where data should be
        reader.NextResult();

        int CurCharID = 0;

        xml.Append("<backpacks>\n");
        while (reader.Read())
        {
            Int64 InvID = Convert.ToInt64(reader["InventoryID"]);
            int CharID = Convert.ToInt32(reader["CharID"]);
            int ItemID = Convert.ToInt32(reader["ItemID"]);
            int Quantity = Convert.ToInt32(reader["Quantity"]);
            int Slot = Convert.ToInt32(reader["BackpackSlot"]);
            int Var1 = Convert.ToInt32(reader["Var1"]);
            int Var2 = Convert.ToInt32(reader["Var2"]);
            int Var3 = Convert.ToInt32(reader["Var3"]);

            if (CharID != CurCharID)
            {
                if (CurCharID > 0)
                {
                    xml.Append("</b>");
                }
                xml.Append(string.Format("<b CharID=\"{0}\">", CharID));
                CurCharID = CharID;
            }

            xml.Append("<i ");
            xml.Append(xml_attr("id", InvID));
            xml.Append(xml_attr("itm", ItemID));
            xml.Append(xml_attr("qt", Quantity));
            xml.Append(xml_attr("s", Slot));
            if (Var1 >= 0) xml.Append(xml_attr("v1", Var1));
            if (Var2 >= 0) xml.Append(xml_attr("v2", Var2));
            if (Var3 >= 0) xml.Append(xml_attr("v3", Var3));
            xml.Append("/>");
        }
        if(CurCharID > 0) // close last backpack
            xml.Append("</b>");

        xml.Append("</backpacks>\n");
    }

    protected override void Execute()
    {
        bool needLoginCheck = true;

        // to be able to test IIS response time in Nagios
        string nagios_server_key = web.OptionalParam("NagiosKey", true);
        if (nagios_server_key != null && nagios_server_key == "AS&Dasjkhd738rKSAHD7we5iuhfdzjscn")
            needLoginCheck = false;

        if(needLoginCheck)
            if (!WoCheckLoginSession())
                return;

        string CustomerID = web.CustomerID();

        // if we have "CharID" parameter, it means GetProfile call was called from server
        int CharID = 0;
        try
        {
            CharID = Convert.ToInt32(web.Param("CharID"));
        }
        catch { }


        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_GetAccountInfo2";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
        sqcmd.Parameters.AddWithValue("@in_CharID", CharID);

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();

        string curTime = string.Format("{0} {1} {2} {3} {4}",
            DateTime.Now.Year, DateTime.Now.Month, DateTime.Now.Day,
            DateTime.Now.Hour, DateTime.Now.Minute);

        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append("<account ");
        xml.Append(xml_attr("CustomerID", reader["CustomerID"]));
        xml.Append(xml_attr("AccountStatus", reader["AccountStatus"]));
        xml.Append(xml_attr("AccountType", reader["AccountType"]));
        xml.Append(xml_attr("GamePoints", reader["GamePoints"]));
        xml.Append(xml_attr("GameDollars", reader["GameDollars"]));
        xml.Append(xml_attr("ResWood", reader["ResWood"]));
        xml.Append(xml_attr("ResStone", reader["ResStone"]));
        xml.Append(xml_attr("ResMetal", reader["ResMetal"]));
        xml.Append(xml_attr("time", curTime));
        string IsDeveloper = reader["IsDeveloper"].ToString();
        if(IsDeveloper != "0")
            xml.Append(xml_attr("IsDeveloper", IsDeveloper));
        xml.Append(xml_attr("TimePlayed", reader["TimePlayed"]));

        int PremiumLeft = Convert.ToInt32(reader["PremiumLeft"]);
        if (PremiumLeft > 0)
            xml.Append(xml_attr("PremiumLeft", PremiumLeft));

        // special code to check if server wasn't closed successfully and account info is dirty
        string GameServerId = reader["GameServerId"].ToString();
        if (GameServerId != "" && GameServerId != "0")
        {
            // seconds while game wasn't closed successfully
            Int64 SecFromLastGame = Convert.ToInt64(reader["SecFromLastGame"]);
            // we allow 40 sec for server to finish updating things
            if(SecFromLastGame < 40)
                xml.Append(xml_attr("DataDirty", SecFromLastGame));
        }

        xml.Append(">\n");

        // all chars
        AddChars(ref xml, CharID > 0);

        // inventory
        AddInventory(ref xml);

        // all backpacks
        AddBackpacks(ref xml);

        xml.Append("</account>");

        GResponse.Write(xml.ToString());
    }
}
