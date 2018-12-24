#ifndef HEADER_RESOURCE_OBJPARSER_H
#define HEADER_RESOURCE_OBJPARSER_H

#include <istream>
#include "../util/SourceGetter.hpp"

class ObjParser{
	public:
		float *vertex = NULL,*normal = NULL;
		uint32_t *index = NULL;
		int n, ni, polyType, dim;
		ObjParser();
		~ObjParser();
		void parse(std::istream& in, int num, int num_inidices);
		SourceGetter<float*>* getVertexSource();
		SourceGetter<float*>* getNormalSource();
		SourceGetter<uint32_t*>* getIndexSource();
		void freeCache();
	private:
		int parseLine(char *line, float *buf);
};

#endif