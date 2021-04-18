#include "resource/ObjParser.hpp"
#include "util/log.hpp"
#include <cstring>
#include <vector>

#define MAX_POLY_TYPE 4
#define MAX_DIM 3

struct ObjParserIndex {
    int v, t, n;
};

void ObjParser::parse(std::istream& in) {
    std::vector<float> vertex;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<ObjParserIndex> index;
    const int bufsize = 256;
    char buf[bufsize];
    polyType = -1;
    boundingBox = { HUGE_VALF, -HUGE_VALF, HUGE_VALF, -HUGE_VALF, HUGE_VALF, -HUGE_VALF };
    dim = -1;
    while (in.getline(buf, bufsize)) {
        if (in.fail()) {
            LOGE("fail");
        }
        if (buf[0] == 'v') {
            if (buf[1] == ' ') {
                int r = 0;
                char* line = buf + 2;
                Vec3 v;
                for (;;) {
                    char* end;
                    float f = std::strtof(line, &end);
                    if (line == end) {
                        break;
                    }
                    line = end;
                    if (++r > MAX_DIM) {
                        LOGW("dimension to high");
                        break;
                    }
                    if (r == 1)
                        v.x = f;
                    if(r == 2)
                        v.y = f;
                    if(r == 3)
                        v.z = f;
                    vertex.push_back(f);
                }
                boundingBox.enlargeToInclude(v);
                if (r != dim) {
                    if (dim == -1) {
                        dim = r;
                    } else {
                        LOGW("inconsistent dimension");
                    }
                }
            } else if (buf[1] == 't') {
                int r = 0;
                char* line = buf + 2;
                for (;;) {
                    char* end;
                    float f = std::strtof(line, &end);
                    if (line == end) {
                        break;
                    }
                    line = end;
                    if (++r > 2) {
                        if (f != 0.0f)
                            LOGW("expected 2d texture coordinates");
                    } else {
                        texCoords.push_back(f);
                    }
                }
            } else if (buf[1] == 'n') {
                int r = 0;
                char* line = buf + 2;
                for (;;) {
                    char* end;
                    float f = std::strtof(line, &end);
                    if (line == end) {
                        break;
                    }
                    line = end;
                    if (++r > MAX_DIM) {
                        LOGW("dimension to high");
                        break;
                    }
                    normals.push_back(f);
                }
                if (r != dim) {
                    if (dim == -1) {
                        dim = r;
                    } else {
                        LOGW("inconsistent dimension");
                    }
                }
            }
        } else if (buf[0] == 'f') {
            int r = 0;
            char* line = buf + 1;
            for (;;) {
                char* end;
                int l = std::strtol(line, &end, 10) - 1;
                if (l == -1) {
                    break;
                }
                if (++r > MAX_POLY_TYPE) {
                    LOGW("poly type to high");
                    break;
                }
                ObjParserIndex ind;
                ind.v = l;
                if (end[0] == '/') {
                    line = end + 1;
                    l = std::strtol(line, &end, 10) - 1;
                    if (l != -1) {
                        ind.t = l;
                    } else {
                        LOGD("failed to parse texture index %s", line);
                        ind.t = -1;
                    }
                    if (end[0] == '/') {
                        line = end + 1;
                        l = std::strtol(line, &end, 10) - 1;
                        if (l != -1) {
                            ind.n = l;
                        } else {
                            LOGD("failed to parse normal index: %s", line);
                            ind.n = -1;
                        }
                    }
                }
                index.push_back(ind);
                line = end;
            }
            if (r != polyType) {
                if (polyType == -1) {
                    polyType = r;
                } else {
                    LOGW("inconsistent poly type");
                }
            }
        }
    }
    boundingBox.debug();
    LOGD("vertex: %d, index: %d tex: %d normal: %d", vertex.size() / dim, index.size() / polyType, texCoords.size() / 2, normals.size() / dim);
    int ni = index.size() / polyType;
    int tri = polyType - 2;
    n = ni * tri * 3;
    if (false) {
        vertexCount = n;
        float* out_vertex = new float[n * dim];
        float* out_normals = new float[n * dim];
        float* out_texCoords = new float[n * 2];
        for (int i = 0; i < ni; i++) {
            for (int j = 0; j < tri; j++) {
                for (int k = 0; k < dim; k++) {
                    // clang-format off
                    out_vertex[((i*tri+j)*3  )*dim+k] = vertex[index[i*polyType    ].v*dim+k];
                    out_vertex[((i*tri+j)*3+1)*dim+k] = vertex[index[i*polyType+j+1].v*dim+k];
                    out_vertex[((i*tri+j)*3+2)*dim+k] = vertex[index[i*polyType+j+2].v*dim+k];
                    out_normals[((i*tri+j)*3  )*dim+k] = normals[index[i*polyType    ].n*dim+k];
                    out_normals[((i*tri+j)*3+1)*dim+k] = normals[index[i*polyType+j+1].n*dim+k];
                    out_normals[((i*tri+j)*3+2)*dim+k] = normals[index[i*polyType+j+2].n*dim+k];
                    // clang-format on
                }
                // clang-format off
                out_texCoords[((i*tri+j)*3  )*2+0] = texCoords[index[i*polyType    ].t*2+0];
                out_texCoords[((i*tri+j)*3+1)*2+0] = texCoords[index[i*polyType+j+1].t*2+0];
                out_texCoords[((i*tri+j)*3+2)*2+0] = texCoords[index[i*polyType+j+2].t*2+0];
                out_texCoords[((i*tri+j)*3  )*2+1] = 1.0f - texCoords[index[i*polyType    ].t*2+1];
                out_texCoords[((i*tri+j)*3+1)*2+1] = 1.0f - texCoords[index[i*polyType+j+1].t*2+1];
                out_texCoords[((i*tri+j)*3+2)*2+1] = 1.0f - texCoords[index[i*polyType+j+2].t*2+1];
                // clang-format on
            }
        }
        this->vertex = VertexBuffer::upload(out_vertex, dim, vertexCount);
        this->normals = VertexBuffer::upload(out_normals, dim, vertexCount);
        this->texCoords = VertexBuffer::upload(out_texCoords, 2, vertexCount);
        this->indices.clear();
    } else {
        vertexCount = texCoords.size() / 2;
        GLuint* out_indices = new GLuint[n];
        float* out_vertex = new float[vertexCount * dim];
        float* out_normals = new float[vertexCount * dim];
        for (int i = 1; i < texCoords.size(); i += 2) {
            texCoords[i] = 1.0f - texCoords[i];
        }
        for (int i = 0; i < index.size(); i++) {
            for (int j = 0; j < dim; j++) {
                out_vertex[index[i].t * dim + j] = vertex[index[i].v * dim + j];
                out_normals[index[i].t * dim + j] = normals[index[i].n * dim + j];
            }
        }
        for (int i = 0; i < ni; i++) {
            for (int j = 0; j < tri; j++) {
                // clang-format off
                out_indices[(i*tri+j)*3  ] = index[i*polyType    ].t;
                out_indices[(i*tri+j)*3+1] = index[i*polyType+j+1].t;
                out_indices[(i*tri+j)*3+2] = index[i*polyType+j+2].t;
                // clang-format on
            }
        }
        this->vertex = VertexBuffer::upload(out_vertex, dim, vertexCount);
        this->normals = VertexBuffer::upload(out_normals, dim, vertexCount);
        this->texCoords = VertexBuffer::upload(texCoords.data(), 2, vertexCount);
        this->indices = IndexBuffer::upload(out_indices, n);
    }
}
