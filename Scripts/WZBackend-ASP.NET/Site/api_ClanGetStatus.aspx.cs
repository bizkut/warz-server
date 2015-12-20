using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_ClanGetStatus : WOApiWebPage
{
    void ClanGetInvites(ref StringBuilder xml)
    {
        string CustomerID = web.CustomerID();

        // report invites
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanInviteGetInvitesForPlayer";
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));

        if (!CallWOApi(sqcmd))
            return;

        xml.Append("<clinvites>");
        while (reader.Read())
        {
            xml.Append("<inv ");
            // info about invite
            xml.Append(xml_attr("id", reader["ClanInviteID"]));
            xml.Append(xml_attr("gt", reader["Gamertag"]));
            // info about clan
            xml.Append(xml_attr("cname", reader["ClanName"]));
            xml.Append(xml_attr("cl", reader["ClanLevel"]));
            xml.Append(xml_attr("cem", reader["ClanEmblemID"]));
            xml.Append(xml_attr("cemc", reader["ClanEmblemColor"]));
            xml.Append(xml_attr("cm1", reader["MaxClanMembers"]));
            xml.Append(xml_attr("cm2", reader["NumClanMembers"]));
            xml.Append("/>\n");
        }
        xml.Append("</clinvites>");
    }

    void ClanGetApplications(ref StringBuilder xml)
    {
        string CustomerID = web.CustomerID();

        // report clan applications
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanApplyGetList";
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));

        if (!CallWOApi(sqcmd))
            return;

        xml.Append("<clapps>");
        while (reader.Read())
        {
            xml.Append("<app ");
            xml.Append(xml_attr("id", reader["ClanApplicationID"]));
            xml.Append(xml_attr("note", reader["ApplicationText"]));
            xml.Append(xml_attr("gt", reader["Gamertag"]));
            xml.Append(xml_attr("xp", reader["XP"]));
            xml.Append(xml_attr("tp", reader["TimePlayed"]));
            xml.Append(xml_attr("r", reader["Reputation"]));
            xml.Append(xml_attr("ts00", reader["Stat00"]));
            xml.Append(xml_attr("ts01", reader["Stat01"]));
            xml.Append(xml_attr("ts02", reader["Stat02"]));
            xml.Append(xml_attr("ts03", reader["Stat03"]));
            xml.Append("/>\n");
        }
        xml.Append("</clapps>");
    }

    void ReportClanStatus(ref StringBuilder xml)
    {
        string CustomerID = web.CustomerID();

        xml.Append("<clan>");

        int ClanID = 0;
        int ClanRank = 0;

        // get user clan info
        {
            SqlCommand sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = "WZ_ClanGetPlayerData";
            sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));

            if (!CallWOApi(sqcmd))
                return;

            reader.Read();
            ClanID = getInt("ClanID");
            ClanRank = getInt("ClanRank");
        }

        if (ClanID == 0)
        {
            // if no clan yet
            xml.Append("<cldata ");
            xml.Append(xml_attr("ID", ClanID));
            xml.Append("/>\n");

            ClanGetInvites(ref xml);
        }
        else
        {
            // we have clan
            xml.Append("<cldata ");
            xml.Append(xml_attr("ID", ClanID));
            xml.Append(xml_attr("rank", ClanRank));
            xml.Append(xml_attr("cm1", reader["NumClanMembers"]));
            xml.Append(xml_attr("cm2", reader["MaxClanMembers"]));
            xml.Append("/>\n");

            // if officer, report applications
            if (ClanRank <= 1)
            {
                ClanGetApplications(ref xml);
            }
        }

        xml.Append("</clan>");
    }

    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");

        ReportClanStatus(ref xml);
        
        GResponse.Write(xml.ToString());
    }
}
