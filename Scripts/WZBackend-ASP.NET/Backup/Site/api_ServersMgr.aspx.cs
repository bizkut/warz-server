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

public partial class api_ServersMgr : WOApiWebPage
{
    const int HOURLY_RENTID_START = 100;

    class PriceOptions
    {
        public int[] BasePrice = {0, 0, 0, 0}; // US, EU, RU, SA
        public int PVE;
        public int PVP;
        public int Slot1;
        public int Slot2;
        public int Slot3;
        public int Slot4;
        public int Slot5;
        public int Passworded;
        public int MonthX2;
        public int MonthX3;
        public int MonthX6;
        public int WeekX1;
        public int OptNameplates;
        public int OptCrosshair;
        public int OptTracers;
    };
    PriceOptions OptGameServer = new PriceOptions();
    PriceOptions OptStronghold = new PriceOptions();

    // US, EU, EU, SA, then same for strongholds
    int[] Capacity = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int[] Games    = { 0, 0, 0, 0, 0, 0, 0, 0 };

    void ReadPriceOptions(ref PriceOptions opt)
    {
        reader.Read();

        opt.BasePrice[0] = getInt("Base_US");
        opt.BasePrice[1] = getInt("Base_EU");
        opt.BasePrice[2] = getInt("Base_RU");
        opt.BasePrice[3] = getInt("Base_SA");
        opt.PVE = getInt("PVE");
        opt.PVP = getInt("PVP");
        opt.Slot1 = getInt("Slot1");
        opt.Slot2 = getInt("Slot2");
        opt.Slot3 = getInt("Slot3");
        opt.Slot4 = getInt("Slot4");
        opt.Slot5 = getInt("Slot5");
        opt.Passworded = getInt("Passworded");
        opt.MonthX2 = getInt("MonthX2");
        opt.MonthX3 = getInt("MonthX3");
        opt.MonthX6 = getInt("MonthX6");
        opt.WeekX1 = getInt("WeekX1");
        opt.OptNameplates = getInt("OptNameplates");
        opt.OptCrosshair = getInt("OptCrosshair");
        opt.OptTracers = getInt("OptTracers");
    }

    void ReadPricesFromDB(bool isRent)
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ServerGetPrices";

        if (!CallWOApi(sqcmd))
            throw new ApiExitException("can't get server prices");

        ReadPriceOptions(ref OptGameServer);
        reader.NextResult();
        ReadPriceOptions(ref OptStronghold);

        // FOR UPLOADING TO PUBLIC API - temporary disable strongholds
        //OptStronghold.BasePrice[0] = 0;
        //OptStronghold.BasePrice[1] = 0;
        //OptStronghold.BasePrice[2] = 0;
        //OptStronghold.BasePrice[3] = 0;

       // capacity for games
        reader.NextResult();
        reader.Read();

        Capacity[0] = getInt("Capacity_US");
        Capacity[1] = getInt("Capacity_EU");
        Capacity[2] = getInt("Capacity_RU");
        Capacity[3] = getInt("Capacity_SA");
        Games[0]    = getInt("Games_US");
        Games[1]    = getInt("Games_EU");
        Games[2]    = getInt("Games_RU");
        Games[3]    = getInt("Games_SA");

      /*  // capacity for strongholds
        reader.NextResult();
        reader.Read();

        Capacity[4] = getInt("Capacity_US");
        Capacity[5] = getInt("Capacity_EU");
        Capacity[6] = getInt("Capacity_RU");
        Capacity[7] = getInt("Capacity_SA");
        Games[4]    = getInt("Games_US");
        Games[5]    = getInt("Games_EU");
        Games[6]    = getInt("Games_RU");
        Games[7]    = getInt("Games_SA");*/

