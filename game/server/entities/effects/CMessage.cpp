#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "CMessage.h"

LINK_ENTITY_TO_CLASS( env_message, CMessage );

void CMessage::Spawn( void )
{
	Precache();

	SetSolidType( SOLID_NOT );
	SetMoveType( MOVETYPE_NONE );

	switch( pev->impulse )
	{
	case 1: // Medium radius
		pev->speed = ATTN_STATIC;
		break;

	case 2:	// Large radius
		pev->speed = ATTN_NORM;
		break;

	case 3:	//EVERYWHERE
		pev->speed = ATTN_NONE;
		break;

	default:
	case 0: // Small radius
		pev->speed = ATTN_IDLE;
		break;
	}
	pev->impulse = 0;

	// No volume, use normal
	if( pev->scale <= 0 )
		pev->scale = 1.0;
}

void CMessage::Precache( void )
{
	if( pev->noise )
		PRECACHE_SOUND( ( char * ) STRING( pev->noise ) );
}

void CMessage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pPlayer = NULL;

	if( pev->spawnflags & SF_MESSAGE_ALL )
		UTIL_ShowMessageAll( GetMessage() );
	else
	{
		if( pActivator && pActivator->IsPlayer() )
			pPlayer = pActivator;
		else
		{
			pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
		}
		if( pPlayer )
			UTIL_ShowMessage( GetMessage(), pPlayer );
	}
	if( pev->noise )
	{
		EMIT_SOUND( this, CHAN_BODY, STRING( pev->noise ), pev->scale, pev->speed );
	}
	if( pev->spawnflags & SF_MESSAGE_ONCE )
		UTIL_Remove( this );

	SUB_UseTargets( this, USE_TOGGLE, 0 );
}

void CMessage::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "messagesound" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "messagevolume" ) )
	{
		pev->scale = atof( pkvd->szValue ) * 0.1;
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "messageattenuation" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		CPointEntity::KeyValue( pkvd );
}