using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Data;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Data.SqlClient;

public partial class api_ClanInvites : WOApiWebPage
{
    // send invite from clan to player 
    void SendInvite()
    {
        string Gamertag = web.Param("Gamertag");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanInviteSendToPlayer";
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_InvGamertag", Gamertag);

        if (!CallWOApi(sqcmd))
            return;

        Response.Write("WO_0");
    }

    // player accept invite from clan
    void AnswerInvite()
    {
        string InviteID = web.Param("InviteID");
        string Answer = web.Param("Answer");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_ClanInviteAnswer";
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_ClanInviteID", InviteID);
        sqcmd.Parameters.AddWithValue("@in_Answer", Answer);

        if (!CallWOApi(sqcmd))
            return;

        reader.Read();
        int ClanID = getInt("ClanID");

        Response.Write("WO_0");
        Response.Write(string.Format("{0}", ClanID));
    }

    // give player list of all pending invites from clans
    void CheckInvites()
    {
        // not used now, we'll get invites in login session poller in client
        throw new ApiExitException("use api_LoginSessionPoller::ClanGetInvites");
    }

    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        string func = web.Param("func");
        if (func == "check")
            CheckInvites();
        else if (func == "send")
            SendInvite();
        else if (func == "answer")
            AnswerInvite();
        else
            throw new ApiExitException("bad func");
    }
}
