/*
 * tele_npc_vote - teleporter / utility NPC that unlocks most features after voting.
 *
 * Ported from the 2011-2013 era ScriptPCH / CreatureScript::OnGossipHello style to the
 * current TrinityCore 3.3.5 branch, where gossip is handled by CreatureAI.
 *
 * DB:  creature_template.ScriptName = 'tele_npc_vote'  (unchanged)
 *      npcflag must include gossip (1).
 *
 * Vote data comes from the `voting_points` table in the auth database (id = account id).
 * Player::HasVoted() / Player::GetGuildHouseMask() were custom core code that stock
 * TrinityCore does not have, so they are re-implemented as SQL in namespace TeleVote.
 * Guild house mask comes from `guild_house_masks` in the characters database
 * (guid = guild id, phasemask = the guild's phase; > 1 means the guild owns a house).
 *
 * NOTE: the old core also applied that phase mask to the player on map 37. This script only
 * uses it to decide who sees the Guild House menu - it does not set any phase itself.
 */

#include "ScriptMgr.h"
#include "Chat.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GossipDef.h"
#include "InstanceSaveMgr.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellHistory.h"
#include "WorldSession.h"

namespace TeleVote
{
    constexpr uint32 INSTANCE_RESET_VP_COST = 2;        // vote points
    constexpr uint32 COOLDOWN_RESET_COST    = 1000000;  // copper (100 gold)

    // ASSUMPTION: "has voted" = the account's last vote (voting_points.time) is newer than this.
    // 12h matches the cooldown of most vote sites - change it to match yours.
    constexpr uint32 VOTE_VALID_SECONDS     = 12 * 3600;

    constexpr char const* MSG_NOT_VOTED      = "You have not voted yet. Vote to unlock dungeons, player tools, PvP and the guild house.";
    constexpr char const* MSG_NO_GUILD_HOUSE = "Your guild does not own a guild house.";

    // voting_points (auth DB): id = account id, points = vote point balance, time = unix time of last vote
    #define TELEVOTE_SQL_HAS_VOTED    "SELECT 1 FROM `voting_points` WHERE `id` = {} AND `time` > UNIX_TIMESTAMP() - {} LIMIT 1"
    #define TELEVOTE_SQL_HAS_POINTS   "SELECT 1 FROM `voting_points` WHERE `id` = {} AND `points` >= {} LIMIT 1"
    #define TELEVOTE_SQL_SPEND_POINTS "UPDATE `voting_points` SET `points` = `points` - {} WHERE `id` = {} AND `points` >= {}"

    // guild_house_masks (characters DB): guid = guild id, phasemask = guild house phase (> 1 = has a house)
    #define TELEVOTE_SQL_GUILD_MASK   "SELECT `phasemask` FROM `guild_house_masks` WHERE `guid` = {}"

    bool HasVoted(Player* player)
    {
        QueryResult result = LoginDatabase.PQuery(TELEVOTE_SQL_HAS_VOTED, player->GetSession()->GetAccountId(), VOTE_VALID_SECONDS);
        return bool(result);
    }

    bool TryChargeVotePoints(Player* player, uint32 cost)
    {
        uint32 account = player->GetSession()->GetAccountId();

        QueryResult result = LoginDatabase.PQuery(TELEVOTE_SQL_HAS_POINTS, account, cost);
        if (!result)
            return false;

        // the "points >= cost" guard in the UPDATE keeps the balance from going negative
        LoginDatabase.PExecute(TELEVOTE_SQL_SPEND_POINTS, cost, account, cost);
        return true;
    }

    uint32 GetGuildHouseMask(Player* player)
    {
        if (!player->GetGuildId())
            return 0;

        QueryResult result = CharacterDatabase.PQuery(TELEVOTE_SQL_GUILD_MASK, uint32(player->GetGuildId()));
        return result ? (*result)[0].GetUInt32() : 0;
    }
}

struct TeleLocation
{
    uint32 action;
    uint32 mapId;
    float x, y, z, o;
};

