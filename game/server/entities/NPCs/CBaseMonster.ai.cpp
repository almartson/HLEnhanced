/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

  h_ai.cpp - halflife specific ai code

*/


#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"entities/NPCs/Monsters.h"
#include	"Server.h"
	
#define		NUM_LATERAL_CHECKS		13  // how many checks are made on each side of a monster looking for lateral cover
#define		NUM_LATERAL_LOS_CHECKS		6  // how many checks are made on each side of a monster looking for lateral cover

//float flRandom = RANDOM_FLOAT(0,1);

//=========================================================
// 
// AI UTILITY FUNCTIONS
//
// !!!UNDONE - move CBaseMonster functions to monsters.cpp
//=========================================================

//=========================================================
// FBoxVisible - a more accurate ( and slower ) version
// of FVisible. 
//
// !!!UNDONE - make this CBaseMonster?
//=========================================================
bool FBoxVisible( CBaseEntity* pLooker, CBaseEntity* pTarget, Vector &vecTargetOrigin, float flSize )
{
	// don't look through water
	if( ( pLooker->GetWaterLevel() != WATERLEVEL_HEAD && pTarget->GetWaterLevel() == WATERLEVEL_HEAD )
		|| ( pLooker->GetWaterLevel() == WATERLEVEL_HEAD && pTarget->GetWaterLevel() == WATERLEVEL_DRY ) )
		return false;

	TraceResult tr;
	Vector	vecLookerOrigin = pLooker->GetAbsOrigin() + pLooker->pev->view_ofs;//look through the monster's 'eyes'
	for (int i = 0; i < 5; i++)
	{
		Vector vecTarget = pTarget->GetAbsOrigin();
		vecTarget.x += RANDOM_FLOAT( pTarget->pev->mins.x + flSize, pTarget->pev->maxs.x - flSize);
		vecTarget.y += RANDOM_FLOAT( pTarget->pev->mins.y + flSize, pTarget->pev->maxs.y - flSize);
		vecTarget.z += RANDOM_FLOAT( pTarget->pev->mins.z + flSize, pTarget->pev->maxs.z - flSize);

		UTIL_TraceLine(vecLookerOrigin, vecTarget, ignore_monsters, ignore_glass, pLooker->edict(), &tr);
		
		if (tr.flFraction == 1.0)
		{
			vecTargetOrigin = vecTarget;
			return true;// line of sight is valid.
		}
	}
	return false;// Line of sight is not established
}

void UTIL_MoveToOrigin( CBaseEntity* pEntity, const Vector& vecGoal, float flDist, const MoveToOrigin moveType )
{
	MOVE_TO_ORIGIN( pEntity->edict(), vecGoal, flDist, moveType );
}

//
// VecCheckToss - returns the velocity at which an object should be lobbed from vecspot1 to land near vecspot2.
// returns g_vecZero if toss is not feasible.
// 
Vector VecCheckToss ( CBaseEntity* pEntity, const Vector &vecSpot1, Vector vecSpot2, float flGravityAdj )
{
	TraceResult		tr;
	Vector			vecMidPoint;// halfway point between Spot1 and Spot2
	Vector			vecApex;// highest point 
	Vector			vecScale;
	Vector			vecGrenadeVel;
	Vector			vecTemp;
	float			flGravity = g_psv_gravity->value * flGravityAdj;

	if (vecSpot2.z - vecSpot1.z > 500)
	{
		// to high, fail
		return g_vecZero;
	}

	UTIL_MakeVectors( pEntity->pev->angles );

	// toss a little bit to the left or right, not right down on the enemy's bean (head). 
	vecSpot2 = vecSpot2 + gpGlobals->v_right * ( RANDOM_FLOAT(-8,8) + RANDOM_FLOAT(-16,16) );
	vecSpot2 = vecSpot2 + gpGlobals->v_forward * ( RANDOM_FLOAT(-8,8) + RANDOM_FLOAT(-16,16) );
	
	// calculate the midpoint and apex of the 'triangle'
	// UNDONE: normalize any Z position differences between spot1 and spot2 so that triangle is always RIGHT

	// How much time does it take to get there?

	// get a rough idea of how high it can be thrown
	vecMidPoint = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	UTIL_TraceLine(vecMidPoint, vecMidPoint + Vector(0,0,500), ignore_monsters, pEntity->edict(), &tr);
	vecMidPoint = tr.vecEndPos;
	// (subtract 15 so the grenade doesn't hit the ceiling)
	vecMidPoint.z -= 15;

	if (vecMidPoint.z < vecSpot1.z || vecMidPoint.z < vecSpot2.z)
	{
		// to not enough space, fail
		return g_vecZero;
	}

	// How high should the grenade travel to reach the apex
	float distance1 = (vecMidPoint.z - vecSpot1.z);
	float distance2 = (vecMidPoint.z - vecSpot2.z);

	// How long will it take for the grenade to travel this distance
	float time1 = sqrt( distance1 / (0.5 * flGravity) );
	float time2 = sqrt( distance2 / (0.5 * flGravity) );

	if (time1 < 0.1)
	{
		// too close
		return g_vecZero;
	}

	// how hard to throw sideways to get there in time.
	vecGrenadeVel = (vecSpot2 - vecSpot1) / (time1 + time2);
	// how hard upwards to reach the apex at the right time.
	vecGrenadeVel.z = flGravity * time1;

	// find the apex
	vecApex  = vecSpot1 + vecGrenadeVel * time1;
	vecApex.z = vecMidPoint.z;

	UTIL_TraceLine(vecSpot1, vecApex, dont_ignore_monsters, pEntity->edict(), &tr);
	if (tr.flFraction != 1.0)
	{
		// fail!
		return g_vecZero;
	}

	// UNDONE: either ignore monsters or change it to not care if we hit our enemy
	UTIL_TraceLine(vecSpot2, vecApex, ignore_monsters, pEntity->edict(), &tr); 
	if (tr.flFraction != 1.0)
	{
		// fail!
		return g_vecZero;
	}
	
	return vecGrenadeVel;
}


//
// VecCheckThrow - returns the velocity vector at which an object should be thrown from vecspot1 to hit vecspot2.
// returns g_vecZero if throw is not feasible.
// 
Vector VecCheckThrow ( CBaseEntity* pEntity, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj )
{
	float			flGravity = g_psv_gravity->value * flGravityAdj;

	Vector vecGrenadeVel = (vecSpot2 - vecSpot1);

	// throw at a constant time
	float time = vecGrenadeVel.Length( ) / flSpeed;
	vecGrenadeVel = vecGrenadeVel * (1.0 / time);

	// adjust upward toss to compensate for gravity loss
	vecGrenadeVel.z += flGravity * time * 0.5;

	Vector vecApex = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);
	
	TraceResult tr;
	UTIL_TraceLine(vecSpot1, vecApex, dont_ignore_monsters, pEntity->edict(), &tr);
	if (tr.flFraction != 1.0)
	{
		// fail!
		return g_vecZero;
	}

	UTIL_TraceLine(vecSpot2, vecApex, ignore_monsters, pEntity->edict(), &tr);
	if (tr.flFraction != 1.0)
	{
		// fail!
		return g_vecZero;
	}

	return vecGrenadeVel;
}


