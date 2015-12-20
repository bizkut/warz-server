using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_CharSlots : WOApiWebPage
{
    void APICharReviveCheck(out int NeedMoney, out int SecToRevive)
    {
        string CustomerID = web.CustomerID();
        string CharID = web.Param("CharID");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_CharReviveCheck";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
        sqcmd.Parameters.AddWithValue("@in_CharID", CharID);

        if (!CallWOApi(sqcmd))
            throw new ApiExitException("APICharReviveCheck failed");

        reader.Read();
        NeedMoney = getInt("NeedMoney");
        SecToRevive = getInt("SecToRevive");
    }

    void CharReviveCheck()
    {
        int NeedMoney;
        int SecToRevive;
        APICharReviveCheck(out NeedMoney, out SecToRevive);

        Response.Write("WO_0");
        Response.Write(string.Format("{0} {1}", NeedMoney, SecToRevive));
    }

    void CharRevive()
    {
        string CustomerID = web.CustomerID();
        string CharID = web.Param("CharID");

        int NeedMoney;
        int SecToRevive;
        APICharReviveCheck(out NeedMoney, out SecToRevive);

        // buy revive item if we must
        if (NeedMoney > 0)
        {
            // make sure that client understand that we're reviving with money
            int ClientNeedMoney = web.GetInt("NeedMoney");
            if (NeedMoney > 0 && ClientNeedMoney == 0)
                throw new ApiExitException("client must ack revive");

            int CHAR_REVIVE_ITEMID = 301159;

            // permanent real $ buying
            string BuyIdx = "4";

            SqlCommand sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = BuyItem3.GetBuyProcFromIdx(BuyIdx);
            sqcmd.Parameters.AddWithValue("@in_IP", LastIP);
            sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
            sqcmd.Parameters.AddWithValue("@in_ItemId", CHAR_REVIVE_ITEMID);
            sqcmd.Parameters.AddWithValue("@in_BuyDays", 2000);

            if (!CallWOApi(sqcmd))
                return;

            reader.Read();
            int balance = getInt("Balance");
        }

        // actual revive
        {
            SqlCommand sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = "WZ_CharRevive";
            sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
            sqcmd.Parameters.AddWithValue("@in_CharID", CharID);

            if (!CallWOApi(sqcmd))
                return;

            Response.Write("WO_0");
        }
    }

    bool CheckCharName(string Gamertag)
    {
        char[] AllowedChars = " 1234567890abcdefghijklmnopqrstuvwxyzабвгдеёжзийклмнопрстуфхцчшщъыьэюя".ToCharArray();
        foreach (char c in Gamertag)
        {
            if (c.ToString().ToLower().IndexOfAny(AllowedChars) == -1)
            {
                return false;
            }
        }
        return true;
    }

    void CharCreate()
    {
        string Gamertag = web.Param("Gamertag");
        if (!CheckCharName(Gamertag))
        {
            Response.Write("WO_7");
            Response.Write("Character name cannot contain special symbols");
            return;
        }

        string CustomerID = web.CustomerID();

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_CharCreate";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
        sqcmd.Parameters.AddWithValue("@in_Hardcore", web.Param("Hardcore"));
        sqcmd.Parameters.AddWithValue("@in_Gamertag", Gamertag);
        sqcmd.Parameters.AddWithValue("@in_HeroItemID", web.Param("HeroItemID"));
        sqcmd.Parameters.AddWithValue("@in_HeadIdx", web.Param("HeadIdx"));
        sqcmd.Parameters.AddWithValue("@in_BodyIdx", web.Param("BodyIdx"));
        sqcmd.Parameters.AddWithValue("@in_LegsIdx", web.Param("LegsIdx"));

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int CharID = getInt("CharID");

        Response.Write("WO_0");
        Response.Write(CharID.ToString());
    }

    void CharDelete()
    {
        string CustomerID = web.CustomerID();
        string CharID = web.Param("CharID");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_CharDelete";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
        sqcmd.Parameters.AddWithValue("@in_CharID", CharID);

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void CharSkin()
    {
        string CustomerID = web.CustomerID();

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_CharChangeSkin";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_HeadIdx", web.Param("HeadIdx"));
        sqcmd.Parameters.AddWithValue("@in_BodyIdx", web.Param("BodyIdx"));
        sqcmd.Parameters.AddWithValue("@in_LegsIdx", web.Param("LegsIdx"));

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void CharRename()
    {
        string Gamertag = web.Param("Gamertag");
        if (!CheckCharName(Gamertag))
        {
            Response.Write("WO_7");
            Response.Write("Character name cannot contain special symbols");
            return;
        }

        // check if can rename
        {
            SqlCommand sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = "WZ_CharRenameCheck";
            sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
            sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
            sqcmd.Parameters.AddWithValue("@in_Gamertag", Gamertag);

            if (!CallWOApi(sqcmd))
                return;

            reader.Read();
            int MinutesLeft = getInt("MinutesLeft");
            if (MinutesLeft > 0)
            {

                Response.Write(string.Format("WO_4{0}", MinutesLeft));
                return;
            }
        }

        // buy item
        {
            int CHAR_RENAME_ITEMID = 301399;

            // permanent real $ buying
            string BuyIdx = "4";

            SqlCommand sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = BuyItem3.GetBuyProcFromIdx(BuyIdx);
            sqcmd.Parameters.AddWithValue("@in_IP", LastIP);
            sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
            sqcmd.Parameters.AddWithValue("@in_ItemId", CHAR_RENAME_ITEMID);
            sqcmd.Parameters.AddWithValue("@in_BuyDays", 2000);

            if (!CallWOApi(sqcmd))
                return;
        }

        // exec rename
        {
            SqlCommand sqcmd = new SqlCommand();
            sqcmd.CommandType = CommandType.StoredProcedure;
            sqcmd.CommandText = "WZ_CharRename";
            sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
            sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
            sqcmd.Parameters.AddWithValue("@in_Gamertag", Gamertag);

            if (!CallWOApi(sqcmd))
                return;
        }

        Response.Write("WO_0");
    }

    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        string func = web.Param("func");
        if (func == "revive")
            CharRevive();
        else if (func == "revcheck")
            CharReviveCheck();
        else if (func == "create")
            CharCreate();
        else if (func == "delete")
            CharDelete();
        else if (func == "skin")
            CharSkin();
        else if (func == "rename")
            CharRename();
        else
            throw new ApiExitException("bad func");
    }
}