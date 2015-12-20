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

public partial class api_SteamBuyGP : WOApiWebPage
{
    bool ParseResponse(SteamXML.MicroTnxResponse resp)
    {
        if (resp.error != null)
            throw new ApiExitException(string.Format("resp.error: {0} {1}", resp.error.errorcode, resp.error.errordesc));
        
        if (resp.params_ == null)
            throw new ApiExitException("no resp.params");

        if (resp.result != "OK")
            throw new ApiExitException("bad resp.result " + resp.params_.status);

        return true;
    }

    void AuthTransaction()
    {
        string steamId = web.Param("steamId");

        int priceInCents = web.GetInt("price");
        if (priceInCents <= 0)
            throw new ApiExitException("bad price");

        int GP = priceInCents * api_GPConvert.GetGPConversionRateFromPrice(priceInCents, true) / 100;

        // get user wallet currency
        SteamApi api = new SteamApi();
        SteamXML.MicroTnxResponse resp = api.GetUserInfo(steamId);
        if (!ParseResponse(resp))
            return;
        if (resp.params_.currency == null)
            throw new ApiExitException("can't get wallet currency");
        if (resp.params_.country == null)
            throw new ApiExitException("can't get country");

        // start order
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SteamStartOrder";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_SteamID", steamId);
        sqcmd.Parameters.AddWithValue("@in_Price", priceInCents / 100.0);
        sqcmd.Parameters.AddWithValue("@in_GP", GP);
        sqcmd.Parameters.AddWithValue("@in_Currency", resp.params_.currency);
        sqcmd.Parameters.AddWithValue("@in_Country", resp.params_.country);
        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        string OrderID = getString("OrderID");
        string WalletCurrency = getString("Currency");
        int WalletPriceCents = getInt("PriceCents");
        if (WalletPriceCents == 0)
        {
            // we wasn't able to convert price
            Response.Write("WO_7can't convert");
            return;
        }

        // init steam transaction
        resp = api.InitTxn(
            OrderID, steamId,
            WalletCurrency, WalletPriceCents.ToString(),
            "1", string.Format("{0} Gold Credits", GP),
            "EN");
        //System.Diagnostics.Debug.WriteLine("@@@InitTxn: " + api.lastData_);

        if (resp.error != null && resp.error.errorcode == 8)
        {
            // 8 Transaction currency does not match user's Steam Wallet currency
            Response.Write("WO_7Steam8");
            return;
        }

        if (!ParseResponse(resp))
            return;

        Response.Write("WO_0");
    }

    void FinalizeTransaction()
    {
        string orderId = web.Param("orderId");
        string CustomerID = web.CustomerID();

        // check that transaction was approved and get it transactionID
        SteamApi api = new SteamApi();
        SteamXML.MicroTnxResponse resp = api.QueryTxn(orderId);
        //System.Diagnostics.Debug.WriteLine("@@@QueryTxn: " + api.lastData_);
        if (!ParseResponse(resp))
            return;
        if (resp.params_.status != "Approved")
        {
            Response.Write("WO_5");
            Response.Write(string.Format("bad status: {0}", resp.params_.status));
            return;
        }

        // finalize transaction in steam. NOTE: fucking steam give http error instead of valid answer here
        resp = api.FinalizeTxn(orderId);
        //System.Diagnostics.Debug.WriteLine("@@@FinalizeTxn: " + api.lastData_);
        if (!ParseResponse(resp))
            return;

        // note: we can't pass transaction id as bigint here, because stream using *unsigned* int64.
        string transid = resp.params_.transid;

        // finalize transaction in DB and get new balance
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SteamFinishOrder";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
        sqcmd.Parameters.AddWithValue("@in_OrderID", orderId);
        sqcmd.Parameters.AddWithValue("@in_transid", transid);
        if (!CallWOApi(sqcmd))
            return;

        // return new GP balance
        reader.Read();
        int balance = getInt("Balance");
        Response.Write("WO_0");
        Response.Write(balance.ToString());
    }

    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        string func = web.Param("func");
        if (func == "auth")
            AuthTransaction();
        else if (func == "fin")
            FinalizeTransaction();
        else
            throw new ApiExitException("bad func");
    }
}
