#include "renderer.hpp"
#include "GL/glwrapper.hpp"
#include "math/Vec2.hpp"
#include "math/Vec3.hpp"
#include "math/Mat4x4.hpp"
#include "util/SourceGetter.hpp"
#include "graphic/Shader.hpp"
#include "util/log.hpp"
#include "lgraphic/DepthMap.hpp"
#include <string>
#include "lgraphic/ShaderBuilder.hpp"
#include <cmath>
#include <ctime>

class DebugRenderable : public Renderable{
	public:
		Bufferf *source;
		Vec4 color;
		DebugRenderable(Bufferf *s,Vec4 c){
			source = s;
			color = c;
		}
		void draw(
			Shader *shader,
			MvpMatrix **mvpMatrices,
			std::string positionAttrib,
			std::string normalAttrib,
			std::string colorAttrib,
			std::string textureUniform,
			std::string colorUniform,
			std::string *mvpUniforms,
			int matrixCount
		){
			if(matrixCount){
				int i;
				for(i=0;i<matrixCount;i++){
					shader->uniformMat4x4(mvpUniforms[i],mvpMatrices[i]->setupMvp());
				}
			}
			shader->attribPointer(positionAttrib,3,*source);
			if(!colorUniform.empty())shader->uniformVec4(colorUniform,color);
			glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		}
};

Vec3 up = Vec3(0,1,0);
LShader *shader,*refShader;
PointLighting *pointLight;
SpotLighting *spotLight;
AmbientLighting *ambientLight;
DebugRenderable *obj1,*obj2,*obj3,*obj4,*obj5;
Bufferf *f1,*f2,*f3,*f4,*f5,*d_coord,*d_texCoord,*tex_Coords;
MvpMatrix mvpMatrix;
Shader depthShader(
	"attribute vec4 a_position;\n"
	"attribute vec3 a_texCoord;\n"
	"varying vec3 v_texCoord;\n"
	"void main(){\n"
	"	gl_Position = a_position;\n"
	"	v_texCoord = a_texCoord;\n"
	"}",
	"uniform samplerCube u_texture;\n"
	"varying vec3 v_texCoord;\n"
	"void main(){\n"
	"	float far = 15.0;\n"
	"	vec4 bits = vec4(far,far/255.0,far/255.0/255.0,far/255.0/255.0/255.0);\n"
	"	vec4 f = textureCube(u_texture,v_texCoord);\n"
	"	gl_FragColor = vec4(vec3(dot(bits,f))/far,1.0);\n"
	"}");
Shader texShader(
	"attribute vec4 a_position;"
	"varying vec2 v_texPos;"
	"void main(){"
	"	gl_Position = a_position;"
	"	v_texPos = (a_position.xy + 1.0) / 2.0;"
	"}",
	"varying vec2 v_texPos;"
	"uniform sampler2D u_texture;"
	"void main(){"
	"	float far = 20.0;"
	"	vec4 bits = vec4(far,far/255.0,far/255.0/255.0,far/255.0/255.0/255.0);"
	"	vec4 f = texture2D(u_texture, v_texPos);"
	"	gl_FragColor = f;"//vec4(vec3(dot(bits,f))/far,1.0);"
	"}");
Reflection *reflection;
clock_t lticks;
float obj5_coords[4*3];

