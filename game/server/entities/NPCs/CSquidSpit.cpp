/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#include "extdll.h"
#include "util.h"
#include "Skill.h"
#include "Decals.h"
#include "cbase.h"

#include "CSquidSpit.h"

int iSquidSpitSprite;

BEGIN_DATADESC( CSquidSpit )
	DEFINE_FIELD( m_maxFrame, FIELD_INTEGER ),
	DEFINE_THINKFUNC( Animate ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( squidspit, CSquidSpit );

void CSquidSpit::Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;

	SetSolidType( SOLID_BBOX );
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SetModel( "sprites/bigspit.spr" );
	pev->frame = 0;
	pev->scale = 0.5;

	SetSize( Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	m_maxFrame = ( float ) MODEL_FRAMES( GetModelIndex() ) - 1;
}

void CSquidSpit::Shoot( CBaseEntity* pOwner, Vector vecStart, Vector vecVelocity )
{
	auto pSpit = static_cast<CSquidSpit*>( UTIL_CreateNamedEntity( "squidspit" ) );
	pSpit->Spawn();

	pSpit->SetAbsOrigin( vecStart );
	pSpit->SetAbsVelocity( vecVelocity );
	pSpit->SetOwner( pOwner );

	pSpit->SetThink( &CSquidSpit::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CSquidSpit::Touch( CBaseEntity *pOther )
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( this, CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );

	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( this, CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	case 1:
		EMIT_SOUND_DYN( this, CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	}

	if( !pOther->pev->takedamage )
	{

		// make a splat on the wall
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace( &tr, DECAL_SPIT1 + RANDOM_LONG( 0, 1 ) );

		// make some flecks
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( tr.vecEndPos.x );	// pos
		WRITE_COORD( tr.vecEndPos.y );
		WRITE_COORD( tr.vecEndPos.z );
		WRITE_COORD( tr.vecPlaneNormal.x );	// dir
		WRITE_COORD( tr.vecPlaneNormal.y );
		WRITE_COORD( tr.vecPlaneNormal.z );
		WRITE_SHORT( iSquidSpitSprite );	// model
		WRITE_BYTE( 5 );			// count
		WRITE_BYTE( 30 );			// speed
		WRITE_BYTE( 80 );			// noise ( client will divide by 100 )
		MESSAGE_END();
	}
	else
	{
		pOther->TakeDamage( this, this, gSkillData.GetBullsquidDmgSpit(), DMG_GENERIC );
	}

	SetThink( &CSquidSpit::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CSquidSpit::Animate( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if( pev->frame++ )
	{
		if( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}