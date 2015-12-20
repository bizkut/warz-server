using System;
using System.Collections.Generic;
using System.Web;
using System.Configuration;

/// <summary>
/// Summary description for IPLocationCheck
/// </summary>
public class IPLocationCheck
{
    static string DB_PATH = null;
    static string LIC_PATH = null;

    static public string GetCountryCode(string strIPAddress)
    {
        if (DB_PATH == null)
        {
            string path = HttpContext.Current.Server.MapPath("~/App_Data");
            DB_PATH = path + "/IP-COUNTRY.BIN";
            LIC_PATH = path + "/License.key";
        }

        IP2Location.IPResult oIPResult = new IP2Location.IPResult();
        IP2Location.Component oIP2Location = new IP2Location.Component();
        try
        {
            //Set Database Paths
            oIP2Location.IPDatabasePath = DB_PATH;
            oIP2Location.IPLicensePath = LIC_PATH;

            oIPResult = oIP2Location.IPQuery(strIPAddress);
            switch (oIPResult.Status.ToString())
            {
                case "OK":
                    return oIPResult.CountryShort;
                default:
                    return null;
            }
        }
        catch (Exception ex)
        {
            throw new ApiExitException(ex.Message);
        }
        finally
        {
            oIPResult = null;
            oIP2Location = null;
        }
    }
}
