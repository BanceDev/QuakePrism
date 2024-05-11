#include "mdl.h"
/* Vector */
typedef float vec3_t[3];

/* Table of precalculated normals */
vec3_t anorms_table[162] = {
#include "anorms.h"
};

/* Palette */
unsigned char colormap[256][3] = {
#include "colormap.h"
};

/* MDL header */
struct mdl_header_t
{
	int ident;            /* magic number: "IDPO" */
	int version;          /* version: 6 */

	vec3_t scale;         /* scale factor */
	vec3_t translate;     /* translation vector */
	float boundingradius;
	vec3_t eyeposition;   /* eyes' position */

	int num_skins;        /* number of textures */
	int skinwidth;        /* texture width */
	int skinheight;       /* texture height */

	int num_verts;        /* number of vertices */
	int num_tris;         /* number of triangles */
	int num_frames;       /* number of frames */

	int synctype;         /* 0 = synchron, 1 = random */
	int flags;            /* state flag */
	float size;
};

/* Skin */
struct mdl_skin_t
{
	int group;      /* 0 = single, 1 = group */
	GLubyte *data;  /* texture data */
};

/* Texture coords */
struct mdl_texcoord_t
{
	int onseam;
	int s;
	int t;
};

/* Triangle info */
struct mdl_triangle_t
{
	int facesfront;  /* 0 = backface, 1 = frontface */
	int vertex[3];   /* vertex indices */
};

/* Compressed vertex */
struct mdl_vertex_t
{
	unsigned char v[3];
	unsigned char normalIndex;
};

/* Simple frame */
struct mdl_simpleframe_t
{
	struct mdl_vertex_t bboxmin; /* bouding box min */
	struct mdl_vertex_t bboxmax; /* bouding box max */
	char name[16];
	struct mdl_vertex_t *verts;  /* vertex list of the frame */
};

/* Model frame */
struct mdl_frame_t
{
	int type;                        /* 0 = simple, !0 = group */
	struct mdl_simpleframe_t frame;  /* this program can't read models
									 composed of group frames! */
};

/* MDL model structure */
struct mdl_model_t
{
	struct mdl_header_t header;

	struct mdl_skin_t *skins;
	struct mdl_texcoord_t *texcoords;
	struct mdl_triangle_t *triangles;
	struct mdl_frame_t *frames;

	GLuint *tex_id;
	int iskin;
};

// An MDL model
mdl_model_t mdlfile;

/**
* Make a texture given a skin index 'n'.
*/
GLuint MakeTextureFromSkin(int n, const struct mdl_model_t *mdl)
{
	int i;
	GLuint id;

	GLubyte *pixels = (GLubyte *)
		malloc(mdl->header.skinwidth * mdl->header.skinheight * 3);

	/* Convert indexed 8 bits texture to RGB 24 bits */
	for (i = 0; i < mdl->header.skinwidth * mdl->header.skinheight; ++i)
	{
		pixels[(i * 3) + 0] = colormap[mdl->skins[n].data[i]][0];
		pixels[(i * 3) + 1] = colormap[mdl->skins[n].data[i]][1];
		pixels[(i * 3) + 2] = colormap[mdl->skins[n].data[i]][2];
	}

	/* Generate OpenGL texture */
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, mdl->header.skinwidth,
		mdl->header.skinheight, GL_RGB, GL_UNSIGNED_BYTE,
		pixels);

	/* OpenGL has its own copy of image data */
	free(pixels);
	return id;
}

