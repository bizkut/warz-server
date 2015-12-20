using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_CharBackpack : WOApiWebPage
{
    void InventoryOp(string CustomerID, string CharID, int OpCode, Int64 v1, int v2, int v3)
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
        sqcmd.Parameters.AddWithValue("@in_CharID", CharID);

        if (OpCode >= 50)
        {
            string skey1 = web.Param("skey1");
            if (skey1 != SERVER_API_KEY)
                throw new ApiExitException("bad key");
        }

        switch (OpCode)
        {
            default:
                throw new ApiExitException("bad op code");

            case 10: // ApiBackpackToInventory
                sqcmd.CommandText = "WZ_BackpackToInv";
                sqcmd.Parameters.AddWithValue("@in_InventoryID", v1);
                sqcmd.Parameters.AddWithValue("@in_Slot", v2);
                sqcmd.Parameters.AddWithValue("@in_Amount", v3);
                break;

            case 11: // ApiBackpackFromInventory
                sqcmd.CommandText = "WZ_BackpackFromInv";
                sqcmd.Parameters.AddWithValue("@in_InventoryID", v1);
                sqcmd.Parameters.AddWithValue("@in_Slot",   v2);
                sqcmd.Parameters.AddWithValue("@in_Amount", v3);
                break;

            case 12: // ApiBackpackGridSwap
                sqcmd.CommandText = "WZ_BackpackGridSwap";
                sqcmd.Parameters.AddWithValue("@in_SlotFrom", v1);
                sqcmd.Parameters.AddWithValue("@in_SlotTo",   v2);
                break;

            case 13: // ApiBackpackGridJoin
                sqcmd.CommandText = "WZ_BackpackGridJoin";
                sqcmd.Parameters.AddWithValue("@in_SlotFrom", v1);
                sqcmd.Parameters.AddWithValue("@in_SlotTo", v2);
                break;

            case 16: // ApiBackpackChange
                sqcmd.CommandText = "WZ_BackpackChange";
                sqcmd.Parameters.AddWithValue("@in_InventoryID", v1);
                break;

            //
            // server codes
            //
            case 52: // ApiBackpackToInventory
                sqcmd.CommandText = "WZ_BackpackToInv";
                sqcmd.Parameters.AddWithValue("@in_InventoryID", v1);
                sqcmd.Parameters.AddWithValue("@in_Slot", v2);
                sqcmd.Parameters.AddWithValue("@in_Amount", v3);
                sqcmd.Parameters.AddWithValue("@in_ServerCall", 1);
                break;
            case 53: // ApiBackpackFromInventory
                sqcmd.CommandText = "WZ_BackpackFromInv";
                sqcmd.Parameters.AddWithValue("@in_InventoryID", v1);
                sqcmd.Parameters.AddWithValue("@in_Slot", v2);
                sqcmd.Parameters.AddWithValue("@in_Amount", v3);
                sqcmd.Parameters.AddWithValue("@in_ServerCall", 1);
                break;
            case 56: // Change Backpack ID/Size
                sqcmd.CommandText = "WZ_Backpack_SRV_Change";
                sqcmd.Parameters.AddWithValue("@in_BackpackID", v1);
                sqcmd.Parameters.AddWithValue("@in_BackpackSize", v2);
                break;
        }

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        Int64 InventoryID = Convert.ToInt64(reader["InventoryID"]);

        Response.Write("WO_0");
        Response.Write(string.Format("{0}", InventoryID));
    }

    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        int OpCode = web.GetInt("op");
        Int64 v1 = Convert.ToInt64(web.Param("v1"));
        int v2 = web.GetInt("v2");
        int v3 = web.GetInt("v3");

        string CustomerID = web.CustomerID();
        string CharID = web.Param("CharID");

        InventoryOp(CustomerID, CharID, OpCode, v1, v2, v3);
    }
}