void onSurfaceCreated(){
	pointLight = new PointLighting(
		Vec3(0,1,-2),
		1024,1024,
		Vec3(1,1,1),
		15,
		1.0f,
		0.2f
	);
	spotLight = new SpotLighting(
		Vec3(0,3.5,0),
		Vec3(0.1,-3,0.1),
		Vec3(0,1,0),
		M_PI*0.6,
		M_PI*0.6,
		2048,2048,
		Vec3(1,1,1)*0.7f,
		15
	);
	ambientLight = new AmbientLighting(Vec3(1, 1, 1)*0.2f);
	reflection = new Reflection(Vec3(0,1,0),Vec3(0,-2,0),2048,2048,0.5f);
	reflection->framebuffer.setClearColor(0,0,0,1);
	Framebuffer::DEFAULT.setClearColor(0,0,0,1);
	f1 = new Bufferf(GL_ARRAY_BUFFER);
	f1->bufferData(new DirectSourceGetter<float*>(new float[3*4]{-1,-1,-1,1,-1,-1,-1,-1,1,1,-1,1}),3*4*4,GL_STATIC_DRAW);
	f2 = new Bufferf(GL_ARRAY_BUFFER);
	f2->bufferData(new DirectSourceGetter<float*>(new float[3*4]{-1,3,0,1,3,0,-1,3,2,1,3,2}),3*4*4,GL_STATIC_DRAW);
	f3 = new Bufferf(GL_ARRAY_BUFFER);
	f3->bufferData(new DirectSourceGetter<float*>(new float[3*4]{-10,-2,-10,-10,-2,10,10,-2,-10,10,-2,10}),3*4*4,GL_STATIC_DRAW);
	f4 = new Bufferf(GL_ARRAY_BUFFER);
	f4->bufferData(new DirectSourceGetter<float*>(new float[3*4]{-10,4,-10,-10,4,10,10,4,-10,10,4,10}),3*4*4,GL_STATIC_DRAW);
	f5 = new Bufferf(TE_BUFFER);
	f5->bufferData(new DirectSourceGetter<float*>(obj5_coords),3*4*4,GL_DYNAMIC_DRAW);
	tex_Coords = new Bufferf(GL_ARRAY_BUFFER);
	tex_Coords->bufferData(new DirectSourceGetter<float*>(new float[2*4]{-1,-1, 1,-1, -1,1, 1,1}), 2*4*4, GL_STATIC_DRAW);
	d_coord = new Bufferf(GL_ARRAY_BUFFER);
	float x1 = -1;
	float x2 = 0;
	float y1 = -1;
	float add = 1.0f/3.0f;
	float y2 = -1+add;
	d_coord->bufferData(new DirectSourceGetter<float*>(new float[6*2*3*2]{
		 x1,y1,x2,y1,x1,y2, x2,y1,x1,y2,x2,y2,
		 x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
		 x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
		 x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
		 x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
		 x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2
	}),6*2*3*2*4,GL_STATIC_DRAW);
	d_texCoord = new Bufferf(GL_ARRAY_BUFFER);
	d_texCoord->bufferData(new DirectSourceGetter<float*>(new float[6*2*3*3]{
		 -1,-1,-1, 1,-1,-1, -1,1,-1,    1,-1,-1, -1,1,-1, 1,1,-1, //negZ
		 -1,-1,1, 1,-1,1, -1,1,1,       1,-1,1, -1,1,1, 1,1,1, //posZ
		 -1,-1,-1, 1,-1,-1, -1,-1,1,    1,-1,-1, -1,-1,1, 1,-1,1, //negY
		 -1,1,-1, 1,1,-1, -1,1,1,       1,1,-1, -1,1,1, 1,1,1, //posY
		 -1,-1,-1, -1,1,-1, -1,-1,1,    -1,1,-1, -1,-1,1, -1,1,1, //negX
		 1,-1,-1, 1,1,-1, 1,-1,1,       1,1,-1, 1,-1,1, 1,1,1, //posX
	}),6*2*3*3*4,GL_STATIC_DRAW);
	obj1 = new DebugRenderable(f1,Vec4(1,0,0,1));
	obj2 = new DebugRenderable(f2,Vec4(0,1,0,1));
	obj3 = new DebugRenderable(f3,Vec4(1,1,1,1));
	obj4 = new DebugRenderable(f4,Vec4(1,1,0,1));
	obj5 = new DebugRenderable(f5,Vec4(0,0,1,1));
	mvpMatrix.model.setIdentity();
	mvpMatrix.view.setLookAt(Vec3(7,2,0),Vec3(0,-1,0),up);
	ShaderBuilder b;
	b.setVertexPositionAttrib("a_position");
	b.setFragmentColorUniform("u_color");
	b.setPositionMvpUniform("u_mvpMatrix");
	b.addLighting(*ambientLight);
	b.addLighting(*pointLight);
	b.addLighting(*spotLight);
	shader = b.build();
	ShaderBuilder rb;
	rb.setVertexPositionAttrib("a_position");
	rb.setFragmentColorUniform("u_color");
	rb.setPositionMvpUniform("u_mvpMatrix");
	rb.addLighting(*ambientLight);
	rb.addLighting(*pointLight);
	rb.addLighting(*spotLight);
	rb.addReflection(*reflection);
	refShader = rb.build();
	lticks = clock();
}

