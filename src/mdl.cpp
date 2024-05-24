/*
Copyright (C) 2024 Lance Borden

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3.0
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.

*/

#include "mdl.h"
#include <cstdio>
#include <fstream>
#include <iostream>

/* Table of precalculated normals */
vec3_t anorms_table[162] = {
#include "anorms.h"
};

/* Palette */
unsigned char colormap[256][3] = {
#include "colormap.h"
};

/* MDL header */
struct mdl_header_t {
	int ident;	 /* magic number: "IDPO" */
	int version; /* version: 6 */

	vec3_t scale;	  /* scale factor */
	vec3_t translate; /* translation vector */
	float boundingradius;
	vec3_t eyeposition; /* eyes' position */

	int num_skins;	/* number of textures */
	int skinwidth;	/* texture width */
	int skinheight; /* texture height */

	int num_verts;	/* number of vertices */
	int num_tris;	/* number of triangles */
	int num_frames; /* number of frames */

	int synctype; /* 0 = synchron, 1 = random */
	int flags;	  /* state flag */
	float size;
};

/* Skin */
struct mdl_skin_t {
	int group;	   /* 0 = single, 1 = group */
	GLubyte *data; /* texture data */
};

/* Texture coords */
struct mdl_texcoord_t {
	int onseam;
	int s;
	int t;
};

/* Triangle info */
struct mdl_triangle_t {
	int facesfront; /* 0 = backface, 1 = frontface */
	int vertex[3];	/* vertex indices */
};

/* Compressed vertex */
struct mdl_vertex_t {
	unsigned char v[3];
	unsigned char normalIndex;
};

/* Simple frame */
struct mdl_simpleframe_t {
	struct mdl_vertex_t bboxmin; /* bouding box min */
	struct mdl_vertex_t bboxmax; /* bouding box max */
	char name[16];
	struct mdl_vertex_t *verts; /* vertex list of the frame */
};

/* Model frame */
struct mdl_frame_t {
	int type;						/* 0 = simple, !0 = group */
	struct mdl_simpleframe_t frame; /* this program can't read models
					composed of group frames! */
};

/* MDL model structure */
struct mdl_model_t {
	struct mdl_header_t header;

	struct mdl_skin_t *skins;
	struct mdl_texcoord_t *texcoords;
	struct mdl_triangle_t *triangles;
	struct mdl_frame_t *frames;

	GLuint *tex_id;
	int iskin;
};

/* Header for the .tga texture export */
#pragma pack(push, 1)
struct tga_header_t {
	uint8_t idLength;
	uint8_t colorMapType;
	uint8_t dataTypeCode;
	uint16_t colorMapOrigin;
	uint16_t colorMapLength;
	uint8_t colorMapDepth;
	uint16_t xOrigin;
	uint16_t yOrigin;
	uint16_t width;
	uint16_t height;
	uint8_t bitsPerPixel;
	uint8_t imageDescriptor;
};
#pragma pack(pop)
// An MDL model
mdl_model_t mdlfile;

