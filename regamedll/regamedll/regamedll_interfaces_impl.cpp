/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "precompiled.h"

void CCSEntity::FireBullets(int iShots, Vector &vecSrc, Vector &vecDirShooting, Vector &vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker) {
	m_pContainingEntity->FireBullets(iShots, vecSrc, vecDirShooting, vecSpread, flDistance, iBulletType, iTracerFreq, iDamage, pevAttacker);
};

Vector CCSEntity::FireBullets3(Vector &vecSrc, Vector &vecDirShooting, float vecSpread, float flDistance, int iPenetration, int iBulletType, int iDamage, float flRangeModifier, entvars_t *pevAttacker, bool bPistol, int shared_rand) {
	return m_pContainingEntity->FireBullets3(vecSrc, vecDirShooting, vecSpread, flDistance, iPenetration, iBulletType, iDamage, flRangeModifier, pevAttacker, bPistol, shared_rand);
};

bool CCSPlayer::JoinTeam(TeamName team)
{
	CBasePlayer *pPlayer = BasePlayer();
	switch (team)
	{
	case SPECTATOR:
	{
		// are we already a spectator?
		if (pPlayer->m_iTeam == SPECTATOR)
			return false;

		pPlayer->RemoveAllItems(TRUE);
		pPlayer->m_bHasC4 = false;

		pPlayer->m_iTeam = SPECTATOR;
		pPlayer->m_iJoiningState = JOINED;

		pPlayer->m_pIntroCamera = NULL;
		pPlayer->m_bTeamChanged = true;

		pPlayer->TeamChangeUpdate();

		edict_t *pentSpawnSpot = g_pGameRules->GetPlayerSpawnSpot(pPlayer);
		pPlayer->StartObserver(pentSpawnSpot->v.origin, pentSpawnSpot->v.angles);

		// do we have fadetoblack on? (need to fade their screen back in)
		if (fadetoblack.value)
		{
			UTIL_ScreenFade(pPlayer, Vector(0, 0, 0), 0.001, 0, 0, FFADE_IN);
		}

		CSGameRules()->CheckWinConditions();
		return true;
	}
	case CT:
	case TERRORIST:
	{
		if (pPlayer->m_iTeam == SPECTATOR)
		{
			// If they're switching into spectator, setup spectator properties..
			pPlayer->m_bNotKilled = true;
			pPlayer->m_iIgnoreGlobalChat = IGNOREMSG_NONE;
			pPlayer->m_iTeamKills = 0;

			if (pPlayer->m_iAccount < int(startmoney.value)) {
				pPlayer->m_iAccount = int(startmoney.value);
			}

			pPlayer->pev->solid = SOLID_NOT;
			pPlayer->pev->movetype = MOVETYPE_NOCLIP;
			pPlayer->pev->effects = EF_NODRAW;
			pPlayer->pev->effects |= EF_NOINTERP;
			pPlayer->pev->takedamage = DAMAGE_NO;
			pPlayer->pev->deadflag = DEAD_DEAD;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->punchangle = g_vecZero;

			pPlayer->m_bHasNightVision = false;
			pPlayer->m_iHostagesKilled = 0;
			pPlayer->m_fDeadTime = 0;
			pPlayer->has_disconnected = false;

			pPlayer->m_iJoiningState = GETINTOGAME;
			pPlayer->SendItemStatus();

			SET_CLIENT_MAXSPEED(ENT(pPlayer->pev), 1);
			SET_MODEL(ENT(pPlayer->pev), "models/player.mdl");
		}
		break;
	}
	}

	if (pPlayer->pev->deadflag == DEAD_NO)
	{
		ClientKill(pPlayer->edict());
		pPlayer->pev->frags++;
	}

	MESSAGE_BEGIN(MSG_BROADCAST, gmsgScoreInfo);
		WRITE_BYTE(ENTINDEX(pPlayer->edict()));
		WRITE_SHORT(int(pPlayer->pev->frags));
		WRITE_SHORT(pPlayer->m_iDeaths);
		WRITE_SHORT(0);
		WRITE_SHORT(0);
	MESSAGE_END();

	// Switch their actual team...
	pPlayer->m_bTeamChanged = true;
	pPlayer->m_iTeam = team;
	pPlayer->TeamChangeUpdate();

	CSGameRules()->CheckWinConditions();
	return true;
}