// action id (same ids as the old script's gossip actions) -> destination
static constexpr TeleLocation TeleLocations[] =
{
    // ---- Alliance ----
    {  1001,   1, 9947.52f, 2482.73f, 1316.21f, 0.0f }, // Darnassus
    {  1005, 530, -3954.20f, -11656.54f, -138.69f, 0.0f }, // Exodar
    {  1010,   0, -4924.07f, -951.95f, 501.55f, 5.40f }, // Ironforge
    {  1015,   0, -8960.14f, 516.266f, 96.3568f, 0.0f }, // Stormwind
    // ---- Horde ----
    {  2001,   1, 1552.5f, -4420.66f, 8.94802f, 0.0f }, // Orgrimmar
    {  2005, 530, 9338.74f, -7277.27f, 13.7895f, 0.0f }, // Silvermoon
    {  2010,   1, -1290.0f, 147.033997f, 129.682007f, 4.919000f }, // Thunder Bluff
    {  2015,   0, 1819.71f, 238.79f, 60.5321f, 0.0f }, // Undercity
    // ---- Neutral ----
    {  3001,   0, -14438.2f, 472.22f, 15.32f, 0.65f }, // Booty Bay
    {  3005, 571, 5819.08f, 643.99f, 647.80f, 3.32f }, // Dalaran
    {  3010,   1, 6724.77f, -4610.68f, 720.78f, 4.78f }, // Everlook
    {  3015,   1, -7173.26f, -3785.60f, 8.37f, 6.13f }, // Gadgetzan
    {  3020,   1, -4564.79f, -3172.38f, 33.93f, 3.21f }, // Mudsprocket
    {  3025,   1, -956.664f, -3754.71f, 5.33239f, 0.996637f }, // Ratchet
    {  3030, 530, -1850.209961f, 5435.821777f, -10.961435f, 3.403913f }, // Shattrath City
    {  3035, 530, 12947.4f, -6893.31f, 5.68398f, 3.09154f }, // Isle Of Quel'Danas
    // ---- Kalimdor ----
    {  6001,   1, 4248.72f, 744.35f, -24.67f, 1.34f }, // Blackfathom Deeps
    {  6005,   1, -8173.66f, -4746.36f, 33.84f, 4.94f }, // Caverns of Time
    {  6010,   1, -3960.95f, 1130.64f, 161.05f, 0.0f }, // Dire Maul
    {  6015,   1, -1431.33f, 2962.34f, 98.23f, 4.74f }, // Maraudon
    {  6020,   1, -4707.44f, -3726.82f, 54.6723f, 3.8f }, // Onyxia's Lair
    {  6025,   1, 1814.47f, -4419.46f, -18.78f, 5.28f }, // Ragefire Chasm
    {  6030,   1, -4657.88f, -2525.59f, 81.4f, 4.16f }, // Razorfen Downs
    {  6035,   1, -4463.6f, -1664.53f, 82.26f, 0.85f }, // Razorfen Kraul
    {  6040,   1, -8413.33f, 1501.27f, 29.64f, 2.61f }, // Ruins of Ahn'Qiraj
    {  6045,   1, -8245.837891f, 1983.736206f, 129.071686f, 0.936195f }, // Temple of Ahn'Qiraj
    {  6050,   1, -722.53f, -2226.30f, 16.94f, 2.71f }, // Wailing Caverns
    {  6055,   1, -6801.9f, -2890.22f, 8.88f, 6.25f }, // Zul'Farrak
    // ---- Eastern Kingdoms ----
    {  7001,   0, -7180.57f, -920.04f, 165.49f, 5.02f }, // Blackrock Depths
    {  7005,   0, -7526.77f, -1225.64f, 285.73f, 5.31f }, // Blackrock Spire
    {  7010, 469, -7672.61f, -1107.21f, 396.65f, 3.75f }, // Blackwing Lair
    {  7015,   0, -11208.2f, 1675.92f, 24.57f, 1.48f }, // Deadmines
    {  7020,   0, -5163.32f, 927.18f, 257.158f, 1.44f }, // Gnomeregan
    {  7025, 530, 13010.00f, -6907.64f, 9.58f, 4.58f }, // Isle Of Quel'Danas
    {  7030,   0, -11119.6f, -2011.42f, 47.09f, 0.65f }, // Karazhan
    {  7035, 230, 1114.85f, -457.76f, -102.81f, 3.83f }, // Molten Core
    {  7040,   0, 2843.89f, -693.74f, 139.32f, 5.11f }, // Scarlet Monastery
    {  7045,   0, 1273.06f, -2574.01f, 92.66f, 2.06f }, // Scholomance
    {  7050,   0, -239.54f, 1550.8f, 76.89f, 1.18f }, // Shadowfang Keep
    {  7055,   0, 3370.76f, -3343.63f, 142.26f, 5.23f }, // Stratholme
    {  7060,   0, -10346.92f, -3851.90f, -43.41f, 6.09f }, // Sunken Temple
    {  7065,   0, -8766.89f, 844.6f, 88.43f, 0.69f }, // The Stockade
    {  7070,   0, -6070.72f, -2955.33f, 209.78f, 0.05f }, // Uldaman
    {  7075, 530, 6851.09f, -7979.71f, 183.54f, 4.72f }, // Zul'Aman
    {  7080,   0, -11916.4f, -1216.23f, 92.28f, 4.75f }, // Zul'Gurub
    // ---- Outland ----
    {  8001, 530, -3322.92f, 4931.02f, -100.56f, 1.86f }, // Auchindoun
    {  8005, 530, -3649.1f, 317.33f, 35.19f, 2.97f }, // Black Temple
    {  8010, 530, 721.08f, 6846.77f, -68.75f, 0.34f }, // Coilfang Reservoir
    {  8015, 530, 3539.01f, 5082.36f, 1.69f, 0.0f }, // Gruul's Lair
    {  8020, 530, -292.71f, 3146.77f, 31.60f, 2.05f }, // Hellfire Citadel
    {  8025, 530, 3087.62f, 1376.27f, 184.8f, 4.63f }, // Tempest Keep
    // ---- Northrend ----
    {  9001, 571, 3646.76f, 2045.17f, 1.79f, 4.37f }, // Ahn'Kahet
    {  9005, 571, 3675.43f, 2169.00f, 35.90f, 2.29f }, // Azjol-Nerub
    {  9010, 571, 4450.86f, -2045.25f, 162.83f, 0.00f }, // Drak'Tharon Keep
    {  9015, 571, 6937.54f, -4455.98f, 450.68f, 1.00f }, // Gun'Drak
    {  9020, 571, 6139.24f, 2233.72f, 510.79f, 3.56f }, // Icecrown Citadel
    {  9025, 571, 3668.72f, -1262.46f, 243.622f, 4.785f }, // Naxxramas
    {  9030, 571, 3789.95f, 6961.48f, 104.23f, 0.28f }, // The Nexus
    {  9035, 571, 3625.78f, 280.4f, -120.14f, 3.25f }, // The Obsidian Sanctum
    {  9040, 571, 1219.72f, -4865.28f, 41.25f, 0.31f }, // Utgarde Keep
    {  9045, 571, 1245.69f, -4856.59f, 216.86f, 3.45f }, // Utgarde Pinnacle
    {  9050, 571, 8976.24f, -1281.33f, 1059.01f, 0.58f }, // Ulduar
    {  9055, 571, 5679.82f, 486.80f, 652.40f, 4.08f }, // Violet Hold
    {  9060, 571, 5440.11f, 2840.76f, 420.42f, 0.01f }, // Vault of Archavon
    // ---- Guild house ----
    { 10035,  37, 1009.9f, 284.2f, 328.96f, 0.17f }, // Guild Hall
    { 10040,  37, 779.7f, 152.2f, 269.5f, 1.770f }, // Guild Arena
    { 10045,  37, 486.89f, 58.80f, 262.38f, 3.875f }, // Summit Course
};

