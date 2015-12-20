using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;
using System.Net.Mail;
using System.Configuration;

public partial class api_Login : WOApiWebPage
{
    void RegisterLoginIP(int CustomerID)
    {
        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ACCOUNT_RegisterLoginIP";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", CustomerID);
        sqcmd.Parameters.AddWithValue("@in_IP", LastIP);

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int IsNew = getInt("IsNew");
        string email = getString("email");
        if (IsNew == 0)
            return;

        // we have new ip, send email to devs about that
        string GeoCode = IPLocationCheck.GetCountryCode(LastIP);

        try
        {
            MailMessage mail = new MailMessage("loginwarning@thewarz.com", "ptumik@thewarinc.com");
            SmtpClient client = new SmtpClient();
            client.DeliveryMethod = SmtpDeliveryMethod.Network;
            client.Host = "smtp.mandrillapp.com";
            client.Port = 25;
            client.UseDefaultCredentials = false;
            client.Credentials = new System.Net.NetworkCredential("billing@playwarz.comm", "a216ff3e-8cee-431a-a111-3c151803ac5b");
            mail.Subject = "WarZ Developer Account Login IP detected";
            mail.Body = string.Format(
                "Developer Account {0} {1} was logged from IP {2}, country:{3}",
                CustomerID, email, LastIP, GeoCode);
            client.Send(mail);
        }
        catch (System.Exception ex)
        {
            throw new ApiExitException("can't send dev login email: " + ex.Message);
        }
    }

    void SendLockEmail(string email, string countryIP, string token)
    {
        //string url = "http://localhost:56016/Site/api_AccUnlock.aspx?token=" + token;
        string url = "http://202.162.78.185/WarZ/api/api_AccUnlock.aspx?token=" + token;

        string subj = "Infestation: Survivor Stories Account Lock Notice";
        string body = "";
        body += string.Format("Unusual login attempt was detected from {0} ({1}).\n", LastIP, countryIP);
        body += string.Format("To unlock your account please visit the following page: {0}\n", url);

        // check if we can read correct email
        try
        {
            string fname = HttpContext.Current.Server.MapPath("~/App_Data") + "/lock_email.html";
            body = System.IO.File.ReadAllText(fname);
            body = body.Replace("{IP}", string.Format("{0} ({1})", LastIP, countryIP));
            body = body.Replace("{URL}", url);
        }
        catch
        {
        }

        try
        {
            MailMessage mail = new MailMessage("support@202.162.78.185.com", email);
            SmtpClient client = new SmtpClient();
            client.DeliveryMethod = SmtpDeliveryMethod.Network;
            client.Host = "smtp.mandrillapp.com";
            client.Port = 25;
            client.UseDefaultCredentials = false;
            client.Credentials = new System.Net.NetworkCredential("billing@playwarz.comm", "a216ff3e-8cee-431a-a111-3c151803ac5b");
            mail.IsBodyHtml = true;
            mail.Subject = subj;
            mail.Body = body;
            client.Send(mail);
        }
        catch
        {
            // fail silently, shit happens. 
            // throw new ApiExitException("can't send dev login email: " + ex.Message);
        }
    }

    protected override void Execute()
    {
        string username = web.Param("username");
        string password = web.Param("password");

        string countryIP = "";
        if (!String.IsNullOrEmpty(Request["HTTP_CF_IPCOUNTRY"]))
            countryIP = Request["HTTP_CF_IPCOUNTRY"];            

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ACCOUNT_LOGIN";
        sqcmd.Parameters.AddWithValue("@in_IP", LastIP);
        sqcmd.Parameters.AddWithValue("@in_EMail", username);
        sqcmd.Parameters.AddWithValue("@in_Password", password);
        sqcmd.Parameters.AddWithValue("@in_Country", countryIP);

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int CustomerID = getInt("CustomerID");
        int AccountStatus = getInt("AccountStatus");
        int SessionID = 0;
        int IsDeveloper = 0;

        if (CustomerID > 0)
        {
            SessionID = getInt("SessionID");
            IsDeveloper = getInt("IsDeveloper");

            // if this is a steam user, check if he own game
            string SteamUserID = getString("SteamUserID");
            if (SteamUserID != "0")
            {
                SteamApi api = new SteamApi();
                bool Have_Game = api.CheckAppOwnership(SteamUserID, "226700"); // base game
                if (!Have_Game)
                {
                    // special 1001 code for running under steam but without game.
                    Response.Write("WO_0");
                    Response.Write(string.Format("{0} {1} {2}",
                        0, 0, 1001));
                    return;
                }
            }

            if (AccountStatus == 103)
            {
                // first time account lock, send email and override status to normal lock
                AccountStatus = 102;
                string LockToken = getString("LockToken");
                SendLockEmail(username, countryIP, LockToken);
            }

            if (IsDeveloper > 0)
            {
                RegisterLoginIP(CustomerID);
            }
        }

        GResponse.Write("WO_0");
        GResponse.Write(string.Format("{0} {1} {2}",
            CustomerID, SessionID, AccountStatus));
    }
}