bool CCSPlayer::RemovePlayerItem(const char* pszItemName)
{
	if (!pszItemName)
		return false;

	CBasePlayer *pPlayer = BasePlayer();

	// if it item_ ?
	if (pszItemName[0] == 'i') {
		pszItemName += sizeof("item_") - 1;

		// item_thighpack
		if (FStrEq(pszItemName, "thighpack"))
		{
			// if we don't have it?
			if (!pPlayer->m_bHasDefuser)
				return false;

			pPlayer->m_bHasDefuser = false;
			pPlayer->pev->body = 0;

			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev);
				WRITE_BYTE(STATUSICON_HIDE);
				WRITE_STRING("defuser");
			MESSAGE_END();

			pPlayer->SendItemStatus();
		}
		// item_longjump
		else if (FStrEq(pszItemName, "longjump"))
		{
			// if we don't have it?
			if (!pPlayer->m_fLongJump)
				return false;

			pPlayer->m_fLongJump = FALSE;
			SET_PHYSICS_KEY_VALUE(pPlayer->edict(), "slj", "0");
		}
		// item_assaultsuit
		else if (FStrEq(pszItemName, "assaultsuit"))
		{
			// if we don't have it?
			if (pPlayer->m_iKevlar != ARMOR_VESTHELM)
				return false;

			pPlayer->m_iKevlar = ARMOR_NONE;
			pPlayer->pev->armorvalue = 0;

			MESSAGE_BEGIN(MSG_ONE, gmsgArmorType, NULL, pPlayer->pev);
				WRITE_BYTE(0);
			MESSAGE_END();
		}
		// item_kevlar
		else if (FStrEq(pszItemName, "kevlar"))
		{
			// if we don't have it?
			if (pPlayer->m_iKevlar != ARMOR_KEVLAR)
				return false;

			pPlayer->m_iKevlar = ARMOR_NONE;
			pPlayer->pev->armorvalue = 0;
		}
		else
			return false;

		return true;
	}

	else if (FStrEq(pszItemName, "weapon_shield")) {
		if (!pPlayer->HasShield())
			return false;

		bool bIsProtectedShield = pPlayer->IsProtectedByShield();
		pPlayer->RemoveShield();

		CBasePlayerWeapon *pWeapon = static_cast<CBasePlayerWeapon *>(pPlayer->m_pActiveItem);

		if (pWeapon)
		{
			if (!pWeapon->CanHolster())
				return false;

			if (pWeapon->m_iId == WEAPON_HEGRENADE || pWeapon->m_iId == WEAPON_FLASHBANG || pWeapon->m_iId == WEAPON_SMOKEGRENADE)
			{
				if (pPlayer->m_rgAmmo[ pWeapon->m_iPrimaryAmmoType ] <= 0)
					g_pGameRules->GetNextBestWeapon(pPlayer, pWeapon);
			}

			if (pWeapon->m_flStartThrow != 0.0f)
				pWeapon->Holster();

			if (pPlayer->IsReloading())
			{
				pWeapon->m_fInReload = FALSE;
				pPlayer->m_flNextAttack = 0;
			}

			if (bIsProtectedShield)
				pWeapon->SecondaryAttack();

			pWeapon->Deploy();
		}

		return true;
	}

	for (auto pItem : pPlayer->m_rgpPlayerItems) {
		while (pItem != nullptr)
		{
			if (FClassnameIs(pItem->pev, pszItemName))
			{
				CBasePlayerWeapon *pWeapon = static_cast<CBasePlayerWeapon *>(pItem);
				if (pWeapon->IsWeapon())
				{
					if (FClassnameIs(pWeapon->pev, "weapon_c4"))
					{
						pPlayer->m_bHasC4 = false;
						pPlayer->pev->body = 0;
						pPlayer->SetBombIcon(FALSE);
						pPlayer->SetProgressBarTime(0);
					}

					pWeapon->RetireWeapon();
				}

				if (pWeapon->iItemSlot() == PRIMARY_WEAPON_SLOT) {
					pPlayer->m_bHasPrimary = false;
				}

				pPlayer->pev->weapons &= ~(1 << pItem->m_iId);
				pPlayer->RemovePlayerItem(pItem);
				pItem->Kill();
				return true;
			}

			pItem = pItem->m_pNext;
		}
	}

	return false;
}