static TeleLocation const* FindLocation(uint32 action)
{
    for (TeleLocation const& loc : TeleLocations)
        if (loc.action == action)
            return &loc;
    return nullptr;
}

static void Tell(Player* player, char const* text)
{
    ChatHandler(player->GetSession()).SendSysMessage(text);
}

struct tele_npc_voteAI : public ScriptedAI
{
    tele_npc_voteAI(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipHello(Player* player) override
    {
        ClearGossipMenuFor(player);

        // remove stealth, vanish, prowl
        for (uint32 spellId : { 1784u, 26889u, 5215u })
            player->RemoveAurasDueToSpell(spellId);

        bool voted = TeleVote::HasVoted(player);
        if (!voted)
            Tell(player, TeleVote::MSG_NOT_VOTED);

        ShowMainMenu(player, voted);
        return true;
    }

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
    {
        uint32 sender = player->PlayerTalkClass->GetGossipOptionSender(gossipListId);
        uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
        ClearGossipMenuFor(player);

        if (sender == GOSSIP_SENDER_MAIN)
            HandleAction(player, action);

        return true;
    }

private:
    void ShowMainMenu(Player* player, bool voted)
    {
        bool alliance = player->GetTeam() == ALLIANCE;

        if (alliance)
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Achievement_PVP_A_16:35:35:-22:0|t|cff0000ffAlliance Town ->|r", GOSSIP_SENDER_MAIN, 1000);
        else
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Achievement_PVP_H_16:35:35:-22:0|t|cff0000ffHorde Town ->|r", GOSSIP_SENDER_MAIN, 2000);

        AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Spell_Arcane_TeleportShattrath:35:35:-22:0|t|cff0000ffNeutral Town ->|r", GOSSIP_SENDER_MAIN, 3000);

        if (voted)
        {
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Spell_Holy_EmpowerChampion:35:35:-22:0|t|cff0000ffDungeons ->|r", GOSSIP_SENDER_MAIN, 5000);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\INV_Misc_Gear_01:35:35:-22:0|t|cff0000ffPlayer Tools ->|r", GOSSIP_SENDER_MAIN, 4000);
            // the PvP entry shows the enemy faction's icon, as in the original
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, alliance
                ? "|TInterface\\Icons\\Achievement_PVP_H_16:35:35:-22:0|t|cff0000ffAlliance vs Horde PvP|r"
                : "|TInterface\\Icons\\Achievement_PVP_A_16:35:35:-22:0|t|cff0000ffAlliance vs Horde PvP|r", GOSSIP_SENDER_MAIN, 3500);
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface\\Icons\\INV_Misc_GroupLooking:35:35:-22:0|t|cff0000ffGuild House Menu|r", GOSSIP_SENDER_MAIN, 10010);
        }

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
    }

    void ResetInstanceLocks(Player* player)
    {
        // never touch the bind of the map the player is standing in (same rule as .instance unbind)
        uint32 count = 0;
        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
            for (auto const& [mapId, bind] : player->GetBoundInstances(Difficulty(i)))
                if (mapId != player->GetMapId())
                    ++count;

        if (!count)
        {
            Tell(player, "You have no instance locks that can be reset.");
            return;
        }

        if (!TeleVote::TryChargeVotePoints(player, TeleVote::INSTANCE_RESET_VP_COST))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You need %u vote points to reset your instance locks.", TeleVote::INSTANCE_RESET_VP_COST);
            return;
        }

        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
            Player::BoundInstancesMap& binds = player->GetBoundInstances(Difficulty(i));
            for (Player::BoundInstancesMap::iterator itr = binds.begin(); itr != binds.end();)
            {
                if (itr->first != player->GetMapId())
                    player->UnbindInstance(itr, Difficulty(i));
                else
                    ++itr;
            }
        }

        Tell(player, "Your instance locks have been reset.");
    }

    void HandleAction(Player* player, uint32 action)
    {
        // not allowed in combat
        if (player->IsInCombat())
        {
            CloseGossipMenuFor(player);
            me->Say("You are in combat!", LANG_UNIVERSAL, player);
            return;
        }

        // every plain teleport
        if (TeleLocation const* loc = FindLocation(action))
        {
            CloseGossipMenuFor(player);
            player->TeleportTo(loc->mapId, loc->x, loc->y, loc->z, loc->o);
            return;
        }

        switch (action)
        {
            case 1000: // Alliance Town
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Leader_Tyrande_Whisperwind:35:35:-22:0|t|cff0000ffDarnassus|r", GOSSIP_SENDER_MAIN, 1001);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Leader_Prophet_Velen:35:35:-22:0|t|cff0000ffExodar|r", GOSSIP_SENDER_MAIN, 1005);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Leader_King_Magni_Bronzebeard:35:35:-22:0|t|cff0000ffIronforge|r", GOSSIP_SENDER_MAIN, 1010);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Leader_King_Varian_Wrynn:35:35:-22:0|t|cff0000ffStormwind|r", GOSSIP_SENDER_MAIN, 1015);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- Main Menu|r", GOSSIP_SENDER_MAIN, 5005);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 2000: // Horde Town
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Leader_Thrall:35:35:-22:0|t|cff0000ffOrgrimmar|r", GOSSIP_SENDER_MAIN, 2001);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Leader_Lorthemar_Theron:35:35:-22:0|t|cff0000ffSilvermoon|r", GOSSIP_SENDER_MAIN, 2005);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Leader_Cairne_Bloodhoof:35:35:-22:0|t|cff0000ffThunder Bluff|r", GOSSIP_SENDER_MAIN, 2010);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Leader_Sylvanas:35:35:-22:0|t|cff0000ffUndercity|r", GOSSIP_SENDER_MAIN, 2015);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- Main Menu|r", GOSSIP_SENDER_MAIN, 5005);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 3000: // Neutral Town
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Zone_Stranglethorn_01:35:35:-22:0|t|cff0000ffBooty Bay|r", GOSSIP_SENDER_MAIN, 3001);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Spell_Arcane_TeleportDalaran:35:35:-22:0|t|cff0000ffDalaran|r", GOSSIP_SENDER_MAIN, 3005);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Zone_Winterspring:35:35:-22:0|t|cff0000ffEverlook|r", GOSSIP_SENDER_MAIN, 3010);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Zone_Tanaris_01:35:35:-22:0|t|cff0000ffGadgetzan|r", GOSSIP_SENDER_MAIN, 3015);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Zone_DustwallowMarsh:35:35:-22:0|t|cff0000ffMudsprocket|r", GOSSIP_SENDER_MAIN, 3020);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Zone_Barrens_01:35:35:-22:0|t|cff0000ffRatchet|r", GOSSIP_SENDER_MAIN, 3025);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Zone_Terrokar:35:35:-22:0|t|cff0000ffShattrath City|r", GOSSIP_SENDER_MAIN, 3030);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Zone_IsleOfQuelDanas:35:35:-22:0|t|cff0000ffIsle Of Quel'Danas|r", GOSSIP_SENDER_MAIN, 3035);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- Main Menu|r", GOSSIP_SENDER_MAIN, 5005);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 5000: // Dungeons
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Achievement_Zone_Kalimdor_01:35:35:-22:0|t|cff0000ffKalimdor ->|r", GOSSIP_SENDER_MAIN, 5010);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Achievement_Zone_EasternKingdoms_01:35:35:-22:0|t|cff0000ffEastern Kingdoms ->|r", GOSSIP_SENDER_MAIN, 5015);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Achievement_Zone_Outland_01:35:35:-22:0|t|cff0000ffOutland ->|r", GOSSIP_SENDER_MAIN, 5025);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Achievement_Zone_Northrend_01:35:35:-22:0|t|cff0000ffNorthrend ->|r", GOSSIP_SENDER_MAIN, 5030);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- Main Menu|r", GOSSIP_SENDER_MAIN, 5005);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 5010: // Kalimdor
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Bazil_Akumai:35:35:-22:0|t|cff0000ffBlackfathom Deeps|r", GOSSIP_SENDER_MAIN, 6001);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_CoTStratholme:35:35:-22:0|t|cff0000ffCaverns of Time|r", GOSSIP_SENDER_MAIN, 6005);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Ability_Warrior_DecisiveStrike:35:35:-22:0|t|cff0000ffDire Maul|r", GOSSIP_SENDER_MAIN, 6010);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_PrincessTheradras:35:35:-22:0|t|cff0000ffMaraudon|r", GOSSIP_SENDER_MAIN, 6015);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Onyxia:35:35:-22:0|t|cff0000ffOnyxia's Lair|r", GOSSIP_SENDER_MAIN, 6020);
                if (player->GetTeam() == HORDE)
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Spell_Shadow_SummonFelGuard:35:35:-22:0|t|cff0000ffRagefire Chasm|r", GOSSIP_SENDER_MAIN, 6025);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Amnennar_the_Coldbringer:35:35:-22:0|t|cff0000ffRazorfen Downs|r", GOSSIP_SENDER_MAIN, 6030);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_CharlgaRazorflank:35:35:-22:0|t|cff0000ffRazorfen Kraul|r", GOSSIP_SENDER_MAIN, 6035);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_OssirianTheUnscarred:35:35:-22:0|t|cff0000ffRuins of Ahn'Qiraj|r", GOSSIP_SENDER_MAIN, 6040);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_CThun:35:35:-22:0|t|cff0000ffTemple of Ahn'Qiraj|r", GOSSIP_SENDER_MAIN, 6045);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Mutanus_the_Devourer:35:35:-22:0|t|cff0000ffWailing Caverns|r", GOSSIP_SENDER_MAIN, 6050);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_ChiefUkorzSandscalp:35:35:-22:0|t|cff0000ffZul'Farrak|r", GOSSIP_SENDER_MAIN, 6055);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- Back|r", GOSSIP_SENDER_MAIN, 5000);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 5015: // Eastern Kingdoms 1
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_EmperorDagranThaurissan:35:35:-22:0|t|cff0000ffBlackrock Depths|r", GOSSIP_SENDER_MAIN, 7001);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_GeneralDrakkisath:35:35:-22:0|t|cff0000ffBlackrock Spire|r", GOSSIP_SENDER_MAIN, 7005);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Nefarion:35:35:-22:0|t|cff0000ffBlackwing Lair|r", GOSSIP_SENDER_MAIN, 7010);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_EdwinVancleef:35:35:-22:0|t|cff0000ffDeadmines|r", GOSSIP_SENDER_MAIN, 7015);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Mekgineer_Thermaplugg:35:35:-22:0|t|cff0000ffGnomeregan|r", GOSSIP_SENDER_MAIN, 7020);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Zone_IsleOfQuelDanas:35:35:-22:0|t|cff0000ffIsle Of Quel'Danas|r", GOSSIP_SENDER_MAIN, 7025);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_PrinceMalchezaar_02:35:35:-22:0|t|cff0000ffKarazhan|r", GOSSIP_SENDER_MAIN, 7030);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Ragnaros:35:35:-22:0|t|cff0000ffMolten Core|r", GOSSIP_SENDER_MAIN, 7035);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\INV_Misc_Gear_01:35:35:-22:0|t|cff0000ffMore ->|r", GOSSIP_SENDER_MAIN, 5020);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- Back|r", GOSSIP_SENDER_MAIN, 5000);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 5020: // Eastern Kingdoms 2
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\INV_Helmet_52:35:35:-22:0|t|cff0000ffScarlet Monastery|r", GOSSIP_SENDER_MAIN, 7040);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Spell_Holy_SenseUndead:35:35:-22:0|t|cff0000ffScholomance|r", GOSSIP_SENDER_MAIN, 7045);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_ArchmageArugal:35:35:-22:0|t|cff0000ffShadowfang Keep|r", GOSSIP_SENDER_MAIN, 7050);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_CoTStratholme:35:35:-22:0|t|cff0000ffStratholme|r", GOSSIP_SENDER_MAIN, 7055);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_ShadeOfEranikus:35:35:-22:0|t|cff0000ffSunken Temple|r", GOSSIP_SENDER_MAIN, 7060);
                if (player->GetTeam() == ALLIANCE)
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Bazil_Thredd:35:35:-22:0|t|cff0000ffThe Stockade|r", GOSSIP_SENDER_MAIN, 7065);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Archaedas:35:35:-22:0|t|cff0000ffUldaman|r", GOSSIP_SENDER_MAIN, 7070);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Zuljin:35:35:-22:0|t|cff0000ffZul'Aman|r", GOSSIP_SENDER_MAIN, 7075);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Hakkar:35:35:-22:0|t|cff0000ffZul'Gurub|r", GOSSIP_SENDER_MAIN, 7080);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\INV_Misc_Gear_01:35:35:-22:0|t|cff0000ff<- Back|r", GOSSIP_SENDER_MAIN, 5015);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- Main Menu|r", GOSSIP_SENDER_MAIN, 5005);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 5025: // Outland
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\INV_Mount_Raven_54:35:35:-22:0|t|cff0000ffAuchindoun|r", GOSSIP_SENDER_MAIN, 8001);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Illidan:35:35:-22:0|t|cff0000ffBlack Temple|r", GOSSIP_SENDER_MAIN, 8005);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\ACHIEVEMENT_BOSS_LADYVASHJ:35:35:-22:0|t|cff0000ffCoilfang Reservoir|r", GOSSIP_SENDER_MAIN, 8010);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_GruulTheDragonkiller:35:35:-22:0|t|cff0000ffGruul's Lair|r", GOSSIP_SENDER_MAIN, 8015);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Boss_Magtheridon:35:35:-22:0|t|cff0000ffHellfire Citadel|r", GOSSIP_SENDER_MAIN, 8020);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Character_Bloodelf_Male:35:35:-22:0|t|cff0000ffTempest Keep|r", GOSSIP_SENDER_MAIN, 8025);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- Back|r", GOSSIP_SENDER_MAIN, 5000);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 5030: // Northrend
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_AzjolLowercity_Normal:35:35:-22:0|t|cff0000ffAhn'Kahet|r", GOSSIP_SENDER_MAIN, 9001);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_AzjolUppercity_Normal:35:35:-22:0|t|cff0000ffAzjol-Nerub|r", GOSSIP_SENDER_MAIN, 9005);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_Drak'Tharon_Normal:35:35:-22:0|t|cff0000ffDrak'Tharon Keep|r", GOSSIP_SENDER_MAIN, 9010);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_Gundrak_Normal:35:35:-22:0|t|cff0000ffGun'Drak|r", GOSSIP_SENDER_MAIN, 9015);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_Icecrown_IcecrownEntrance:35:35:-22:0|t|cff0000ffIcecrown Citadel|r", GOSSIP_SENDER_MAIN, 9020);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_Naxxramas_Normal:35:35:-22:0|t|cff0000ffNaxxramas|r", GOSSIP_SENDER_MAIN, 9025);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_Nexus70_Normal:35:35:-22:0|t|cff0000ffThe Nexus|r", GOSSIP_SENDER_MAIN, 9030);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_CoABlackDragonflight_Heroic:35:35:-22:0|t|cff0000ffThe Obsidian Sanctum|r", GOSSIP_SENDER_MAIN, 9035);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_UtgardeKeep_Normal:35:35:-22:0|t|cff0000ffUtgarde Keep|r", GOSSIP_SENDER_MAIN, 9040);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_UtgardePinnacle_Heroic:35:35:-22:0|t|cff0000ffUtgarde Pinnacle|r", GOSSIP_SENDER_MAIN, 9045);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_UlduarRaid_Archway_01:35:35:-22:0|t|cff0000ffUlduar|r", GOSSIP_SENDER_MAIN, 9050);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Dungeon_TheVioletHold_Normal:35:35:-22:0|t|cff0000ffViolet Hold|r", GOSSIP_SENDER_MAIN, 9055);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\INV_Misc_Statue_07:35:35:-22:0|t|cff0000ffVault of Archavon|r", GOSSIP_SENDER_MAIN, 9060);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- Back|r", GOSSIP_SENDER_MAIN, 5000);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 4000: // Player Tools
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Spell_Holy_LayOnHands:35:35:-22:0|t|cff0000ffHeal Me!|r", GOSSIP_SENDER_MAIN, 10030);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_General:35:35:-22:0|t|cff0000ffReset Talent Points|r", GOSSIP_SENDER_MAIN, 4010);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Ability_Hunter_BeastTraining:35:35:-22:0|t|cff0000ffReset Pet Talent Points|r", GOSSIP_SENDER_MAIN, 4015);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Spell_Holy_GuardianSpirit:35:35:-22:0|t|cff0000ffRemove Resurrect Sickness|r", GOSSIP_SENDER_MAIN, 4020);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\INV_Misc_Key_10:35:35:-22:0|t|cff0000ffReset Instance Locks Cost: [2 VP]|r", GOSSIP_SENDER_MAIN, 10015);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Ability_Rogue_Sprint:35:35:-22:0|t|cff0000ffReset Cooldowns Cost: [100 Gold]|r", GOSSIP_SENDER_MAIN, 10025);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- [Main Menu]|r", GOSSIP_SENDER_MAIN, 5005);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 10015: // Reset instance locks (confirm)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\INV_Misc_Coin_01:35:35:-22:0|t|cff0000ffYes, at a Cost of 2 VP.|r", GOSSIP_SENDER_MAIN, 10020);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ffNo, Return to Main Menu.|r", GOSSIP_SENDER_MAIN, 5005);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                break;

            case 5005: // Back to main menu
                ShowMainMenu(player, TeleVote::HasVoted(player));
                break;

            case 10010: // Guild House Menu
                if (TeleVote::GetGuildHouseMask(player) > 1)
                {
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\INV_Misc_GroupLooking:35:35:-22:0|t|cff0000ffGuild Hall|r", GOSSIP_SENDER_MAIN, 10035);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\INV_Misc_ArmorKit_17:35:35:-22:0|t|cff0000ffGuild Arena|r", GOSSIP_SENDER_MAIN, 10040);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|TInterface\\Icons\\Achievement_Arena_2v2_7:35:35:-22:0|t|cff0000ffSummit Course|r", GOSSIP_SENDER_MAIN, 10045);
                    AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface\\Icons\\Ability_Druid_Flightform:35:35:-22:0|t|cff0000ff<- [Main Menu]|r", GOSSIP_SENDER_MAIN, 5005);
                    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
                }
                else
                {
                    CloseGossipMenuFor(player);
                    Tell(player, TeleVote::MSG_NO_GUILD_HOUSE);
                }
                break;

            ////////////////////////////////////////// Player tools //////////////////////////////////////////

            case 4005: // Innkeeper: bind your home (not shown in any menu)
                CloseGossipMenuFor(player);
                player->SetBindPoint(me->GetGUID());
                break;

            case 4010: // Reset talent points
                CloseGossipMenuFor(player);
                player->SendRespecWipeConfirm(me->GetGUID(), player->GetNextResetTalentsCost());
                break;

            case 4015: // Reset pet talent points
                CloseGossipMenuFor(player);
                if (player->GetPet())
                    player->ResetPetTalents();
                else
                    me->Whisper("You don't have a pet.", LANG_UNIVERSAL, player);
                break;

            case 4020: // Remove resurrection sickness
                if (!player->HasAura(15007))
                {
                    me->Whisper("You don't have resurrection sickness.", LANG_UNIVERSAL, player);
                    OnGossipHello(player);
                    return;
                }
                me->CastSpell(player, 38588, false); // healing effect
                player->RemoveAurasDueToSpell(15007);
                CloseGossipMenuFor(player);
                break;

            case 10020: // Reset instance locks (confirmed)
                CloseGossipMenuFor(player);
                ResetInstanceLocks(player);
                break;

            case 10025: // Reset cooldowns
                if (player->GetMoney() < TeleVote::COOLDOWN_RESET_COST)
                    Tell(player, "You do not have enough gold.");
                else
                {
                    player->ModifyMoney(-int32(TeleVote::COOLDOWN_RESET_COST));
                    player->GetSpellHistory()->ResetAllCooldowns();
                    Tell(player, "Your cooldowns have been reset.");
                }
                CloseGossipMenuFor(player);
                break;

            case 10030: // Heal me
                if (player->IsAlive())
                {
                    player->SetFullHealth();
                    for (Powers power : { POWER_MANA, POWER_ENERGY })
                        player->SetPower(power, player->GetMaxPower(power));
                }
                else
                    player->ResurrectPlayer(1.0f);
                CloseGossipMenuFor(player);
                break;

            case 3500: // Alliance vs Horde PvP
                CloseGossipMenuFor(player);
                if (player->GetTeam() == ALLIANCE) // Razor Hill for Alliance
                    player->TeleportTo(1, 315.721f, -4743.4f, 10.4867f, 0.0f);
                else                               // Goldshire for Horde
                    player->TeleportTo(0, -9464.0f, 62.0f, 56.0f, 0.0f);
                break;

            default:
                CloseGossipMenuFor(player);
                break;
        }
    }
};

void AddSC_tele_npc_vote()
{
    // registered under the old name so creature_template.ScriptName stays 'tele_npc_vote'
    new GenericCreatureScript<tele_npc_voteAI>("tele_npc_vote");
}