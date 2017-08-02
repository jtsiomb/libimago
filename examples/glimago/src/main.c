#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <imago2.h>

struct image {
	char *name;
	int width, height;
	unsigned int tex;
};

void disp(void);
void draw_chess(int xdiv, int ydiv);
void reshape(int x, int y);
void keyb(unsigned char key, int x, int y);
int process_args(int argc, char **argv);
void next_image(void);

struct image *imglist;
int nimg, cur = -1;

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("imago - OpenGL image viewer");

	glutDisplayFunc(disp);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyb);

	if(process_args(argc, argv) == -1) {
		return 1;
	}
	next_image();

	glutMainLoop();
	return 0;
}

void disp(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	draw_chess(32, 32);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, imglist[cur].tex);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glBegin(GL_QUADS);
	glColor4f(1, 1, 1, 1);
	glTexCoord2f(0, 1);
	glVertex2f(-1, -1);
	glTexCoord2f(1, 1);
	glVertex2f(1, -1);
	glTexCoord2f(1, 0);
	glVertex2f(1, 1);
	glTexCoord2f(0, 0);
	glVertex2f(-1, 1);
	glEnd();

	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	glutSwapBuffers();
}

void draw_chess(int xdiv, int ydiv)
{
	static const float colors[][4] = {
		{0.6, 0.5, 0.5, 1.0},
		{0.7, 0.7, 0.8, 1.0}
	};
	float dx = 2.0 / xdiv;
	float dy = 2.0 / ydiv;
	float x, y;
	int i, j;

	glBegin(GL_QUADS);

	x = -1.0f;
	for(i=0; i<xdiv; i++) {
		y = -1.0f;
		for(j=0; j<ydiv; j++) {
			glColor3fv((i & 1) == (j & 1) ? colors[0] : colors[1]);

			glVertex2f(x, y);
			glVertex2f(x + dx, y);
			glVertex2f(x + dx, y + dy);
			glVertex2f(x, y + dy);

			y += dy;
		}
		x += dx;
	}

	glEnd();
}

void reshape(int x, int y)
{
	float rcp_aspect = (float)y / (float)x;
	glViewport(0, 0, x, y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1, 1, -rcp_aspect, rcp_aspect, -1, 1);
}

void keyb(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
	case 'q':
	case 'Q':
		exit(0);

	case ' ':
		next_image();
		glutPostRedisplay();
		break;
	}
}

int process_args(int argc, char **argv)
{
	int i;

	if(!(imglist = calloc(argc - 1, sizeof *imglist))) {
		perror("failed to allocate image list");
		return -1;
	}

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			fprintf(stderr, "invalid option: %s\n", argv[i]);
			return -1;
		} else {
			unsigned int tex;
			char *last_slash = strrchr(argv[i], '/');

			if(!(tex = img_gltexture_load(argv[i]))) {
				fprintf(stderr, "failed to load image: %s\n", argv[i]);
				continue;
			}
			imglist[nimg].name = last_slash ? last_slash + 1 : argv[i];
			imglist[nimg].tex = tex;

			glBindTexture(GL_TEXTURE_2D, tex);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &imglist[nimg].width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &imglist[nimg].height);
			++nimg;
		}
	}

	if(!nimg) {
		fprintf(stderr, "no images to show\n");
		return -1;
	}
	return 0;
}

void next_image(void)
{
	char *title;

	cur = (cur + 1) % nimg;
	glutReshapeWindow(imglist[cur].width, imglist[cur].height);

	title = alloca(strlen(imglist[cur].name) + 16);
	sprintf(title, "[%d/%d] %s\n", cur + 1, nimg, imglist[cur].name);
	glutSetWindowTitle(title);
}
