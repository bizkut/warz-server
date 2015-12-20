using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_ClanGetInfo : WOApiWebPage
{
    public static void OutClanInfo(StringBuilder xml, SqlDataReader reader)
    {
        xml.Append("<clan ");
        xml.Append(xml_attr("ClanID", reader["ClanID"]));
        xml.Append(xml_attr("ClanNameColor", reader["ClanNameColor"]));
        xml.Append(xml_attr("ClanTagColor", reader["ClanTagColor"]));
        xml.Append(xml_attr("ClanEmblemID", reader["ClanEmblemID"]));
        xml.Append(xml_attr("ClanEmblemColor", reader["ClanEmblemColor"]));
        xml.Append(xml_attr("ClanXP", reader["ClanXP"]));
        xml.Append(xml_attr("ClanLevel", reader["ClanLevel"]));
        xml.Append(xml_attr("ClanGP", reader["ClanGP"]));
        xml.Append(xml_attr("NumClanMembers", reader["NumClanMembers"]));
        xml.Append(xml_attr("MaxClanMembers", reader["MaxClanMembers"]));
        xml.Append(xml_attr("ClanName", reader["ClanName"]));
        xml.Append(xml_attr("ClanTag", reader["ClanTag"]));
        xml.Append(xml_attr("ClanLore", reader["ClanLore"]));
        xml.Append(xml_attr("OwnerGamertag", reader["gamertag"]));
        xml.Append("/>");
    }

    void GetLeaderboard()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanGetInfo";
        sqcmd.Parameters.AddWithValue("@in_ClanID", 0);
        sqcmd.Parameters.AddWithValue("@in_GetMembers", 0);

        if (!CallWOApi(sqcmd))
            return;

        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append("<clans>");
        while (reader.Read())
        {
            OutClanInfo(xml, reader);
        }
        xml.Append("</clans>");

        GResponse.Write(xml.ToString());
    }

    void GetLeaderboardNew()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanGetList";
        sqcmd.Parameters.AddWithValue("@in_SortType", web.Param("t"));
        sqcmd.Parameters.AddWithValue("@in_Start", web.Param("pos"));
        sqcmd.Parameters.AddWithValue("@in_Size", web.Param("s"));

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int Size = getInt("Size");

        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append(string.Format("<clans size=\"{0}\">", Size));
        reader.NextResult();
        while (reader.Read())
        {
            OutClanInfo(xml, reader);
        }
        xml.Append("</clans>");

        GResponse.Write(xml.ToString());
    }

    void GetInfo()
    {
        string ClanID = web.Param("ClanID");
        string GetMembers = web.Param("GetMembers");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanGetInfo";
        sqcmd.Parameters.AddWithValue("@in_ClanID", ClanID);
        sqcmd.Parameters.AddWithValue("@in_GetMembers", GetMembers);

        if (!CallWOApi(sqcmd))
            return;

        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append("<clan_info>");
        
        // out clan info
        reader.Read();
        OutClanInfo(xml, reader);

        // out clan members
        if (GetMembers != "0")
        {
            reader.NextResult();

            xml.Append("<members>");
            while (reader.Read())
            {
                xml.Append("<m ");
                xml.Append(xml_attr("id", reader["CharID"]));
                xml.Append(xml_attr("cr", reader["ClanRank"]));
                xml.Append(xml_attr("gt", reader["Gamertag"]));
                xml.Append(xml_attr("cgp", reader["ClanContributedGP"]));
                xml.Append(xml_attr("cxp", reader["ClanContributedXP"]));
                xml.Append(xml_attr("xp", reader["XP"]));
                xml.Append(xml_attr("tp", reader["TimePlayed"]));
                xml.Append(xml_attr("r", reader["Reputation"]));
                xml.Append(xml_attr("ts00", reader["Stat00"]));
                xml.Append(xml_attr("ts01", reader["Stat01"]));
                xml.Append(xml_attr("ts02", reader["Stat02"]));
                xml.Append(xml_attr("ts03", reader["Stat03"]));
                xml.Append("/>");
            }
            xml.Append("</members>");
        }

        xml.Append("</clan_info>");

        GResponse.Write(xml.ToString());
    }

    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        string func = web.Param("func");
        if (func == "info")
            GetInfo();
        else if (func == "lb")
            GetLeaderboard();
        else if (func == "lb2")
            GetLeaderboardNew();
        else
            throw new ApiExitException("bad func");
    }
}
