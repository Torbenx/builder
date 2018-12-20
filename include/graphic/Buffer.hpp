#ifndef HEADER_GRAPHIC_BUFFER_H
#define HEADER_GRAPHIC_BUFFER_H

#include "../GL/glwrapper.hpp"
#include "../util/SourceGetter.hpp"

template <typename T>
class Buffer{
	public:
		GLint target;
		GLint usage;
		GLuint glId;
		GLsizeiptr size;
		SourceGetter<T*> *source;
		Buffer(GLint target);
		~Buffer();
		bool init();
		void bind();
		void bufferData(SourceGetter<T*> *data,GLsizeiptr sizeInBytres,GLint usage);
		void bufferSubData(T *data,GLintptr offset,GLsizeiptr sizeInBytes);
};

const static GLint TE_BUFFER = 0x00cf;

#define Bufferf Buffer<float>
#define Bufferi Buffer<int>

template class Buffer<float>;
template class Buffer<int>;

#endif