namespace QuakePrism::MDL {
// Init namespace variables from header
float interpAmt = 1.0f;
int currentFrame = 0;
int totalFrames = 0;
vec3_t modelAngles = {-90.0f, 0.0f, -90.0f};
vec3_t modelPosition = {0.0f, 0.0f, -100.0f};
GLfloat modelScale = 1.0f;

/**
 * Make a texture given a skin index 'n'.
 */
GLuint MakeTextureFromSkin(int n, const struct mdl_model_t *mdl) {
	GLuint id;

	GLubyte *pixels =
		(GLubyte *)malloc(mdl->header.skinwidth * mdl->header.skinheight * 3);

	/* Convert indexed 8 bits texture to RGB 24 bits */
	for (int i = 0; i < mdl->header.skinwidth * mdl->header.skinheight; i++) {
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
					  mdl->header.skinheight, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	/* OpenGL has its own copy of image data */
	free(pixels);
	return id;
}
bool ExportTextureToTGA(const struct mdl_model_t *mdl, int skinIndex,
						const std::string &filename) {
	if (skinIndex < 0 || skinIndex >= mdl->header.num_skins) {
		return false;
	}

	int width = mdl->header.skinwidth;
	int height = mdl->header.skinheight;

	// Allocate memory for the RGB data
	GLubyte *pixels = (GLubyte *)malloc(width * height * 3);

	// Convert indexed 8 bits texture to RGB 24 bits
	for (int i = 0; i < width * height; i++) {
		int colorIndex = mdl->skins[skinIndex].data[i];
		pixels[(i * 3) + 0] = colormap[colorIndex][2];
		pixels[(i * 3) + 1] = colormap[colorIndex][1];
		pixels[(i * 3) + 2] = colormap[colorIndex][0];
	}

	tga_header_t tgaHeader = {0};
	tgaHeader.dataTypeCode = 2; // Uncompressed, true-color image
	tgaHeader.width = width;
	tgaHeader.height = height;
	tgaHeader.bitsPerPixel = 24;
	tgaHeader.imageDescriptor = 0x20; // Top-left origin

	std::ofstream ofs(filename, std::ios::binary);
	if (!ofs) {
		free(pixels);
		return false;
	}

	ofs.write(reinterpret_cast<char *>(&tgaHeader), sizeof(tga_header_t));
	ofs.write(reinterpret_cast<char *>(pixels), width * height * 3);
	ofs.close();

	free(pixels);
	return true;
}
/**
 * Load an MDL model from file.
 *
 * Note: MDL format stores model's data in little-endian ordering.  On
 * big-endian machines, you'll have to perform proper conversions.
 */
int ReadMDLModel(const char *filename, struct mdl_model_t *mdl) {
	FILE *fp;

	fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "error: couldn't open \"%s\"!\n", filename);
		return 0;
	}

	/* Read header */
	fread(&mdl->header, 1, sizeof(struct mdl_header_t), fp);

	if ((mdl->header.ident != 1330660425) || (mdl->header.version != 6)) {
		/* Error! */
		fprintf(stderr, "Error: bad version or identifier\n");
		fclose(fp);
		return 0;
	}

	/* Memory allocations */
	mdl->skins = (struct mdl_skin_t *)malloc(sizeof(struct mdl_skin_t) *
											 mdl->header.num_skins);
	mdl->texcoords = (struct mdl_texcoord_t *)malloc(
		sizeof(struct mdl_texcoord_t) * mdl->header.num_verts);
	mdl->triangles = (struct mdl_triangle_t *)malloc(
		sizeof(struct mdl_triangle_t) * mdl->header.num_tris);
	mdl->frames = (struct mdl_frame_t *)malloc(sizeof(struct mdl_frame_t) *
											   mdl->header.num_frames);
	mdl->tex_id = (GLuint *)malloc(sizeof(GLuint) * mdl->header.num_skins);

	mdl->iskin = 0;

	/* Read texture data */
	for (int i = 0; i < mdl->header.num_skins; i++) {
		mdl->skins[i].data = (GLubyte *)malloc(
			sizeof(GLubyte) * mdl->header.skinwidth * mdl->header.skinheight);

		fread(&mdl->skins[i].group, sizeof(int), 1, fp);
		fread(mdl->skins[i].data, sizeof(GLubyte),
			  mdl->header.skinwidth * mdl->header.skinheight, fp);

		mdl->tex_id[i] = MakeTextureFromSkin(i, mdl);
	}

	fread(mdl->texcoords, sizeof(struct mdl_texcoord_t), mdl->header.num_verts,
		  fp);
	fread(mdl->triangles, sizeof(struct mdl_triangle_t), mdl->header.num_tris,
		  fp);