void CCSPlayer::GiveNamedItemEx(const char *pszName)
{
	CBasePlayer *pPlayer = BasePlayer();

	if (FStrEq(pszName, "weapon_c4")) {
		pPlayer->m_bHasC4 = true;
		pPlayer->SetBombIcon();

		if (pPlayer->m_iTeam == TERRORIST) {
			pPlayer->pev->body = 1;
		}
	} else if (FStrEq(pszName, "weapon_shield")) {
		DropPrimary(pPlayer);
		pPlayer->GiveShield();
		return;
	}

	pPlayer->GiveNamedItemEx(pszName);
}

bool CCSPlayer::IsConnected() const { return m_pContainingEntity->has_disconnected == false; }
void CCSPlayer::SetAnimation(PLAYER_ANIM playerAnim) { BasePlayer()->SetAnimation(playerAnim); }
void CCSPlayer::AddAccount(int amount, RewardType type, bool bTrackChange) { BasePlayer()->AddAccount(amount, type, bTrackChange); }
void CCSPlayer::GiveNamedItem(const char *pszName) { BasePlayer()->GiveNamedItem(pszName); }
void CCSPlayer::GiveDefaultItems() { BasePlayer()->GiveDefaultItems(); }
void CCSPlayer::GiveShield(bool bDeploy) { BasePlayer()->GiveShield(bDeploy); }
void CCSPlayer::DropShield(bool bDeploy) { BasePlayer()->DropShield(bDeploy); }
void CCSPlayer::DropPlayerItem(const char *pszItemName) { BasePlayer()->DropPlayerItem(pszItemName); }
void CCSPlayer::RemoveShield() { BasePlayer()->RemoveShield(); }
void CCSPlayer::RemoveAllItems(bool bRemoveSuit) { BasePlayer()->RemoveAllItems(bRemoveSuit ? TRUE : FALSE); }
void CCSPlayer::SetPlayerModel(bool bHasC4) { BasePlayer()->SetPlayerModel(bHasC4 ? TRUE : FALSE); }
void CCSPlayer::SetPlayerModelEx(const char *modelName) { strncpy(m_szModel, modelName, sizeof(m_szModel) - 1); m_szModel[sizeof(m_szModel) - 1] = '\0'; };
void CCSPlayer::SetNewPlayerModel(const char *modelName) { BasePlayer()->SetNewPlayerModel(modelName); }
void CCSPlayer::ClientCommand(const char *cmd, const char *arg1, const char *arg2, const char *arg3) { BasePlayer()->ClientCommand(cmd, arg1, arg2, arg3); }
void CCSPlayer::SetProgressBarTime(int time) { BasePlayer()->SetProgressBarTime(time); }
void CCSPlayer::SetProgressBarTime2(int time, float timeElapsed) { BasePlayer()->SetProgressBarTime2(time, timeElapsed); }
edict_t *CCSPlayer::EntSelectSpawnPoint() { return BasePlayer()->EntSelectSpawnPoint(); }
void CCSPlayer::SendItemStatus() { BasePlayer()->SendItemStatus(); }
void CCSPlayer::SetBombIcon(bool bFlash) { BasePlayer()->SetBombIcon(bFlash ? TRUE : FALSE); }
void CCSPlayer::SetScoreAttrib(CBasePlayer *dest) { BasePlayer()->SetScoreAttrib(dest); }
void CCSPlayer::ReloadWeapons(CBasePlayerItem *pWeapon, bool bForceReload, bool bForceRefill) { BasePlayer()->ReloadWeapons(pWeapon, bForceReload, bForceRefill); }
void CCSPlayer::Observer_SetMode(int iMode) { BasePlayer()->Observer_SetMode(iMode); }
bool CCSPlayer::SelectSpawnSpot(const char *pEntClassName, CBaseEntity* &pSpot) { return BasePlayer()->SelectSpawnSpot(pEntClassName, pSpot); }
bool CCSPlayer::SwitchWeapon(CBasePlayerItem *pWeapon) { return BasePlayer()->SwitchWeapon(pWeapon) != FALSE; }
void CCSPlayer::SwitchTeam() { BasePlayer()->SwitchTeam(); }
void CCSPlayer::StartObserver(Vector& vecPosition, Vector& vecViewAngle) { BasePlayer()->StartObserver(vecPosition, vecViewAngle); }
void CCSPlayer::TeamChangeUpdate() { BasePlayer()->TeamChangeUpdate(); }
