void() ClientKill = {};
void() ClientConnect = {};
void() ClientDisconnect = {};
void() PlayerPreThink = {};
void() PlayerPostThink = {};
void() PutClientInServer = {
	local entity spawn_spot;
	spawn_spot = find(world, classname, "info_player_start");
	
	self.classname = "player";
	self.health = self.max_health = 100;
	self.takedamage = DAMAGE_AIM;
	self.solid = SOLID_SLIDEBOX;
	self.movetype = MOVETYPE_WALK;
	self.flags = FL_CLIENT;
	
	self.origin = spawn_spot.origin + '0 0 1';
	self.angles = spawn_spot.angles;
	self.fixangle = TRUE;
	
	setmodel(self, "progs/player.mdl");
	setsize(self, VEC_HULL_MIN, VEC_HULL_MAX);
	
	self.view_ofs = '0 0 22';
	self.velocity = '0 0 0';
};
