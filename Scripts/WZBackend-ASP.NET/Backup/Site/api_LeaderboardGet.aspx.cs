using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_LeaderboardGet : WOApiWebPage
{
    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_LeaderboardGet";
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_TableID", web.Param("t"));
        sqcmd.Parameters.AddWithValue("@in_StartPos", web.Param("pos"));

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int CurPos = getInt("StartPos");
        int Size = getInt("Size");

        // report page of leaderboard
        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append(string.Format("<leaderboard pos=\"{0}\" size=\"{1}\">", CurPos, Size));

        reader.NextResult();
        while (reader.Read())
        {
            xml.Append("<l ");
            xml.Append(xml_attr("GT", reader["Gamertag"]));
            xml.Append(xml_attr("a", reader["Alive"]));
            xml.Append(xml_attr("XP", reader["XP"]));
            xml.Append(xml_attr("tp", reader["TimePlayed"]));
            xml.Append(xml_attr("r", reader["Reputation"]));
            xml.Append(xml_attr("ts00", reader["Stat00"]));
            xml.Append(xml_attr("ts01", reader["Stat01"]));
            xml.Append(xml_attr("ts02", reader["Stat02"]));
            xml.Append(xml_attr("ts03", reader["Stat03"]));
            xml.Append(xml_attr("gs", reader["GameServerId"]));
            xml.Append(xml_attr("ci", reader["ClanID"]));
            xml.Append(xml_attr("ct", reader["ClanTag"]));
            xml.Append(xml_attr("cc", reader["ClanTagColor"]));
            xml.Append("/>");
        }
        xml.Append("</leaderboard>");

        GResponse.Write(xml.ToString());
    }
}