/**
* Load an MDL model from file.
*
* Note: MDL format stores model's data in little-endian ordering.  On
* big-endian machines, you'll have to perform proper conversions.
*/
int ReadMDLModel(const char *filename, struct mdl_model_t *mdl)
{
	FILE *fp;
	int i;

	fp = fopen(filename, "rb");
	if (!fp)
	{
		fprintf(stderr, "error: couldn't open \"%s\"!\n", filename);
		return 0;
	}

	/* Read header */
	fread(&mdl->header, 1, sizeof(struct mdl_header_t), fp);

	if ((mdl->header.ident != 1330660425) ||
		(mdl->header.version != 6))
	{
		/* Error! */
		fprintf(stderr, "Error: bad version or identifier\n");
		fclose(fp);
		return 0;
	}

	/* Memory allocations */
	mdl->skins = (struct mdl_skin_t *)
		malloc(sizeof(struct mdl_skin_t) * mdl->header.num_skins);
	mdl->texcoords = (struct mdl_texcoord_t *)
		malloc(sizeof(struct mdl_texcoord_t) * mdl->header.num_verts);
	mdl->triangles = (struct mdl_triangle_t *)
		malloc(sizeof(struct mdl_triangle_t) * mdl->header.num_tris);
	mdl->frames = (struct mdl_frame_t *)
		malloc(sizeof(struct mdl_frame_t) * mdl->header.num_frames);
	mdl->tex_id = (GLuint *)
		malloc(sizeof(GLuint) * mdl->header.num_skins);

	mdl->iskin = 0;

	/* Read texture data */
	for (i = 0; i < mdl->header.num_skins; ++i)
	{
		mdl->skins[i].data = (GLubyte *)malloc(sizeof(GLubyte)
			* mdl->header.skinwidth * mdl->header.skinheight);

		fread(&mdl->skins[i].group, sizeof(int), 1, fp);
		fread(mdl->skins[i].data, sizeof(GLubyte),
			mdl->header.skinwidth * mdl->header.skinheight, fp);

		mdl->tex_id[i] = MakeTextureFromSkin(i, mdl);

		free(mdl->skins[i].data);
		mdl->skins[i].data = NULL;
	}

	fread(mdl->texcoords, sizeof(struct mdl_texcoord_t),
		mdl->header.num_verts, fp);
	fread(mdl->triangles, sizeof(struct mdl_triangle_t),
		mdl->header.num_tris, fp);

	/* Read frames */
	for (i = 0; i < mdl->header.num_frames; ++i)
	{
		/* Memory allocation for vertices of this frame */
		mdl->frames[i].frame.verts = (struct mdl_vertex_t *)
			malloc(sizeof(struct mdl_vertex_t) * mdl->header.num_verts);

		/* Read frame data */
		fread(&mdl->frames[i].type, sizeof(int), 1, fp);
		fread(&mdl->frames[i].frame.bboxmin,
			sizeof(struct mdl_vertex_t), 1, fp);
		fread(&mdl->frames[i].frame.bboxmax,
			sizeof(struct mdl_vertex_t), 1, fp);
		fread(mdl->frames[i].frame.name, sizeof(char), 16, fp);
		fread(mdl->frames[i].frame.verts, sizeof(struct mdl_vertex_t),
			mdl->header.num_verts, fp);
	}

	fclose(fp);
	return 1;
}

/**
* Free resources allocated for the model.
*/
void FreeModel(struct mdl_model_t *mdl)
{
	int i;

	if (mdl->skins)
	{
		free(mdl->skins);
		mdl->skins = NULL;
	}

	if (mdl->texcoords)
	{
		free(mdl->texcoords);
		mdl->texcoords = NULL;
	}

	if (mdl->triangles)
	{
		free(mdl->triangles);
		mdl->triangles = NULL;
	}

	if (mdl->tex_id)
	{
		/* Delete OpenGL textures */
		glDeleteTextures(mdl->header.num_skins, mdl->tex_id);

		free(mdl->tex_id);
		mdl->tex_id = NULL;
	}

	if (mdl->frames)
	{
		for (i = 0; i < mdl->header.num_frames; ++i)
		{
			free(mdl->frames[i].frame.verts);
			mdl->frames[i].frame.verts = NULL;
		}

		free(mdl->frames);
		mdl->frames = NULL;
	}
}

/**
* Render the model at frame n.
*/
void RenderFrame(int n, const struct mdl_model_t *mdl)
{
	int i, j;
	GLfloat s, t;
	vec3_t v;
	struct mdl_vertex_t *pvert;

	/* Check if n is in a valid range */
	if ((n < 0) || (n > mdl->header.num_frames - 1))
		return;

	/* Enable model's texture */
	glBindTexture(GL_TEXTURE_2D, mdl->tex_id[mdl->iskin]);

	/* Draw the model */
	glBegin(GL_TRIANGLES);
	/* Draw each triangle */
	for (i = 0; i < mdl->header.num_tris; ++i)
	{
		/* Draw each vertex */
		for (j = 0; j < 3; ++j)
		{
			pvert = &mdl->frames[n].frame.verts[mdl->triangles[i].vertex[j]];

			/* Compute texture coordinates */
			s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
			t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;

			if (!mdl->triangles[i].facesfront &&
				mdl->texcoords[mdl->triangles[i].vertex[j]].onseam)
			{
				s += mdl->header.skinwidth * 0.5f; /* Backface */
			}

			/* Scale s and t to range from 0.0 to 1.0 */
			s = (s + 0.5) / mdl->header.skinwidth;
			t = (t + 0.5) / mdl->header.skinheight;

			/* Pass texture coordinates to OpenGL */
			glTexCoord2f(s, t);

			/* Normal vector */
			glNormal3fv(anorms_table[pvert->normalIndex]);

			/* Calculate real vertex position */
			v[0] = (mdl->header.scale[0] * pvert->v[0]) + mdl->header.translate[0];
			v[1] = (mdl->header.scale[1] * pvert->v[1]) + mdl->header.translate[1];
			v[2] = (mdl->header.scale[2] * pvert->v[2]) + mdl->header.translate[2];

			glVertex3fv(v);
		}
	}
	glEnd();
}

