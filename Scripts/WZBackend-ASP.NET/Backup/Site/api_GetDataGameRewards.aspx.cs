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

public partial class api_GetDataGameRewards : WOApiWebPage
{
    protected override void Execute()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_GetDataGameRewards";

        if (!CallWOApi(sqcmd))
            return;

        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append("<rewards>");

        while (reader.Read())
        {
            string name = reader["Name"].ToString();

            xml.Append("<rwd ");
            xml.Append(xml_attr("ID", reader["ID"]));
            xml.Append(xml_attr("Name", name));
            xml.Append(xml_attr("GD_SOFT", reader["GD_SOFT"]));
            xml.Append(xml_attr("XP_SOFT", reader["XP_SOFT"]));
            xml.Append(xml_attr("GD_HARD", reader["GD_HARD"]));
            xml.Append(xml_attr("XP_HARD", reader["XP_HARD"]));
            xml.Append("/>\n");
        }

        xml.Append("</rewards>");
        GResponse.Write(xml.ToString());
    }               
}
