#ifndef MY_TEXTURE_2D_H
#define MY_TEXTURE_2D_H

#include <glad/glad.h>

class myTexture2D
{
public:
	myTexture2D();

	// generate texture data
	void generate(unsigned int _width, unsigned int _height, unsigned char* data);

	// bind the texture to OpenGL
	void bind();

	// texture ID in OpenGL
	unsigned int textureID;

	// texture's height & weight
	unsigned int height;
	unsigned int width;

	// texture's format (RGB or RGBA)
	unsigned int internalFormat;
	unsigned int imageFormat;

	// texture configuration in OpenGL
	unsigned int wrapS; 
	unsigned int wrapT; 
	unsigned int filterMin;
	unsigned int filterMax;
};

#endif

