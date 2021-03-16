
#include "g_local.h"

//===============================
// BLOCKED Logic
//===============================

/*
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_DEBUGTRAIL);
		gi.WritePosition (pt1);
		gi.WritePosition (pt2);
		gi.multicast (pt1, MULTICAST_PVS);	

		self->nextthink = level.time + 10;
*/

// plat states, copied from g_func.c

#define	STATE_TOP			0
#define	STATE_BOTTOM		1
#define STATE_UP			2
#define STATE_DOWN			3

void HuntTarget (edict_t *self);

// PMM
qboolean parasite_drain_attack_ok (vec3_t start, vec3_t end);


// blocked_checkplat
//	dist: how far they are trying to walk.
qboolean blocked_checkplat (edict_t *self, float dist)
{
	int			playerPosition;
	trace_t		trace;
	vec3_t		pt1, pt2;
	vec3_t		forward;
	edict_t		*plat;

	if(!self->enemy)
		return false;

	// check player's relative altitude
	if(self->enemy->absmin[2] >= self->absmax[2])
		playerPosition = 1;
	else if(self->enemy->absmax[2] <= self->absmin[2])
		playerPosition = -1;
	else
		playerPosition = 0;

	// if we're close to the same position, don't bother trying plats.
	if(playerPosition == 0)
		return false;

	plat = NULL;

	// see if we're already standing on a plat.
	if(self->groundentity && self->groundentity != world)
	{
		if(!strncmp(self->groundentity->classname, "func_plat", 8))
			plat = self->groundentity;
	}

	// if we're not, check to see if we'll step onto one with this move
	if(!plat)
	{
		AngleVectors (self->s.angles, forward, NULL, NULL);
		VectorMA(self->s.origin, dist, forward, pt1);
		VectorCopy (pt1, pt2);
		pt2[2] -= 384;

		trace = gi.trace(pt1, vec3_origin, vec3_origin, pt2, self, MASK_MONSTERSOLID);
		if(trace.fraction < 1 && !trace.allsolid && !trace.startsolid)
		{
			if(!strncmp(trace.ent->classname, "func_plat", 8))
			{
				plat = trace.ent;
			}
		}
	}

	// if we've found a plat, trigger it.
	if(plat && plat->use)
	{
		if (playerPosition == 1)
		{
			if((self->groundentity == plat && plat->moveinfo.state == STATE_BOTTOM) ||
				(self->groundentity != plat && plat->moveinfo.state == STATE_TOP))
			{
//				if(g_showlogic && g_showlogic->value)
//					gi.dprintf("player above, and plat will raise. using!\n");
				plat->use (plat, self, self);
				return true;			
			}
		}
		else if(playerPosition == -1)
		{
			if((self->groundentity == plat && plat->moveinfo.state == STATE_TOP) ||
				(self->groundentity != plat && plat->moveinfo.state == STATE_BOTTOM))
			{
//				if(g_showlogic && g_showlogic->value)
//					gi.dprintf("player below, and plat will lower. using!\n");
				plat->use (plat, self, self);
				return true;
			}
		}
//		if(g_showlogic && g_showlogic->value)
//			gi.dprintf("hit a plat, not using. ppos: %d   plat: %d\n", playerPosition, plat->moveinfo.state);
	}

	return false;
}

// ============


// *****************************
//	MISCELLANEOUS STUFF
// *****************************

// PMM - inback
// use to see if opponent is behind you (not to side)
// if it looks a lot like infront, well, there's a reason


float realrange (edict_t *self, edict_t *other)
{
	vec3_t dir;
	
	VectorSubtract (self->s.origin, other->s.origin, dir);

	return VectorLength(dir);
}

//
// Monster "Bad" Areas
// 

void badarea_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
//	drawbbox(ent);
}

