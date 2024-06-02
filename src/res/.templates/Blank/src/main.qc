void() precaches;
void() main = {};
void() worldspawn = {
	precaches();
};
void() SetNewParms = {};
void() SetChangeParms = {};
void() StartFrame = {};

void() precaches = {
	precache_model("progs/player.mdl");
};
