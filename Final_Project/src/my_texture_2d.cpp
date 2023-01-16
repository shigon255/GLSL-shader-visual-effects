#include "my_texture_2d.h"

myTexture2D::myTexture2D() 
    : width(0), height(0), internalFormat(GL_RGB), imageFormat(GL_RGB), wrapS(GL_REPEAT), wrapT(GL_REPEAT), filterMin(GL_LINEAR), filterMax(GL_LINEAR)
{
    glGenTextures(1, &this->textureID);
}

void myTexture2D::generate(unsigned int _width, unsigned int _height, unsigned char* data)
{
    // assign width & height
    this->width = _width;
    this->height = _height;

    // create texture
    glBindTexture(GL_TEXTURE_2D, this->textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, _width, _height, 0, this->imageFormat, GL_UNSIGNED_BYTE, data);

    // set OpenGL configurations
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, this->wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, this->wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, this->filterMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, this->filterMax);

    // unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

void myTexture2D::bind()
{
    glBindTexture(GL_TEXTURE_2D, this->textureID);
}