edict_t *SpawnBadArea(vec3_t mins, vec3_t maxs, float lifespan, edict_t *owner)
{
	edict_t *badarea;
	vec3_t	origin;
	
	VectorAdd(mins, maxs, origin);
	VectorScale(origin, 0.5, origin);

	VectorSubtract(maxs, origin, maxs);
	VectorSubtract(mins, origin, mins);

	badarea = G_Spawn();
	VectorCopy(origin, badarea->s.origin);
	VectorCopy(maxs, badarea->maxs);
	VectorCopy(mins, badarea->mins);
	badarea->touch = badarea_touch;
	badarea->movetype = MOVETYPE_NONE;
	badarea->solid = SOLID_TRIGGER;
	badarea->classname = "bad_area";
	gi.linkentity (badarea);

//	gi.dprintf("(%s)-(%s)\n", vtos(badarea->absmin), vtos(badarea->absmax));

	if(lifespan)
	{
		badarea->think = G_FreeEdict;
		badarea->nextthink = level.time + lifespan;
	}
	if(owner)
	{
		badarea->owner = owner;
	}

//	drawbbox(badarea);
	return badarea;
}

// CheckForBadArea
//		This is a customized version of G_TouchTriggers that will check
//		for bad area triggers and return them if they're touched.
edict_t *CheckForBadArea(edict_t *ent)
{
	int			i, num;
	edict_t		*touch[MAX_EDICTS], *hit;
	vec3_t		mins, maxs;

	VectorAdd(ent->s.origin, ent->mins, mins);
	VectorAdd(ent->s.origin, ent->maxs, maxs);

	num = gi.BoxEdicts (mins, maxs, touch, MAX_EDICTS, AREA_TRIGGERS);

//	drawbbox(ent);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (hit->touch == badarea_touch)
		{
			return hit;
		}
	}
	
	return NULL;
}

#define TESLA_DAMAGE_RADIUS		128

qboolean MarkTeslaArea(edict_t *self, edict_t *tesla)
{
	vec3_t	mins, maxs;
	edict_t *e;
	edict_t *tail;
	edict_t *area;

	if(!tesla || !self)
		return false;

	area = NULL;

	// make sure this tesla doesn't have a bad area around it already...
	e = tesla->teamchain;
	tail = tesla;
	while (e)
	{
		tail = tail->teamchain;
		if(!strcmp(e->classname, "bad_area"))
		{
//			gi.dprintf("tesla already has a bad area marked\n");
			return false;
		}
		e = e->teamchain;
	}

	// see if we can grab the trigger directly
	if(tesla->teamchain && tesla->teamchain->inuse)
	{
		edict_t *trigger;

		trigger = tesla->teamchain;

//		VectorAdd (trigger->s.origin, trigger->mins, mins);
//		VectorAdd (trigger->s.origin, trigger->maxs, maxs);
		VectorCopy(trigger->absmin, mins);
		VectorCopy(trigger->absmax, maxs);

		if(tesla->air_finished)
			area = SpawnBadArea (mins, maxs, tesla->air_finished, tesla);
		else
			area = SpawnBadArea (mins, maxs, tesla->nextthink, tesla);
	}
	// otherwise we just guess at how long it'll last.
	else
	{
	
		VectorSet (mins, -TESLA_DAMAGE_RADIUS, -TESLA_DAMAGE_RADIUS, tesla->mins[2]);
		VectorSet (maxs, TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS);

		area = SpawnBadArea(mins, maxs, 30, tesla);
	}

	// if we spawned a bad area, then link it to the tesla
	if(area)
	{
//		gi.dprintf("bad area marker spawned and linked to tesla\n");
		tail->teamchain = area;
	}
	return true;
}