void onSurfaceChanged(int newWidth,int newHeight){
	Framebuffer::DEFAULT.viewport(newWidth,newHeight);
	float as = (float)newWidth/(float)newHeight;
	mvpMatrix.projection.setFrustum(-as,as,-1,1,1,30);
}

void onDrawFrame(){
	clock_t ticks = clock();
	float dt = (ticks - lticks)/(float)CLOCKS_PER_SEC;
	lticks = ticks;
	Vec3 plp = pointLight->position;
	pointLight->position += Vec3(plp.z*dt, plp.z*dt/2.0, -plp.x*dt);
	plp = pointLight->position;
	obj5_coords[0]  = plp.x - 0.3f;
	obj5_coords[1]  = plp.y;
	obj5_coords[2]  = plp.z - 0.3f;
	
	obj5_coords[3]  = plp.x + 0.3f;
	obj5_coords[4]  = plp.y;
	obj5_coords[5]  = plp.z - 0.3f;
	
	obj5_coords[6]  = plp.x - 0.3f;
	obj5_coords[7]  = plp.y;
	obj5_coords[8]  = plp.z + 0.3f;
	
	obj5_coords[9]  = plp.x + 0.3f;
	obj5_coords[10] = plp.y;
	obj5_coords[11] = plp.z + 0.3f;
	glEnable(GL_DEPTH_TEST);
	Framebuffer::DEFAULT.bind();
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	f1->init();
	f2->init();
	f3->init();
	f4->init();
	d_coord->init();
	d_texCoord->init();
	tex_Coords->init();
	shader->init();
	refShader->init();
	pointLight->init();
	spotLight->init();
	reflection->init();
	Renderable *ptrs[] = {obj1,obj2,obj3,obj4};
	pointLight->draw(mvpMatrix.model,ptrs,4);
	spotLight->draw(mvpMatrix.model,ptrs,4);
	MvpMatrix *mvpPtr = &mvpMatrix;
	std::string mvpUs[] = {"u_mvpMatrix"};
	shader->use();
	shader->lighting(*pointLight,mvpMatrix.model);
	shader->lighting(*spotLight,mvpMatrix.model);
	shader->lighting(*ambientLight);
	reflection->bind();
	reflection->getReflectionModel(mvpMatrix.model);
	obj1->draw(shader,&mvpPtr,"a_position","","","","u_color",mvpUs,1);
	obj2->draw(shader,&mvpPtr,"a_position","","","","u_color",mvpUs,1);
	obj4->draw(shader,&mvpPtr,"a_position","","","","u_color",mvpUs,1);
	reflection->unbind();
	mvpMatrix.model.setIdentity();
	obj1->draw(shader,&mvpPtr,"a_position","","","","u_color",mvpUs,1);
	obj2->draw(shader,&mvpPtr,"a_position","","","","u_color",mvpUs,1);
	obj4->draw(shader,&mvpPtr,"a_position","","","","u_color",mvpUs,1);
	obj5->draw(shader,&mvpPtr,"a_position","","","","u_color",mvpUs,1);
	refShader->use();
	refShader->lighting(*ambientLight);
	refShader->reflection(*reflection);
	refShader->lighting(*pointLight,mvpMatrix.model);
	refShader->lighting(*spotLight,mvpMatrix.model);
	obj3->draw(refShader,&mvpPtr,"a_position","","","","u_color",mvpUs,1);
	depthShader.init();
	depthShader.use();
	depthShader.attribPointer("a_position",2,*d_coord);
	depthShader.attribPointer("a_texCoord",3,*d_texCoord);
	depthShader.uniformCubeMap("u_texture",pointLight->depthMap->texture,0);
	//glDrawArrays(GL_TRIANGLES,0,6*2*3);
	texShader.init();
	texShader.use();
	texShader.attribPointer("a_position", 2, *tex_Coords);
	texShader.uniformTexture("u_texture", reflection->texture, 0);
	//glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
