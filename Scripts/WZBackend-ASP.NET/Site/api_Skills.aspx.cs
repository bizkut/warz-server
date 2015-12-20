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

public partial class api_Skills : WOApiWebPage
{
    // enum IDs from client UserSkills::ESkillIDs, used for prereq check
    public const int SKILL_Physical1 = 0;
    public const int SKILL_Weapons1 = 8;
    public const int SKILL_Repair1 = 15;
    public const int SKILL_Survival1 = 19;
    public const int SKILL_WeaponDegradation1 = 31;
    public const int SKILL_ID_END = 34;

    // vars for working with learning skill
    int SkillID = -1;
    int SkillLevel = 0;
    int[] CharSkills = new int[SKILL_ID_END];
    int CharXP = 0;
    int CharSpendXP = 0;
    int[] SkillCostsByLevel = { 0, 0, 0, 0, 0 };
    int SkillCost = 0;


    void ValidateSkillPrerequisite()
    {
        switch (SkillID)
        {
            default:
                // we must have level in previous tier of skill
                if (CharSkills[SkillID - 1] == 0)
                    throw new ApiExitException("previous skill level not learned");
                break;

            case SKILL_Physical1:
                break;

            case SKILL_Weapons1:
                break;

            case SKILL_Repair1:
                break;

            case SKILL_Survival1:
                break;

            case SKILL_WeaponDegradation1:
                if (CharSkills[SKILL_Weapons1+5] == 0)
                    throw new ApiExitException("weapon skill prereq");
                break;
        }
        return;
    }

    void GetCharSkills()
    {
        SkillID = web.GetInt("SkillID");
        SkillLevel = web.GetInt("SkillLevel");
        if (SkillID < 0 || SkillID >= SKILL_ID_END)
            throw new ApiExitException("bad SkillID");
        if (SkillLevel < 1 || SkillLevel > 5)
            throw new ApiExitException("bad SkillLevel");

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SkillsGetCharSkills";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_SkillID", SkillID);
        if (!CallWOApi(sqcmd))
            throw new ApiExitException("sql failed");

        // get character state
        reader.Read();
        string SkillsStr = getString("Skills").TrimEnd();
        CharXP = getInt("XP");
        CharSpendXP = getInt("SpendXP");

        // parse character skills
        for (int i = 0; i < SKILL_ID_END; i++)
        {
            CharSkills[i] = 0;
        }
        for (int i = 0; i < SkillsStr.Length; i++)
        {
            if (SkillsStr[i] >= '0' && SkillsStr[i] <= '9')
                CharSkills[i] = SkillsStr[i] - '0';
        }

        // must advance only one one level at time
        if (SkillLevel != CharSkills[SkillID] + 1)
            throw new ApiExitException("bad SkillLevel");

        // get skill prices data
        reader.NextResult();
        reader.Read();
        SkillCostsByLevel[0] = getInt("Lv1");
        SkillCostsByLevel[1] = getInt("Lv2");
        SkillCostsByLevel[2] = getInt("Lv3");
        SkillCostsByLevel[3] = getInt("Lv4");
        SkillCostsByLevel[4] = getInt("Lv5");

        SkillCost = SkillCostsByLevel[SkillLevel - 1];
        if (SkillCost <= 0)
            throw new ApiExitException("can't learn - no price");
    }

    void SetCharSkills()
    {
        // advance skill
        CharSkills[SkillID] = SkillLevel;

        // reassemble skill string
        string SkillsStr = "";
        for (int i = 0; i < SKILL_ID_END; i++)
        {
            SkillsStr += (char)(CharSkills[i] + '0');
        }
        SkillsStr = SkillsStr.TrimEnd("0".ToCharArray());

        SqlCommand sqcmd = new SqlCommand();
        sqcmd.CommandType = CommandType.StoredProcedure;
        sqcmd.CommandText = "WZ_SkillsSetCharSkills";
        sqcmd.Parameters.AddWithValue("@in_CustomerID", web.CustomerID());
        sqcmd.Parameters.AddWithValue("@in_CharID", web.Param("CharID"));
        sqcmd.Parameters.AddWithValue("@in_Skills", SkillsStr);
        sqcmd.Parameters.AddWithValue("@in_SkillCost", SkillCost);
        if (!CallWOApi(sqcmd))
            throw new ApiExitException("sql failed");
    }

    void SkillLearn()
    {
        GetCharSkills();

        // check if we can learn anything
        if (CharXP < 100)
            throw new ApiExitException("must have 100xp to learn skill");
        if (CharSpendXP + SkillCost > CharXP)
            throw new ApiExitException("not enough xp");

        ValidateSkillPrerequisite();

        // ok, advance skill and set it
        SetCharSkills();

        Response.Write("WO_0");
    }

    protected override void Execute()
    {
        if (!WoCheckLoginSession())
            return;

        string func = web.Param("func");
        if (func == "learn")
            SkillLearn();
        else
            throw new ApiExitException("bad func");
    }
}
