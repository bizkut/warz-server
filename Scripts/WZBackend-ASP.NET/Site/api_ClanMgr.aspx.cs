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

public partial class api_ClanMgr : WOApiWebPage
{
    void ClanLeave()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanLeave";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void ClanKick()
    {
        string MemberID = web.Param("MemberID");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanKickMember";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_MemberID", MemberID);

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void ClanSetRank()
    {
        string MemberID = web.Param("MemberID");
        string Rank = web.Param("Rank");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanSetMemberRank";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_MemberID", MemberID);
        sqcmd.Parameters.AddWithValue("@in_Rank", Rank);

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void ClanSetLore()
    {
        string Lore = web.Param("Lore");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanSetLore";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_Lore", Lore);

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void ClanDonateToClan()
    {
        Response.Write("WO_0");
        return; // this functionality is turned off

        /*string GP = web.Param("GP");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanDonateToClanGP";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_GP", GP);

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");*/
    }

    void ClanDonateToMember()
    {
        Response.Write("WO_0");
        return; // this functionality is turned off
        /*
        string GP = web.Param("GP");
        string MemberID = web.Param("MemberID");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanDonateToMemberGP";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_GP", GP);
        sqcmd.Parameters.AddWithValue("@in_MemberID", MemberID);

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");*/
    }

    void ClanBuySlots()
    {
        // calc item id for add members
        int idx = web.GetInt("idx");
        if (idx < 0 || idx >= 6)
            throw new ApiExitException("bad idx");

        int CLAN_ADDMEMBERS_ITEMID = 301152 + idx;

        // step 1 - buy upgrade item
        int balance = 0;
        {
            // permanent real $ buying
            string BuyIdx = "4";

            SqlCommand sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = BuyItem3.GetBuyProcFromIdx(BuyIdx);
            sqcmd.Parameters.AddWithValue("@in_IP", LastIP);
            sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
            sqcmd.Parameters.AddWithValue("@in_ItemId", CLAN_ADDMEMBERS_ITEMID);
            sqcmd.Parameters.AddWithValue("@in_BuyDays", 2000);

            if (!CallWOApi(sqcmd))
                return;

            reader.Read();
            balance = getInt("Balance");
        }

        // step 2: actually add slots
        {
            SqlCommand sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = "WZ_ClanAddClanMembers";
            sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
            sqcmd.Parameters.AddWithValue("@in_ItemID", CLAN_ADDMEMBERS_ITEMID);

            if (!CallWOApi(sqcmd))
                return;

            Response.Write("WO_0");
        }
    }

    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        string func = web.Param("func");
        if (func == "leave")
            ClanLeave();
        else if (func == "kick")
            ClanKick();
        else if (func == "setrank")
            ClanSetRank();
        else if (func == "setlore")
            ClanSetLore();
        else if (func == "gpmember")
            ClanDonateToMember();
        else if (func == "gpclan")
            ClanDonateToClan();
        else if (func == "buyslot")
            ClanBuySlots();
        else
            throw new ApiExitException("bad func");
    }
}