/**
* Render the model with interpolation between frame n and n+1.
* interp is the interpolation percent. (from 0.0 to 1.0)
*/
void RenderFrameItp(int n, float interp, const struct mdl_model_t *mdl)
{
	int i, j;
	GLfloat s, t;
	vec3_t norm, v;
	GLfloat *n_curr, *n_next;
	struct mdl_vertex_t *pvert1, *pvert2;

	/* Check if n is in a valid range */
	if ((n < 0) || (n > mdl->header.num_frames))
		return;

	/* Enable model's texture */
	glBindTexture(GL_TEXTURE_2D, mdl->tex_id[mdl->iskin]);

	/* Draw the model */
	glBegin(GL_TRIANGLES);
	/* Draw each triangle */
	for (i = 0; i < mdl->header.num_tris; ++i)
	{
		/* Draw each vertex */
		for (j = 0; j < 3; ++j)
		{
			pvert1 = &mdl->frames[n].frame.verts[mdl->triangles[i].vertex[j]];
			pvert2 = &mdl->frames[n + 1].frame.verts[mdl->triangles[i].vertex[j]];

			/* Compute texture coordinates */
			s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
			t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;

			if (!mdl->triangles[i].facesfront &&
				mdl->texcoords[mdl->triangles[i].vertex[j]].onseam)
			{
				s += mdl->header.skinwidth * 0.5f; /* Backface */
			}

			/* Scale s and t to range from 0.0 to 1.0 */
			s = (s + 0.5) / mdl->header.skinwidth;
			t = (t + 0.5) / mdl->header.skinheight;

			/* Pass texture coordinates to OpenGL */
			glTexCoord2f(s, t);

			/* Interpolate normals */
			n_curr = anorms_table[pvert1->normalIndex];
			n_next = anorms_table[pvert2->normalIndex];

			norm[0] = n_curr[0] + interp * (n_next[0] - n_curr[0]);
			norm[1] = n_curr[1] + interp * (n_next[1] - n_curr[1]);
			norm[2] = n_curr[2] + interp * (n_next[2] - n_curr[2]);

			glNormal3fv(norm);

			/* Interpolate vertices */
			v[0] = mdl->header.scale[0] * (pvert1->v[0] + interp
				* (pvert2->v[0] - pvert1->v[0])) + mdl->header.translate[0];
			v[1] = mdl->header.scale[1] * (pvert1->v[1] + interp
				* (pvert2->v[1] - pvert1->v[1])) + mdl->header.translate[1];
			v[2] = mdl->header.scale[2] * (pvert1->v[2] + interp
				* (pvert2->v[2] - pvert1->v[2])) + mdl->header.translate[2];

			glVertex3fv(v);
		}
	}
	glEnd();
}

/**
* Calculate the current frame in animation beginning at frame
* 'start' and ending at frame 'end', given interpolation percent.
* interp will be reseted to 0.0 if the next frame is reached.
*/
void Animate(int start, int end, int *frame, float *interp)
{
	if ((*frame < start) || (*frame > end))
		*frame = start;

	if (*interp >= 1.0f)
	{
		/* Move to next frame */
		*interp = 0.0f;
		(*frame)++;

		if (*frame >= end)
			*frame = start;
	}
}

void cleanup()
{
	FreeModel(&mdlfile);
}

void reshape(int w, int h)
{
	if (h == 0)
		h = 1;

	glViewport(0, 0, (GLsizei)w, (GLsizei)h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, w / (GLdouble)h, 0.1, 1000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void render()
{
	static int n = 0;
	static float interp = 0.0;
	static double curent_time = 0;
	static double last_time = 0;

	GLfloat lightpos[] = { 5.0f, 10.0f, 0.0f, 1.0f };

	// Initialize OpenGL context 
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glShadeModel(GL_SMOOTH);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	// Load MDL model file 
	if (!ReadMDLModel("player.mdl", &mdlfile))
		exit(-1);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	last_time = curent_time;
	//curent_time = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	curent_time = SDL_GetTicks() / 1000.0;

	// Animate model from frames 0 to num_frames-1 
	interp += 10 * (curent_time - last_time);
	Animate(0, mdlfile.header.num_frames - 1, &n, &interp);

	glTranslatef(0.0f, 0.0f, -100.0f);
	glRotatef(-90.0f, 1.0, 0.0, 0.0);
	glRotatef(-90.0f, 0.0, 0.0, 1.0);

	// Draw the model 
	if (mdlfile.header.num_frames > 1)
		RenderFrameItp(n, interp, &mdlfile);
	else
		RenderFrame(n, &mdlfile);

}