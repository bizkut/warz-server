using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_SrvObjects : WOApiWebPage
{
    void GetObjects()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_ObjectsGetAll";
        sqcmd.Parameters.AddWithValue("@in_GameServerID", web.Param("GameSID"));
        if (!CallWOApi(sqcmd))
            return;

        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append("<sobjects>\n");
        while (reader.Read())
        {
            xml.Append("<obj ");
            xml.Append(xml_attr("ServerObjectID", reader["ServerObjectID"]));
            xml.Append(xml_attr("CreateDate", ToUnixTime(Convert.ToDateTime(reader["CreateUtcDate"]))));
            xml.Append(xml_attr("ExpireMins", reader["ExpireMins"]));
            xml.Append(xml_attr("ObjType", reader["ObjType"]));
            xml.Append(xml_attr("ObjClassName", reader["ObjClassName"]));
            xml.Append(xml_attr("ItemID", reader["ItemID"]));
            xml.Append(xml_attr("px", reader["px"]));
            xml.Append(xml_attr("py", reader["py"]));
            xml.Append(xml_attr("pz", reader["pz"]));
            xml.Append(xml_attr("rx", reader["rx"]));
            xml.Append(xml_attr("ry", reader["ry"]));
            xml.Append(xml_attr("rz", reader["rz"]));
            xml.Append(xml_attr("CustomerID", reader["CustomerID"]));
            xml.Append(xml_attr("CharID", reader["CharID"]));
            xml.Append(xml_attr("Var1", reader["Var1"]));
            xml.Append(xml_attr("Var2", reader["Var2"]));
            xml.Append(xml_attr("Var3", reader["Var3"]));

            xml.Append("/>");
        }
        xml.Append("</sobjects>\n");

        GResponse.Write(xml.ToString());
    }

    void AddObject()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_ObjectsAdd";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_GameServerID", web.Param("GameSID"));
        sqcmd.Parameters.AddWithValue("@in_ExpireMins", web.Param("ExpireMins"));
        sqcmd.Parameters.AddWithValue("@in_ObjType", web.Param("ObjType"));
        sqcmd.Parameters.AddWithValue("@in_ObjClassName", web.Param("ObjClass"));
        sqcmd.Parameters.AddWithValue("@in_ItemID", web.Param("ItemID"));
        sqcmd.Parameters.AddWithValue("@in_px", web.Param("px"));
        sqcmd.Parameters.AddWithValue("@in_py", web.Param("py"));
        sqcmd.Parameters.AddWithValue("@in_pz", web.Param("pz"));
        sqcmd.Parameters.AddWithValue("@in_rx", web.Param("rx"));
        sqcmd.Parameters.AddWithValue("@in_ry", web.Param("ry"));
        sqcmd.Parameters.AddWithValue("@in_rz", web.Param("rz"));
        sqcmd.Parameters.AddWithValue("@in_Var1", web.Param("Var1"));
        sqcmd.Parameters.AddWithValue("@in_Var2", web.Param("Var2"));
        sqcmd.Parameters.AddWithValue("@in_Var3", web.Param("Var3"));

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int ServerObjectID = getInt("ServerObjectID");
        Response.Write("WO_0");
        Response.Write(string.Format("{0}", ServerObjectID));
    }

    void DeleteObject()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_ObjectsDelete";
        sqcmd.Parameters.AddWithValue("@in_ServerObjectID", web.Param("ObjectID"));

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void UpdateObject()
    {
        // optional parameter - expire time left in minutes.
        int ExpireMins = -1; // -1 mean expire time will not change
        try
        {
            ExpireMins = web.GetInt("ExpireMins");
        }
        catch { }

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_ObjectsUpdate";
        sqcmd.Parameters.AddWithValue("@in_ServerObjectID", web.Param("ObjectID"));
        sqcmd.Parameters.AddWithValue("@in_Var1", web.Param("Var1"));
        sqcmd.Parameters.AddWithValue("@in_Var2", web.Param("Var2"));
        sqcmd.Parameters.AddWithValue("@in_Var3", web.Param("Var3"));
        sqcmd.Parameters.AddWithValue("@in_ExpireMins", ExpireMins);

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void ResetHibernatedObjects()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SRV_ObjectsResetHibernate";
        sqcmd.Parameters.AddWithValue("@in_GameServerID", web.Param("GameSID"));

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    protected override void Execute()
    {
        string skey1 = web.Param("skey1");
        if (skey1 != SERVER_API_KEY)
            throw new ApiExitException("bad key");

        string func = web.Param("func");
        if (func == "get")
            GetObjects();
        else if (func == "add")
            AddObject();
        else if (func == "del")
            DeleteObject();
        else if (func == "update")
            UpdateObject();
        else if (func == "hstart")
            ResetHibernatedObjects();
        else
            throw new ApiExitException("bad func");
    }
}