// predictive calculator
// target is who you want to shoot
// start is where the shot comes from
// bolt_speed is how fast the shot is
// eye_height is a boolean to say whether or not to adjust to targets eye_height
// offset is how much time to miss by
// aimdir is the resulting aim direction (pass in NULL if you don't want it)
// aimpoint is the resulting aimpoint (pass in NULL if don't want it)
void PredictAim (edict_t *target, vec3_t start, float bolt_speed, qboolean eye_height, float offset, vec3_t aimdir, vec3_t aimpoint)
{
	vec3_t dir, vec;
	float dist, time;

	if (!target || !target->inuse)
	{
		VectorCopy (vec3_origin, aimdir);
		return;
	}

	VectorSubtract(target->s.origin, start, dir);
	if (eye_height)
		dir[2] += target->viewheight;
	dist = VectorLength(dir);
	time = dist / bolt_speed;


	VectorMA(target->s.origin, time - offset, target->velocity, vec);

	if (eye_height)
		vec[2] += target->viewheight;

	if (aimdir)
	{
		VectorSubtract (vec, start, aimdir);
		VectorNormalize (aimdir);
	}
	
	if (aimpoint)
	{
		VectorCopy (vec, aimpoint);
	}
}


qboolean below (edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot;
	vec3_t	down;
	
	VectorSubtract (other->s.origin, self->s.origin, vec);
	VectorNormalize (vec);
	VectorSet (down, 0, 0, -1);
	dot = DotProduct (vec, down);
	
	if (dot > 0.95)  // 18 degree arc below
		return true;
	return false;
}

void drawbbox (edict_t *self)
{
	int	lines[4][3] = {
		{1, 2, 4},
		{1, 2, 7},
		{1, 4, 5},
		{2, 4, 7}
	};

	int starts[4] = {0, 3, 5, 6};

	vec3_t pt[8];
	int i, j, k;
	vec3_t coords[2];
	vec3_t newbox;
	vec3_t f,r,u, dir;

	VectorCopy (self->absmin, coords[0]);
	VectorCopy (self->absmax, coords[1]);

	for (i=0; i<=1; i++)
	{
		for (j=0; j<=1; j++)
		{
			for (k=0; k<=1; k++)
			{
				pt[4*i+2*j+k][0] = coords[i][0];
				pt[4*i+2*j+k][1] = coords[j][1];
				pt[4*i+2*j+k][2] = coords[k][2];
			}
		}
	}
	
	for (i=0; i<= 3; i++)
	{
		for (j=0; j<= 2; j++)
		{
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_DEBUGTRAIL);
			gi.WritePosition (pt[starts[i]]);
			gi.WritePosition (pt[lines[i][j]]);
			gi.multicast (pt[starts[i]], MULTICAST_ALL);	
		}
	}

	vectoangles2 (self->s.angles, dir);
	AngleVectors (dir, f, r, u);

	VectorMA (self->s.origin, 50, f, newbox);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (self->s.origin);
	gi.WritePosition (newbox);
	gi.multicast (self->s.origin, MULTICAST_PVS);	
	VectorClear (newbox);

	VectorMA (self->s.origin, 50, r, newbox);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (self->s.origin);
	gi.WritePosition (newbox);
	gi.multicast (self->s.origin, MULTICAST_PVS);	
	VectorClear (newbox);

	VectorMA (self->s.origin, 50, u, newbox);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (self->s.origin);
	gi.WritePosition (newbox);
	gi.multicast (self->s.origin, MULTICAST_PVS);	
	VectorClear (newbox);
}

//
// New dodge code
//



//=========================
//=========================
qboolean has_valid_enemy (edict_t *self)
{
	if (!self->enemy)
		return false;

	if (!self->enemy->inuse)
		return false;

	if (self->enemy->health < 1)
		return false;

	return true;
}



// this returns a randomly selected coop player who is visible to self
// returns NULL if bad


// only meant to be used in coop
int CountPlayers (void)
{
	edict_t *ent;
	int		count = 0;
	int		player;


	for (player = 1; player <= game.maxclients; player++)
	{
		ent = &g_edicts[player];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		count++;
	}
/*
	ent = g_edicts+1; // skip the worldspawn
	while (ent)
	{
		if ((ent->client) && (ent->inuse))
		{
			ent++;
			count++;
		}
		else
			ent = NULL;
	}
*/
	return count;
}




