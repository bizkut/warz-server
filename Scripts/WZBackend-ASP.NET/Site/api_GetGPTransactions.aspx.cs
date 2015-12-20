using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_GetGPTransactions : WOApiWebPage
{
    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_GetGPTransactions";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int GamePoints = getInt("GamePoints");

        // report page of leaderboard
        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append(string.Format("<gplog balance=\"{0}\">", GamePoints));

        reader.NextResult();
        while (reader.Read())
        {
            xml.Append("<l ");
            xml.Append(xml_attr("i", reader["TransactionID"]));
            xml.Append(xml_attr("t", ToUnixTime(Convert.ToDateTime(reader["TransactionTime"]))));
            xml.Append(xml_attr("a", reader["Amount"]));
            xml.Append(xml_attr("p", reader["Previous"]));
            xml.Append(xml_attr("d", reader["TransactionDesc"]));
            xml.Append("/>");
        }
        xml.Append("</gplog>");

        GResponse.Write(xml.ToString());
    }
}
