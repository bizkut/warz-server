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

public partial class api_GPConvert : WOApiWebPage
{
    /*
    0-$10  = 140GC/$
    10.01 – 30 = $150
    >30 = $160
    */

    // XSOLLA panel:
    // https://tools.xsolla.com/fire_tools/partners/

    // 25% sale
    // numbers for xsolla (projects 8396 & 10638):
    // 210 1.0
    // 2250 10.0
    // 7200 30.0
    /*static int[] GCPriceTable = {
        // max cents, rate
        1000, 210,
        3000, 225,
        999999, 240
    };

    static int[] SteamGCPriceTable = {
        // max cents, rate
        1000, 210,
        3000, 225,
        999999, 240
    };*/

    // 30% sale
    // numbers for xsolla (projects 8396 & 10638):
    // 224 1.0
    // 2400 10.0
    // 7680 30.0
    /*static int[] GCPriceTable = {
        // max cents, rate
        1000, 224,
        3000, 240,
        999999, 256
    };

    static int[] SteamGCPriceTable = {
        // max cents, rate
        1000, (224),
        3000, (240),
        999999, (256)
    };*/

    // 50% sale
    // numbers for xsolla (projects 8396 & 10638):
    // 280 1.0
    // 3000 10.0
    // 9600 30.0
   /* static int[] GCPriceTable = {
        // max cents, rate
        1000, 280,
        3000, 300,
        999999, 320
    };

    static int[] SteamGCPriceTable = {
        // max cents, rate
        1000, (280),
        3000, (300),
        999999, (320)
    };*/

    // regular prices
    // numbers for xsolla (projects 8396 & 10638):
    // 140 1.0
    // 1500 10.0
    // 4800 30.0
    static int[] GCPriceTable = {
        // max cents, rate
        1000, 140,
        3000, 150,
        999999, 160
    };

    static int[] SteamGCPriceTable = {
        // max cents, rate
        1000, (140),
        3000, (150),
        999999, (160)
    };

    static public int GetGPConversionRateFromPrice(int price, bool fromSteam)
    {
        int[] prices = fromSteam ? SteamGCPriceTable : GCPriceTable;
        for (int i = 0; i < prices.Length/2; i++)
        {
            if (price <= prices[i * 2 + 0])
                return prices[i * 2 + 1];
        }

        throw new ApiExitException("bad price");
    }


    void GPGetConvertRates()
    {
        bool fromSteam = false;
        if (web.OptionalParam("steam", false) != null)
            fromSteam = true;

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_GPGetConversionRates";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());

        if (!CallWOApi(sqcmd))
            return;

        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append("<rates>\n");
        while (reader.Read())
        {
            xml.Append("<r ");
            xml.Append(xml_attr("GP", reader["GamePoints"]));
            xml.Append(xml_attr("Rate", reader["ConversionRate"]));
            xml.Append("/>");
        }
        xml.Append("</rates>\n");

        xml.Append("<gc>\n");
        xml.Append("<r ");
        int[] prices = fromSteam ? SteamGCPriceTable : GCPriceTable;
        for (int i = 0; i < prices.Length; i++)
        {
            xml.Append(xml_attr(String.Format("r{0}", i), prices[i].ToString()));
        }
        xml.Append("/>");
        xml.Append("</gc>\n");

        GResponse.Write(xml.ToString());
    }

    void GPConvert()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_GPConvert";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_GP", web.Param("GP"));

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int GameDollars = getInt("GameDollars");

        Response.Write("WO_0");
        Response.Write(string.Format("{0}", GameDollars));
    }

    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        string func = web.Param("func");
        if (func == "rates")
            GPGetConvertRates();
        else if (func == "convert")
            GPConvert();
        else
            throw new ApiExitException("bad func");
    }
}
