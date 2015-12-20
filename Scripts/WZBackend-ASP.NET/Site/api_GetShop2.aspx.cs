using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;
using System.IO;

public partial class api_GetShop2 : WOApiWebPage
{
    void OutSkillPrices(MemoryStream ms)
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SkillsGetPrices";

        if (!CallWOApi(sqcmd))
            throw new ApiExitException("no skill prices");

        // all skills
        while (reader.Read())
        {
            int SkillID = getInt("SkillID");
            int lv1 = getInt("Lv1");
            int lv2 = getInt("Lv2");
            int lv3 = getInt("Lv3");
            int lv4 = getInt("Lv4");
            int lv5 = getInt("Lv5");
            if (lv1 == 0)
                continue;

            msWriteShort(ms, SkillID);
            msWriteInt(ms, lv1);
            msWriteInt(ms, lv2);
            msWriteInt(ms, lv3);
            msWriteInt(ms, lv4);
            msWriteInt(ms, lv5);
        }
        msWriteShort(ms, 0xFFFF);
    }

    void OutShopItem(MemoryStream ms)
    {
        int p4 = getInt("PriceP");
        int p8 = getInt("GPriceP");
        if (p4 == 0 && p8 == 0)
            return;

        Int32 ItemId = getInt("ItemId");

        Int16 flags = 0;
        if (getInt("IsNew") > 0) flags |= 1;

        msWriteInt(ms, ItemId);
        msWriteByte(ms, flags);
        msWriteInt(ms, p4);
        msWriteInt(ms, p8);
    }

    protected override void Execute()
    {
        // "SHO1"
        byte[] itemHdr = new byte[4] { 83, 72, 79, 49 };

        MemoryStream ms = new MemoryStream();
        ms.Write(itemHdr, 0, 4);

        // skills first
        OutSkillPrices(ms);

        // item prices
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_GetShopInfo1";

        if (!CallWOApi(sqcmd))
            return;

        // all items
        while(reader.Read())
        {
            OutShopItem(ms);
        }

        ms.Write(itemHdr, 0, 4);

        GResponse.Write("WO_0");
        GResponse.BinaryWrite(ms.ToArray());
    }               
}