	/* Read frames */
	for (int i = 0; i < mdl->header.num_frames; i++) {
		/* Memory allocation for vertices of this frame */
		mdl->frames[i].frame.verts = (struct mdl_vertex_t *)malloc(
			sizeof(struct mdl_vertex_t) * mdl->header.num_verts);

		/* Read frame data */
		fread(&mdl->frames[i].type, sizeof(int), 1, fp);
		fread(&mdl->frames[i].frame.bboxmin, sizeof(struct mdl_vertex_t), 1,
			  fp);
		fread(&mdl->frames[i].frame.bboxmax, sizeof(struct mdl_vertex_t), 1,
			  fp);
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
void FreeModel(struct mdl_model_t *mdl) {

	for (int i = 0; i < mdl->header.num_skins; i++) {
		free(mdl->skins[i].data);
		mdl->skins[i].data = NULL;
	}

	if (mdl->skins) {
		free(mdl->skins);
		mdl->skins = NULL;
	}

	if (mdl->texcoords) {
		free(mdl->texcoords);
		mdl->texcoords = NULL;
	}

	if (mdl->triangles) {
		free(mdl->triangles);
		mdl->triangles = NULL;
	}

	if (mdl->tex_id) {
		/* Delete OpenGL textures */
		glDeleteTextures(mdl->header.num_skins, mdl->tex_id);

		free(mdl->tex_id);
		mdl->tex_id = NULL;
	}

	if (mdl->frames) {
		for (int i = 0; i < mdl->header.num_frames; i++) {
			free(mdl->frames[i].frame.verts);
			mdl->frames[i].frame.verts = NULL;
		}

		free(mdl->frames);
		mdl->frames = NULL;
	}
}

void SetTextureMode(const int mode, const struct mdl_model_t *mdl) {
	switch (mode) {
	case TEXTURED_MODE:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindTexture(GL_TEXTURE_2D, mdl->tex_id[mdl->iskin]);
		break;
	case TEXTURELESS_MODE: {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindTexture(GL_TEXTURE_2D, 0);
		GLfloat materialAmbient[] = {0.4f, 0.4f, 0.4f, 1.0f};
		GLfloat materialDiffuse[] = {0.9f, 0.9f, 0.9f, 1.0f};
		GLfloat materialEmissive[] = {0.0f, 0.0f, 0.0f, 1.0f};
		glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
		glMaterialfv(GL_FRONT, GL_EMISSION, materialEmissive);
		break;
	}
	case WIREFRAME_MODE:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBindTexture(GL_TEXTURE_2D, 0);
		break;
	default:
		break;
	}
}

/**
 * Render the model at frame n.
 */
void RenderFrame(int n, const int mode, const struct mdl_model_t *mdl) {
	GLfloat s, t;
	vec3_t v;
	struct mdl_vertex_t *pvert;

	/* Check if n is in a valid range */
	if ((n < 0) || (n > mdl->header.num_frames - 1))
		return;

	/* Setup the texture render mode */
	SetTextureMode(mode, mdl);

	/* Draw the model */
	glBegin(GL_TRIANGLES);
	/* Draw each triangle */
	for (int i = 0; i < mdl->header.num_tris; i++) {
		/* Draw each vertex */
		for (int j = 0; j < 3; j++) {
			pvert = &mdl->frames[n].frame.verts[mdl->triangles[i].vertex[j]];

			/* Compute texture coordinates */
			s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
			t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;

			if (!mdl->triangles[i].facesfront &&
				mdl->texcoords[mdl->triangles[i].vertex[j]].onseam) {
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
			v[0] =
				(mdl->header.scale[0] * pvert->v[0]) + mdl->header.translate[0];
			v[1] =
				(mdl->header.scale[1] * pvert->v[1]) + mdl->header.translate[1];
			v[2] =
				(mdl->header.scale[2] * pvert->v[2]) + mdl->header.translate[2];

			glVertex3fv(v);
		}
	}
	glEnd();
}

/**
 * Render the model with interpolation between frame n and n+1.
 * interp is the interpolation percent. (from 0.0 to 1.0)
 */
void RenderFrameItp(int n, float interp, const int mode,
					const struct mdl_model_t *mdl) {
	GLfloat s, t;
	vec3_t norm, v;
	GLfloat *n_curr, *n_next;
	struct mdl_vertex_t *pvert1, *pvert2;

	/* Check if n is in a valid range */
	if ((n < 0) || (n > mdl->header.num_frames))
		return;

	/* Setup the texture render mode */
	SetTextureMode(mode, mdl);

	/* Draw the model */
	glBegin(GL_TRIANGLES);
	/* Draw each triangle */
	for (int i = 0; i < mdl->header.num_tris; i++) {
		/* Draw each vertex */
		for (int j = 0; j < 3; j++) {
			pvert1 = &mdl->frames[n].frame.verts[mdl->triangles[i].vertex[j]];
			pvert2 =
				&mdl->frames[n + 1].frame.verts[mdl->triangles[i].vertex[j]];

			/* Compute texture coordinates */
			s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
			t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;

			if (!mdl->triangles[i].facesfront &&
				mdl->texcoords[mdl->triangles[i].vertex[j]].onseam) {
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
			v[0] = mdl->header.scale[0] *
					   (pvert1->v[0] + interp * (pvert2->v[0] - pvert1->v[0])) +
				   mdl->header.translate[0];
			v[1] = mdl->header.scale[1] *
					   (pvert1->v[1] + interp * (pvert2->v[1] - pvert1->v[1])) +
				   mdl->header.translate[1];
			v[2] = mdl->header.scale[2] *
					   (pvert1->v[2] + interp * (pvert2->v[2] - pvert1->v[2])) +
				   mdl->header.translate[2];

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
void Animate(int start, int end, int *frame, float *interp) {
	if ((*frame < start) || (*frame > end))
		*frame = start;

	if (*interp >= 1.0f) {
		/* Move to next frame */
		*interp = 0.0f;
		(*frame)++;

		if (*frame >= end)
			*frame = start;
	}
}

bool mdlTextureExport(std::filesystem::path modelPath) {
	modelPath.replace_extension(".tga");
	std::string tgaFilename = modelPath.string();
	return ExportTextureToTGA(&mdlfile, 0, tgaFilename);
}

void cleanup() { FreeModel(&mdlfile); }

void reshape(int w, int h) {
	if (h == 0)
		h = 1;

	glViewport(0, 0, (GLsizei)w, (GLsizei)h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, w / (GLdouble)h, 0.1, 1000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void render(const std::filesystem::path modelPath, const int mode,
			const bool paused, const bool lerpEnabled) {
	static double curent_time = 0;
	static double last_time = 0;

	if (modelPath.empty())
		return;

	if (!ReadMDLModel(modelPath.c_str(), &mdlfile))
		return;

	totalFrames = mdlfile.header.num_frames;
	// Initialize OpenGL context
	glClearColor(0.184f, 0.184f, 0.184f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	if (mode != TEXTURELESS_MODE) {
		glShadeModel(GL_SMOOTH);
		GLfloat materialEmissive[] = {1.0f, 1.0f, 1.0f, 1.0f};
		glMaterialfv(GL_FRONT, GL_EMISSION, materialEmissive);
	} else {
		glShadeModel(GL_FLAT);
		GLfloat lightpos[] = {5.0f, 10.0f, 0.0f, 1.0f};
		GLfloat lightAmbient[] = {0.2f, 0.2f, 0.2f, 1.0f};
		GLfloat lightDiffuse[] = {0.5f, 0.5f, 0.5f, 1.0f};
		GLfloat lightSpecular[] = {0.5f, 0.5f, 0.5f, 1.0f};
		glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
		glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
	}
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// This normalize is done to clean up lighting when the model is scaled
	glEnable(GL_NORMALIZE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	last_time = curent_time;
	curent_time = SDL_GetTicks() / 1000.0;

	// Animate model from frames 0 to num_frames-1
	interpAmt += 10 * (curent_time - last_time);
	if (!paused)
		Animate(0, mdlfile.header.num_frames - 1, &currentFrame, &interpAmt);

	glTranslatef(modelPosition[0], modelPosition[1], modelPosition[2]);
	glRotatef(modelAngles[0], 1.0, 0.0, 0.0);
	glRotatef(modelAngles[2], 0.0, 0.0, 1.0);
	glScalef(modelScale, modelScale, modelScale);

	// Draw the model
	if (mdlfile.header.num_frames > 1 && !paused && lerpEnabled)
		RenderFrameItp(currentFrame, interpAmt, mode, &mdlfile);
	else
		RenderFrame(currentFrame, mode, &mdlfile);
}
} // namespace QuakePrism::MDL
