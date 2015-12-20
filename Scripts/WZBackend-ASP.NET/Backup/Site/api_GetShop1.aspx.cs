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

public partial class api_GetShop1 : WOApiWebPage
{
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
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_GetShopInfo1";

        if (!CallWOApi(sqcmd))
            return;

        // "SHO1"
        byte[] itemHdr = new byte[4] { 83, 72, 79, 49 };

        MemoryStream ms = new MemoryStream();
        ms.Write(itemHdr, 0, 4);

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