        if (isRent)
        {
            // check if we have space for new servers (minus some slots for emergency)
            for (int i = 0; i < 4; i++)
            {
                if (Games[i] >= Capacity[i] - 14)
                    OptGameServer.BasePrice[i] = -OptGameServer.BasePrice[i];
            }
            for (int i = 4; i < 8; i++)
            {
                if (Games[i] >= Capacity[i] - 14)
                    OptStronghold.BasePrice[i - 4] = -OptStronghold.BasePrice[i - 4];
            }
        }
    }

    void PriceOptionsToXML(ref StringBuilder xml, ref PriceOptions opt)
    {
        xml.Append(xml_attr("Base_US", opt.BasePrice[0]));
        xml.Append(xml_attr("Base_EU", opt.BasePrice[1]));
        xml.Append(xml_attr("Base_RU", opt.BasePrice[2]));
        xml.Append(xml_attr("Base_SA", opt.BasePrice[3]));
        xml.Append(xml_attr("PVE", opt.PVE));
        xml.Append(xml_attr("PVP", opt.PVP));
        xml.Append(xml_attr("Slot1", opt.Slot1));
        xml.Append(xml_attr("Slot2", opt.Slot2));
        xml.Append(xml_attr("Slot3", opt.Slot3));
        xml.Append(xml_attr("Slot4", opt.Slot4));
        xml.Append(xml_attr("Slot5", opt.Slot5));
        xml.Append(xml_attr("Passworded", opt.Passworded));
        xml.Append(xml_attr("MonthX2", opt.MonthX2));
        xml.Append(xml_attr("MonthX3", opt.MonthX3));
        xml.Append(xml_attr("MonthX6", opt.MonthX6));
        xml.Append(xml_attr("WeekX1", opt.WeekX1));
        xml.Append(xml_attr("OptNameplates", opt.OptNameplates));
        xml.Append(xml_attr("OptCrosshair", opt.OptCrosshair));
        xml.Append(xml_attr("OptTracers", opt.OptTracers));
    }

    void ServerGetPrices()
    {
        ReadPricesFromDB(true);

        StringBuilder xml = new StringBuilder();
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append(string.Format("<srent>"));

        xml.Append("<g ");
        PriceOptionsToXML(ref xml, ref OptGameServer);
        xml.Append("/>");

        xml.Append("<s ");
        PriceOptionsToXML(ref xml, ref OptStronghold);
        xml.Append("/>");

        xml.Append("</srent>");

        GResponse.Write(xml.ToString());
    }

    class ServerRentParams
    {
        public int isGameServer;
        public int mapID;
        public int regionID;
        public int slotID;
        public int rentID;
        public string name;
        public string password;
        public int pveID;
        public int nameplates;
        public int crosshair;
        public int tracers;
        public int client_price;

        public ServerRentParams(WebHelper web)
        {
            isGameServer = web.GetInt("isGameServer");
            mapID = web.GetInt("mapID");
            regionID = web.GetInt("regionID");
            slotID = web.GetInt("slotID");
            rentID = web.GetInt("rentID");
            name = web.Param("name");
            password = web.Param("password");
            pveID = web.GetInt("pveID");
            nameplates = web.GetInt("nameplates");
            crosshair = web.GetInt("crosshair");
            tracers = web.GetInt("tracers");
            client_price = web.GetInt("client_price");

            if(isGameServer == 0 && mapID != 3)
            {
                throw new ApiExitException("bad map for stronghold");
            }
        }
    };


    //
    // WARNING: this code must be in sync with client code
    //
    int getServerRentOptionPrice(int price, int opt)
    {
        // get percent of base price
        int add = price * opt / 100;
        return add;
    }

    //
    // WARNING: this code must be in sync with client code
    //
    int calcServerRentPrice(ServerRentParams prm)
    {
        PriceOptions opt = prm.isGameServer > 0 ? OptGameServer : OptStronghold;

        // calc base
        int server_base = 0;
        switch (prm.regionID)
        {
            case 1:  server_base = opt.BasePrice[0]; break;
            case 10: server_base = opt.BasePrice[1]; break;
            case 20: server_base = opt.BasePrice[2]; break;
            case 30: server_base = opt.BasePrice[3]; break;
            default: throw new ApiExitException("bad regionID");
        }
        if (server_base == 0) return 0;

        // calc all other options
        int price = server_base;

        //[TH] price is set for slot, not by percents
        switch (prm.slotID)
        {
            case 0: price = opt.Slot1; break;
            case 1: price = opt.Slot2; break;
            case 2: price = opt.Slot3; break;
            case 3: price = opt.Slot4; break;
            case 4: price = opt.Slot5; break;
            default: throw new ApiExitException("bad slotID");
        }

        if (prm.password.Length > 0)
            price += getServerRentOptionPrice(server_base, opt.Passworded);

        if (prm.pveID > 0)
            price += getServerRentOptionPrice(server_base, opt.PVE);
        else
            price += getServerRentOptionPrice(server_base, opt.PVP);

        if (prm.nameplates > 0)
            price += getServerRentOptionPrice(server_base, opt.OptNameplates);
        if (prm.crosshair > 0)
            price += getServerRentOptionPrice(server_base, opt.OptCrosshair);
        if (prm.tracers > 0)
            price += getServerRentOptionPrice(server_base, opt.OptTracers);

        // hourly rent, hours is after 100
        if (prm.rentID >= HOURLY_RENTID_START)
        {
            // 10% add for hourly rent
            price += price / 10;
            price = (int)Math.Ceiling((float)price / 24.0f / 31.0f);
            price = (prm.rentID - HOURLY_RENTID_START) * price;
            return price;
        }

        // [TH] adjust per day coeff
        switch (prm.rentID)
        {
            case 0: price *= 3; break;
            case 1: price *= 7; break;
            case 2: price *= 15; break;
            case 3: price *= 30; break;
            case 4: price *= 60; break;
            default: throw new ApiExitException("bad rentid");
        }

        return price;
    }

    int calcRentHours(int rentID)
    {
        // if hourly rent
        if (rentID >= 100)
        {
            return rentID - 100;
        }

        //[TH] rent by hours
        int RentHours = 24;
        switch (rentID)
        {
            case 0: RentHours *= 3; break;
            case 1: RentHours *= 7; break;
            case 2: RentHours *= 15; break;
            case 3: RentHours *= 30; break;
            case 4: RentHours *= 60; break;
            default: throw new ApiExitException("bad rentID:" + rentID);
        }
        return RentHours;
    }

    int ServerGetHoursLeftForDonate()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ServerCheckCanDonate";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());

        if (!CallWOApi(sqcmd))
            throw new ApiExitException("WZ_ServerCheckCanDonate failed");

        reader.Read();
        int HoursLeft = getInt("HoursLeft");
        return HoursLeft;
    }

    void ServerRent()
    {
        ServerRentParams prm = new ServerRentParams(web);
        prm.name = prm.name.Trim();
        if (prm.name.Length == 0)
            throw new ApiExitException("empty server name");
        if (prm.rentID >= HOURLY_RENTID_START)
            throw new ApiExitException("no hourly rent");

        // verify that client price is correct!
        ReadPricesFromDB(true);
        int price = calcServerRentPrice(prm);
        if (price != prm.client_price)
        {
            Response.Write("WO_3");
            Response.Write(string.Format("price calc desync {0} vs {1}", price, prm.client_price));
            return;
        }
        if (price <= 0)
            throw new ApiExitException("no price");

        // get server slots number based on slot index
        int[] GameworldSlots = { 10, 30, 50, 70, 100 };
        int[] StrongholdSlots = { 10, 20, 30, 40, 50 };
        int ServerSlots = (prm.isGameServer > 0) ? GameworldSlots[prm.slotID] : StrongholdSlots[prm.slotID];
        if (ServerSlots < 0)
            throw new ApiExitException("bad slotID");

        // create server flags. SYNC with client
        int SFLAGS_Passworded = 1 << 0;	// passworded server
        int SFLAGS_PVE = 1 << 1;
        int SFLAGS_Nameplates = 1 << 2;
        int SFLAGS_CrossHair = 1 << 3;
        int SFLAGS_Tracers = 1 << 4;
        int ServerFlags = 0;
        if (prm.password.Length > 0)
            ServerFlags |= SFLAGS_Passworded;
        if (prm.pveID > 0)
            ServerFlags |= SFLAGS_PVE;
        if (prm.nameplates > 0)
            ServerFlags |= SFLAGS_Nameplates;
        if (prm.crosshair > 0)
            ServerFlags |= SFLAGS_CrossHair;
        if (prm.tracers > 0)
            ServerFlags |= SFLAGS_Tracers;

        // calc hours for rent
        int RentHours = calcRentHours(prm.rentID);

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ServerRent";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_PriceGP", price);
        sqcmd.Parameters.AddWithValue("@in_ServerRegion", prm.regionID);
        sqcmd.Parameters.AddWithValue("@in_ServerType", prm.isGameServer);
        sqcmd.Parameters.AddWithValue("@in_ServerFlags", ServerFlags);
        sqcmd.Parameters.AddWithValue("@in_ServerMap", prm.mapID);
        sqcmd.Parameters.AddWithValue("@in_ServerSlots", ServerSlots);
        sqcmd.Parameters.AddWithValue("@in_ServerName", prm.name);
        sqcmd.Parameters.AddWithValue("@in_ServerPwd", prm.password);
        sqcmd.Parameters.AddWithValue("@in_RentHours", RentHours);

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int ServerID = getInt("ServerID");

        Response.Write("WO_0");
        Response.Write(string.Format("{0}", ServerID));
    }

    void ServerRenew()
    {
        ServerRentParams prm = new ServerRentParams(web);

        // hourly rent, check if user can donate (there is required delay after account creation)
        if (prm.rentID >= HOURLY_RENTID_START)
        {
            int HoursLeft = ServerGetHoursLeftForDonate();
            if (HoursLeft > 0)
            {
                Response.Write("WO_4");
                Response.Write(string.Format("{0}", HoursLeft));
                return;
            }
        }

        // verify that client price is correct!
        ReadPricesFromDB(false);
        int price = calcServerRentPrice(prm);
        if (price != prm.client_price)
        {
            Response.Write("WO_3");
            Response.Write(string.Format("price calc desync {0} vs {1}", price, prm.client_price));
            return;
        }
        if (price <= 0)
            throw new ApiExitException("no price");

        // calc hours for rent
        int RentHours = calcRentHours(prm.rentID);

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ServerRenew";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_GameServerID", web.Param("GSID"));
        sqcmd.Parameters.AddWithValue("@in_PriceGP", price);
        sqcmd.Parameters.AddWithValue("@in_RentHours", RentHours);

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int ServerID = getInt("ServerID");

        Response.Write("WO_0");
        Response.Write(string.Format("{0}", ServerID));
    }

    static public void OutputXmlServerInfo(StringBuilder xml, SqlDataReader reader, bool server)
    {
        xml.Append("<?xml version=\"1.0\"?>\n");
        xml.Append(string.Format("<slist>"));

        while (reader.Read())
        {
            xml.Append("<srv ");
            xml.Append(xml_attr("ID", reader["GameServerID"]));
            xml.Append(xml_attr("AdminKey", reader["AdminKey"]));
            xml.Append(xml_attr("ServerRegion", reader["ServerRegion"]));
            xml.Append(xml_attr("ServerType", reader["ServerType"]));
            xml.Append(xml_attr("ServerFlags", reader["ServerFlags"]));
            xml.Append(xml_attr("ServerMap", reader["ServerMap"]));
            xml.Append(xml_attr("ServerSlots", reader["ServerSlots"]));
            xml.Append(xml_attr("ServerName", reader["ServerName"]));
            xml.Append(xml_attr("ServerPwd", reader["ServerPwd"]));
            xml.Append(xml_attr("ReservedSlots", reader["ReservedSlots"]));
            xml.Append(xml_attr("RentHours", reader["RentHours"]));
            xml.Append(xml_attr("WorkHours", reader["WorkHours"]));
            if (server)
            {
                xml.Append(xml_attr("OwnerCustomerID", reader["OwnerCustomerID"]));
            }
            xml.Append("/>");
        }
        xml.Append("</slist>");
    }

    void ServerGetList()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ServerGetList";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());

        if (!CallWOApi(sqcmd))
            return;

        StringBuilder xml = new StringBuilder();
        OutputXmlServerInfo(xml, reader, false);

        GResponse.Write(xml.ToString());
    }

    void ServerSetParams()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ServerChangeParams";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_GameServerID", web.Param("GSID"));
        sqcmd.Parameters.AddWithValue("@in_ServerPwd", web.Param("pwd"));
        sqcmd.Parameters.AddWithValue("@in_ServerFlags", web.Param("flags"));

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }
    
    void ServerSetParams2()
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ServerChangeParams2";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_GameServerID", web.Param("GSID"));
        sqcmd.Parameters.AddWithValue("@in_ServerPwd", web.Param("pwd"));
        sqcmd.Parameters.AddWithValue("@in_ServerFlags", web.Param("flags"));
        sqcmd.Parameters.AddWithValue("@in_GameTimeLimit", web.Param("timeLimit"));

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void SRV_ServerGetFullList()
    {
        string skey1 = web.Param("skey1");
        if (skey1 != SERVER_API_KEY)
            throw new ApiExitException("bad key");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ServerGetList";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", 0);

        if (!CallWOApi(sqcmd))
            return;

        StringBuilder xml = new StringBuilder();
        OutputXmlServerInfo(xml, reader, true);

        GResponse.Write(xml.ToString());
    }

    void SRV_ServerTick()
    {
        string skey1 = web.Param("skey1");
        if (skey1 != SERVER_API_KEY)
            throw new ApiExitException("bad key");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ServerTickHour";
        sqcmd.Parameters.AddWithValue("@in_GameServerID", web.Param("GSID"));

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    void SRV_ServerTickAll()
    {
        string skey1 = web.Param("skey1");
        if (skey1 != SERVER_API_KEY)
            throw new ApiExitException("bad key");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ServerTickAll";

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    protected override void Execute()
    {
        string func = web.Param("func");
        // server calls
        if (func == "srv_list")
        {
            SRV_ServerGetFullList();
            return;
        }
        else if (func == "srv_tick")
        {
            SRV_ServerTick();
            return;
        }
        else if (func == "srv_tickall")
        {
            SRV_ServerTickAll();
            return;
        }

        // client calls
        if (!WoCheckLoginSession())
            return;

        if (func == "prices")
            ServerGetPrices();
        else if (func == "rent")
            ServerRent();
        else if (func == "renew")
            ServerRenew();
        else if (func == "list")
            ServerGetList();
        else if (func == "setparams")
            ServerSetParams();
        else if (func == "setparams2")
            ServerSetParams2();
        else
            throw new ApiExitException("bad func");
    }
}
