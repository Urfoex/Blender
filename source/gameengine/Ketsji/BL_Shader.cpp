/** \file gameengine/Ketsji/BL_Shader.cpp
 *  \ingroup ketsji
 */

#include "GL/glew.h"

#include <iostream>
#include <sstream>

#include "BL_Shader.h"
#include "BL_Material.h"

#include "MT_assert.h"
#include "MT_Matrix4x4.h"
#include "MT_Matrix3x3.h"
#include "KX_PyMath.h"
#include "MEM_guardedalloc.h"

#include "RAS_GLExtensionManager.h"
#include "RAS_MeshObject.h"
#include "RAS_IRasterizer.h"


// TODO use better logging
#define spit(x) std::cout << __FILE__ << ":" << __LINE__ << ":" <<  x << std::endl;

#define SORT_UNIFORMS 1
constexpr int UNIFORM_MAX_LEN = (int)sizeof(float)*16;
const int MAX_LOG_LEN = 262144; // bounds

template<typename data_t, unsigned int size>
BL_Uniform<data_t, size>::BL_Uniform()
:	mLoc(-1),
	mDirty(true),
	mTranspose(0)
{
#ifdef SORT_UNIFORMS
#endif
}

template<typename data_t, unsigned int size>
BL_Uniform<data_t, size>::~BL_Uniform()
{
#ifdef SORT_UNIFORMS
#endif
}

template<>
void BL_Uniform<int, 1>::Apply(){
	glUniform1iARB(mLoc, *mData.data());
}

template<>
void BL_Uniform<int, 2>::Apply(){
	glUniform2ivARB(mLoc,1,mData.data());
}

template<>
void BL_Uniform<int, 3>::Apply(){
	glUniform3ivARB(mLoc,1,mData.data());
}

template<>
void BL_Uniform<int, 4>::Apply(){
	glUniform4ivARB(mLoc,1,mData.data());
}

template<>
void BL_Uniform<float, 1>::Apply(){
	glUniform1fARB(mLoc,*mData.data());
}

template<>
void BL_Uniform<float, 2>::Apply(){
	glUniform2fvARB(mLoc,1, mData.data());
}

template<>
void BL_Uniform<float, 3>::Apply(){
	glUniform3fvARB(mLoc,1,mData.data());
}

template<>
void BL_Uniform<float, 4>::Apply(){
	glUniform4fvARB(mLoc,1,mData.data());
}

template<>
void BL_Uniform<float, 9>::Apply(){
	glUniformMatrix3fvARB(mLoc, 1, mTranspose?GL_TRUE:GL_FALSE,mData.data());
}

template<>
void BL_Uniform<float, 16>::Apply(){
	glUniformMatrix4fvARB(mLoc, 1, mTranspose?GL_TRUE:GL_FALSE,mData.data());
}


template<typename data_t, unsigned int size>
void BL_Uniform<data_t, size>::Apply(class BL_Shader *shader)
{
#ifdef SORT_UNIFORMS
	MT_assert(mData);

	if (!mDirty)
		return;

	Apply();
	mDirty = false;
#endif
}

template<typename data_t, unsigned int size>
void BL_Uniform<data_t, size>::SetData( std::array<data_t, size>&& data, int location, bool transpose ) {
#ifdef SORT_UNIFORMS
	mData = std::move(data);
	mLoc	= location;
	mTranspose = transpose;
	mDirty	= true;
#endif
}

std::shared_ptr<BL_ShaderManager> BL_ShaderManager::m_shaderManager = std::make_shared<BL_ShaderManager>();

shared_ptr< BL_Shader > BL_ShaderManager::AddShader(std::string shaderName) {
	auto it = m_shaderLibrary.find(shaderName);
	if( it != std::end(m_shaderLibrary)){
		return it->second;
	}else{
		auto pair = m_shaderLibrary.emplace(std::make_pair(shaderName, std::make_shared<BL_Shader>()));
		pair.first->second->SetName(shaderName);
		return pair.first->second;
	}
}

shared_ptr< BL_Shader > BL_ShaderManager::AddShader() {
	std::string name = "Shader::" + std::to_string(NextShaderIndex());
	auto pair = m_shaderLibrary.emplace(std::make_pair(name, std::make_shared<BL_Shader>()));
	pair.first->second->SetName(name);
	return pair.first->second;
}

unsigned long BL_ShaderManager::numAvailableShader()
{
	return m_shaderLibrary.size();
}

std::string BL_ShaderManager::availableShader()
{
	std::stringstream ss;
	for( auto shader : m_shaderLibrary)
		ss << shader.first << "\n";
	return ss.str();
}

void BL_ShaderManager::RemoveShader(shared_ptr<BL_Shader>&& shader) {
	auto item = m_shaderLibrary.find(shader->GetName());
	if( item == std::end(m_shaderLibrary)){
		return;
	}
	if(item->second.use_count() <= 2){
		m_shaderLibrary.erase(shader->GetName());
	}
}



bool BL_Shader::Ok()const
{
	return (mShader !=0 && mOk && mUse);
}

BL_Shader::BL_Shader()
:	PyObjectPlus(),
	mShader(0),
	mPass(1),
	mOk(false),
	mUse(false),
	mAttr(0),
	mVertProg(""),
	mFragProg(""),
	mError(false),
	mDirty(true)
{
	// if !GLEW_ARB_shader_objects this class will not be used
	//for (int i=0; i<MAXTEX; i++) {
	//	mSampler[i] = BL_Sampler();
	//}
}

BL_Shader::~BL_Shader()
{
	//for (int i=0; i<MAXTEX; i++) {
	//	if (mSampler[i].mOwn) {
	//		if (mSampler[i].mTexture)
	//			mSampler[i].mTexture->DeleteTex();
	//	}
	//}
	ClearUniforms();

	if ( mShader ) {
		glDeleteObjectARB(mShader);
		mShader = 0;
	}
	mVertProg.clear();
	mFragProg.clear();
	mOk			= false;
	glUseProgramObjectARB(0);
}

void BL_Shader::ClearUniforms()
{
	mUniforms.clear();
	mPreDef.clear();
}


std::shared_ptr<BL_UniformBase> BL_Shader::FindUniform(const int location)
{
#ifdef SORT_UNIFORMS
	for( auto& uniform : mUniforms){
		if( uniform->GetLocation() == location){
			return uniform;
		}
	}
#endif
	return nullptr;
}


template<typename data_t, int size>
void BL_Shader::SetUniformv(int location, std::array<data_t, size>&& param, bool transpose)
{
#ifdef SORT_UNIFORMS
	auto uni = FindUniform(location);
	if (uni) {
		(dynamic_cast<BL_Uniform<data_t, size>*>(uni.get()))->SetData(std::move(param),location, transpose);
	}
	else {
		uni = std::make_shared<BL_Uniform<data_t, size>>();
		(dynamic_cast<BL_Uniform<data_t, size>*>(uni.get()))->SetData(std::move(param),location, transpose);
		mUniforms.push_back(uni);
	}
	mDirty = true;
#endif
}

void BL_Shader::ApplyShader()
{
#ifdef SORT_UNIFORMS
	if (!mDirty){
		return;
	}

	for (unsigned int i=0; i<mUniforms.size(); i++){
		mUniforms[i]->Apply(this);
	}

	mDirty = false;
#endif
}

void BL_Shader::UnloadShader()
{
	//
}


bool BL_Shader::LinkProgram()
{
	int vertlen = 0, fraglen = 0, proglen = 0;
	int vertstatus = 0, fragstatus = 0, progstatus = 0;
	unsigned int tmpVert = 0, tmpFrag = 0, tmpProg = 0;
	int char_len = 0;
	char *logInf = nullptr;

	if (mError){
		return OnProgramError(tmpVert, tmpFrag, tmpProg);
	}
	
	if (mVertProg.empty() || mFragProg.empty()) {
		spit("Invalid GLSL sources");
		return false;
	}
	if ( !GLEW_ARB_fragment_shader) {
		spit("Fragment shaders not supported");
		return false;
	}
	if ( !GLEW_ARB_vertex_shader) {
		spit("Vertex shaders not supported");
		return false;
	}
	
	// -- vertex shader ------------------
	tmpVert = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	auto vertSource = mVertProg.c_str();
	glShaderSourceARB(tmpVert, 1, (const char**)&vertSource, 0);
	glCompileShaderARB(tmpVert);
	glGetObjectParameterivARB(tmpVert, GL_OBJECT_INFO_LOG_LENGTH_ARB,(GLint*) &vertlen);
	
	// print info if any
	if ( vertlen > 0 && vertlen < MAX_LOG_LEN) {
		logInf = (char*)MEM_mallocN(vertlen, "vert-log");
		glGetInfoLogARB(tmpVert, vertlen, (GLsizei*)&char_len, logInf);
		if (char_len >0) {
			spit("---- Vertex Shader Error ----");
			spit(logInf);
		}
		MEM_freeN(logInf);
		logInf=0;
	}
	// check for compile errors
	glGetObjectParameterivARB(tmpVert, GL_OBJECT_COMPILE_STATUS_ARB,(GLint*)&vertstatus);
	if (!vertstatus) {
		spit("---- Vertex shader failed to compile ----");
		return OnProgramError(tmpVert, tmpFrag, tmpProg);
	}

	// -- fragment shader ----------------
	tmpFrag = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	auto fragSource = mFragProg.c_str();
	glShaderSourceARB(tmpFrag, 1,(const char**)&fragSource, 0);
	glCompileShaderARB(tmpFrag);
	glGetObjectParameterivARB(tmpFrag, GL_OBJECT_INFO_LOG_LENGTH_ARB, (GLint*) &fraglen);
	if (fraglen >0 && fraglen < MAX_LOG_LEN) {
		logInf = (char*)MEM_mallocN(fraglen, "frag-log");
		glGetInfoLogARB(tmpFrag, fraglen,(GLsizei*) &char_len, logInf);
		if (char_len >0) {
			spit("---- Fragment Shader Error ----");
			spit(logInf);
		}
		MEM_freeN(logInf);
		logInf=0;
	}

	glGetObjectParameterivARB(tmpFrag, GL_OBJECT_COMPILE_STATUS_ARB, (GLint*) &fragstatus);
	if (!fragstatus) {
		spit("---- Fragment shader failed to compile ----");
		return OnProgramError(tmpVert, tmpFrag, tmpProg);
	}

	
	// -- program ------------------------
	//  set compiled vert/frag shader & link
	tmpProg = glCreateProgramObjectARB();
	glAttachObjectARB(tmpProg, tmpVert);
	glAttachObjectARB(tmpProg, tmpFrag);
	glLinkProgramARB(tmpProg);
	glGetObjectParameterivARB(tmpProg, GL_OBJECT_INFO_LOG_LENGTH_ARB, (GLint*) &proglen);
	glGetObjectParameterivARB(tmpProg, GL_OBJECT_LINK_STATUS_ARB, (GLint*) &progstatus);
	

	if (proglen > 0 && proglen < MAX_LOG_LEN) {
		logInf = (char*)MEM_mallocN(proglen, "prog-log");
		glGetInfoLogARB(tmpProg, proglen, (GLsizei*)&char_len, logInf);
		if (char_len >0) {
			spit("---- GLSL Program ----");
			spit(logInf);
		}
		MEM_freeN(logInf);
		logInf=0;
	}

	if (!progstatus) {
		spit("---- GLSL program failed to link ----");
		return OnProgramError(tmpVert, tmpFrag, tmpProg);
	}

	// set
	mShader = tmpProg;
	glDeleteObjectARB(tmpVert);
	glDeleteObjectARB(tmpFrag);
	mOk		= 1;
	mError = 0;
	return true;
}

bool BL_Shader::OnProgramError(unsigned int tmpVert, unsigned int tmpFrag, unsigned int tmpProg) {
	if (tmpVert) {
		glDeleteObjectARB(tmpVert);
		tmpVert=0;
	}
	if (tmpFrag) {
		glDeleteObjectARB(tmpFrag);
		tmpFrag=0;
	}
	
	if (tmpProg) {
		glDeleteObjectARB(tmpProg);
		tmpProg=0;
	}
	
	mOk		= false;
	mUse	= false;
	mError	= true;
	return false;
}


std::string BL_Shader::GetVertPtr()
{
	return mVertProg;
}

std::string BL_Shader::GetFragPtr()
{
	return mFragProg;
}

void BL_Shader::SetVertPtr( std::string vert )
{
	mVertProg = vert;
}

void BL_Shader::SetFragPtr( std::string frag )
{
	mFragProg = frag;
}

unsigned int BL_Shader::GetProg()
{ 
	return mShader;
}
//
//const BL_Sampler* BL_Shader::GetSampler(int i)
//{
//	MT_assert(i<=MAXTEX);
//	return &mSampler[i];
//}

void BL_Shader::SetSampler(int loc, int unit)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		glUniform1iARB(loc, unit);
	}
}
//
//void BL_Shader::InitializeSampler(int unit, BL_Texture* texture)
//{
//	MT_assert(unit<=MAXTEX);
//	mSampler[unit].mTexture = texture;
//	mSampler[unit].mLoc =-1;
//	mSampler[unit].mOwn = 0;
//}

void BL_Shader::SetProg(bool enable)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		if (	mShader != 0 && mOk && enable) {
			glUseProgramObjectARB(mShader);
		}
		else {
			glUseProgramObjectARB(0);
		}
	}
}

string BL_Shader::GetName() {
	return mName;
}

void BL_Shader::SetName( string name ) {
	mName = name;
}


void BL_Shader::Update( const RAS_MeshSlot & ms, RAS_IRasterizer* rasty )
{
	if (!Ok() || !mPreDef.size()) {
		return;
	}

	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		MT_Matrix4x4 model;
		model.setValue(ms.m_OpenGLMatrix);
		const MT_Matrix4x4& view = rasty->GetViewMatrix();

		if (mAttr==SHD_TANGENT){
			ms.m_mesh->SetMeshModified(true);
		}
			
		for (auto& uni : mPreDef)
		{
			if (uni->mLoc == -1) continue;

			switch (uni->mType)
			{
				case MODELMATRIX:
					{
						SetUniform(uni->mLoc, model);
						break;
					}
				case MODELMATRIX_TRANSPOSE:
					{
						SetUniform(uni->mLoc, model, true);
						break;
					}
				case MODELMATRIX_INVERSE:
					{
						model.invert();
						SetUniform(uni->mLoc, model);
						break;
					}
				case MODELMATRIX_INVERSETRANSPOSE:
					{
						model.invert();
						SetUniform(uni->mLoc, model, true);
						break;
					}
				case MODELVIEWMATRIX:
					{
						SetUniform(uni->mLoc, view*model);
						break;
					}

				case MODELVIEWMATRIX_TRANSPOSE:
					{
						MT_Matrix4x4 mat(view*model);
						SetUniform(uni->mLoc, mat, true);
						break;
					}
				case MODELVIEWMATRIX_INVERSE:
					{
						MT_Matrix4x4 mat(view*model);
						mat.invert();
						SetUniform(uni->mLoc, mat);
						break;
					}
				case MODELVIEWMATRIX_INVERSETRANSPOSE:
					{
						MT_Matrix4x4 mat(view*model);
						mat.invert();
						SetUniform(uni->mLoc, mat, true);
						break;
					}
				case CAM_POS:
					{
						MT_Point3 pos(rasty->GetCameraPosition());
						SetUniform(uni->mLoc, pos);
						break;
					}
				case VIEWMATRIX:
					{
						SetUniform(uni->mLoc, view);
						break;
					}
				case VIEWMATRIX_TRANSPOSE:
					{
						SetUniform(uni->mLoc, view, true);
						break;
					}
				case VIEWMATRIX_INVERSE:
					{
						MT_Matrix4x4 viewinv = view;
						viewinv.invert();
						SetUniform(uni->mLoc, view);
						break;
					}
				case VIEWMATRIX_INVERSETRANSPOSE:
					{
						MT_Matrix4x4 viewinv = view;
						viewinv.invert();
						SetUniform(uni->mLoc, view, true);
						break;
					}
				case CONSTANT_TIMER:
					{
						SetUniform(uni->mLoc, (float)rasty->GetTime());
						break;
					}
				default:
					break;
			}
		}
	}
}


int BL_Shader::GetAttribLocation(const char *name)
{
	if (GLEW_ARB_fragment_shader &&
	    GLEW_ARB_vertex_shader &&
	    GLEW_ARB_shader_objects)
	{
		return glGetAttribLocationARB(mShader, name);
	}

	return -1;
}

void BL_Shader::BindAttribute(const char *attr, int loc)
{
	if (GLEW_ARB_fragment_shader &&
	    GLEW_ARB_vertex_shader &&
	    GLEW_ARB_shader_objects )
	{
		glBindAttribLocationARB(mShader, loc, attr);
	}
}

int BL_Shader::GetUniformLocation(const char *name)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects
		)
	{
		MT_assert(mShader!=0);
		int location = glGetUniformLocationARB(mShader, name);
		if (location == -1)
			spit("Invalid uniform value: " << name << ".");
		return location;
	}

	return -1;
}

void BL_Shader::SetUniform(int uniform, const MT_Tuple2& vec)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		float value[2];
		vec.getValue(value);
		glUniform2fvARB(uniform, 1, value);
	}

}

void BL_Shader::SetUniform(int uniform, const MT_Tuple3& vec)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		float value[3];
		vec.getValue(value);
		glUniform3fvARB(uniform, 1, value);
	}
}

void BL_Shader::SetUniform(int uniform, const MT_Tuple4& vec)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		float value[4];
		vec.getValue(value);
		glUniform4fvARB(uniform, 1, value);
	}
}

void BL_Shader::SetUniform(int uniform, const unsigned int& val)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		glUniform1iARB(uniform, val);
	}
}

void BL_Shader::SetUniform(int uniform, const int val)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		glUniform1iARB(uniform, val);
	}
}

void BL_Shader::SetUniform(int uniform, const float& val)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		glUniform1fARB(uniform, val);
	}
}

void BL_Shader::SetUniform(int uniform, const MT_Matrix4x4& vec, bool transpose)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		float value[16];
		// note: getValue gives back column major as needed by OpenGL
		vec.getValue(value);
		glUniformMatrix4fvARB(uniform, 1, transpose?GL_TRUE:GL_FALSE, value);
	}
}

void BL_Shader::SetUniform(int uniform, const MT_Matrix3x3& vec, bool transpose)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		float value[9];
		value[0] = (float)vec[0][0]; value[1] = (float)vec[1][0]; value[2] = (float)vec[2][0]; 
		value[3] = (float)vec[0][1]; value[4] = (float)vec[1][1]; value[5] = (float)vec[2][1]; 
		value[6] = (float)vec[0][2]; value[7] = (float)vec[1][2]; value[8] = (float)vec[2][2];
		glUniformMatrix3fvARB(uniform, 1, transpose?GL_TRUE:GL_FALSE, value);
	}
}

void BL_Shader::SetUniform(int uniform, const float* val, int len)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		if (len == 2) {
			glUniform2fvARB(uniform, 1,(GLfloat*)val);
		}else if (len == 3){
			glUniform3fvARB(uniform, 1,(GLfloat*)val);
		}else if (len == 4){
			glUniform4fvARB(uniform, 1,(GLfloat*)val);
		}else{
			MT_assert(0);
		}
	}
}

void BL_Shader::SetUniform(int uniform, const int* val, int len)
{
	if ( GLEW_ARB_fragment_shader &&
		GLEW_ARB_vertex_shader &&
		GLEW_ARB_shader_objects 
		)
	{
		if (len == 2) {
			glUniform2ivARB(uniform, 1, (GLint*)val);
		}else if (len == 3){
			glUniform3ivARB(uniform, 1, (GLint*)val);
		}else if (len == 4){
			glUniform4ivARB(uniform, 1, (GLint*)val);
		}else{
			MT_assert(0);
		}
	}
}

#ifdef WITH_PYTHON

PyMethodDef BL_Shader::Methods[] = 
{
	// creation
	KX_PYMETHODTABLE( BL_Shader, setSource ),
	KX_PYMETHODTABLE( BL_Shader, delSource ),
	KX_PYMETHODTABLE( BL_Shader, getVertexProg ),
	KX_PYMETHODTABLE( BL_Shader, getFragmentProg ),
	KX_PYMETHODTABLE( BL_Shader, setNumberOfPasses ),
	KX_PYMETHODTABLE( BL_Shader, validate),
	KX_PYMETHODTABLE( BL_Shader, getName),
	KX_PYMETHODTABLE( BL_Shader, numAvailableShader),
	KX_PYMETHODTABLE( BL_Shader, listAvailableShader),
	/// access functions
	KX_PYMETHODTABLE( BL_Shader, isValid),
	KX_PYMETHODTABLE( BL_Shader, setUniform1f ),
	KX_PYMETHODTABLE( BL_Shader, setUniform2f ),
	KX_PYMETHODTABLE( BL_Shader, setUniform3f ),
	KX_PYMETHODTABLE( BL_Shader, setUniform4f ),
	KX_PYMETHODTABLE( BL_Shader, setUniform1i ),
	KX_PYMETHODTABLE( BL_Shader, setUniform2i ),
	KX_PYMETHODTABLE( BL_Shader, setUniform3i ),
	KX_PYMETHODTABLE( BL_Shader, setUniform4i ),
	KX_PYMETHODTABLE( BL_Shader, setAttrib ),

	KX_PYMETHODTABLE( BL_Shader, setUniformfv ),
	KX_PYMETHODTABLE( BL_Shader, setUniformiv ),
	KX_PYMETHODTABLE( BL_Shader, setUniformDef ),

	KX_PYMETHODTABLE( BL_Shader, setSampler  ),
	KX_PYMETHODTABLE( BL_Shader, setUniformMatrix4 ),
	KX_PYMETHODTABLE( BL_Shader, setUniformMatrix3 ),

	{NULL,NULL} //Sentinel
};

PyAttributeDef BL_Shader::Attributes[] = {
	{ NULL }	//Sentinel
};

PyTypeObject BL_Shader::Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"BL_Shader",
	sizeof(PyObjectPlus_Proxy),
	0,
	py_base_dealloc,
	0,
	0,
	0,
	0,
	py_base_repr,
	0,0,0,0,0,0,0,0,0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	0,0,0,0,0,0,0,
	Methods,
	0,
	0,
	&PyObjectPlus::Type,
	0,0,0,0,0,0,
	py_base_new
};

KX_PYMETHODDEF_DOC( BL_Shader, setSource," setSource(vertexProgram, fragmentProgram)" )
{
	if (mShader !=0 && mOk  )
	{
		// already set...
		Py_RETURN_NONE;
	}
	char *v,*f;
	int apply=0;
	if ( PyArg_ParseTuple(args, "ssi:setSource", &v, &f, &apply) )
	{
		mVertProg = std::string(v);
		mFragProg = std::string(f);
		if ( LinkProgram() ) {
			glUseProgramObjectARB( mShader );
			mUse = apply!=0;
			Py_RETURN_NONE;
		}
		mVertProg = "";
		mFragProg = "";
		mUse = false;
		Py_RETURN_NONE;
	}
	return NULL;
}


KX_PYMETHODDEF_DOC( BL_Shader, delSource, "delSource( )" )
{
	ClearUniforms();
	glUseProgramObjectARB(0);

	glDeleteObjectARB(mShader);
	mShader		= false;
	mOk			= false;
	mUse		= false;
	Py_RETURN_NONE;
}

KX_PYMETHODDEF_DOC( BL_Shader, isValid, "isValid()" )
{
	return PyBool_FromLong(( mShader !=0 &&  mOk ));
}

KX_PYMETHODDEF_DOC( BL_Shader, getVertexProg, "getVertexProg( )" )
{
	return PyUnicode_FromString(mVertProg.c_str());
}

KX_PYMETHODDEF_DOC( BL_Shader, getFragmentProg, "getFragmentProg( )" )
{
	return PyUnicode_FromString(mFragProg.c_str());
}

KX_PYMETHODDEF_DOC( BL_Shader, validate, "validate()")
{
	if (mError) {
		Py_RETURN_NONE;
	}
	if (mShader==0) {
		PyErr_SetString(PyExc_TypeError, "shader.validate(): BL_Shader, invalid shader object");
		return NULL;
	}
	int stat = 0;
	glValidateProgramARB(mShader);
	glGetObjectParameterivARB(mShader, GL_OBJECT_VALIDATE_STATUS_ARB,(GLint*) &stat);


	if (stat > 0 && stat < MAX_LOG_LEN) {
		int char_len=0;
		char *logInf = (char*)MEM_mallocN(stat, "validate-log");

		glGetInfoLogARB(mShader, stat,(GLsizei*) &char_len, logInf);
		if (char_len >0) {
			spit("---- GLSL Validation ----");
			spit(logInf);
		}
		MEM_freeN(logInf);
		logInf=0;
	}
	Py_RETURN_NONE;
}

KX_PYMETHODDEF_DOC( BL_Shader, getName, "getName()")
{
	return PyUnicode_FromString(mName.c_str());
}

KX_PYMETHODDEF_DOC( BL_Shader, numAvailableShader, "numAvailableShader()")
{
	return PyLong_FromUnsignedLong(BL_ShaderManager::Instance()->numAvailableShader());
}

KX_PYMETHODDEF_DOC( BL_Shader, listAvailableShader, "listAvailableShader()")
{
	return PyUnicode_FromString(BL_ShaderManager::Instance()->availableShader().c_str());
}

KX_PYMETHODDEF_DOC( BL_Shader, setSampler, "setSampler(name, index)" )
{
	if (mError) {
		Py_RETURN_NONE;
	}

	const char *uniform;
// 	int index=-1;
	std::array<int,1> index = {{-1}};
	if (PyArg_ParseTuple(args, "si:setSampler", &uniform, index.data()))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1) {
			if (index[0] >= MAXTEX || index[0] < 0)
				spit("Invalid texture sample index: " << index[0]);

#ifdef SORT_UNIFORMS
				SetUniformv<int,1>(loc, std::move(index));
#else
			SetUniform(loc, index);
#endif
			//if (index <= MAXTEX)
			//	mSampler[index].mLoc = loc;
			//else
			//	spit("Invalid texture sample index: " << index);
		}
		Py_RETURN_NONE;
	}
	return NULL;
}

KX_PYMETHODDEF_DOC( BL_Shader, setNumberOfPasses, "setNumberOfPasses( max-pass )" )
{
	int pass = 1;
	if (!PyArg_ParseTuple(args, "i:setNumberOfPasses", &pass)){
		return NULL;
	}

	mPass = 1;
	Py_RETURN_NONE;
}

/// access functions
KX_PYMETHODDEF_DOC( BL_Shader, setUniform1f, "setUniform1f(name, fx)" )
{
	if (mError) {
		Py_RETURN_NONE;
	}

	const char *uniform;
	std::array<float,1> value;
	if (PyArg_ParseTuple(args, "sf:setUniform1f", &uniform, value.data() ))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1)
		{
#ifdef SORT_UNIFORMS
			SetUniformv<float,1>(loc, std::move(value));
#else
			SetUniform( loc, (float)value );
#endif
		}
		Py_RETURN_NONE;
	}
	return NULL;
}


KX_PYMETHODDEF_DOC( BL_Shader, setUniform2f, "setUniform2f(name, fx, fy)")
{
	if (mError) {
		Py_RETURN_NONE;
	}
	const char *uniform;
	std::array<float,2> array = {{0, 0}};
	if (PyArg_ParseTuple(args, "sff:setUniform2f", &uniform, &(array.data())[0],&(array.data())[1] ))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1)
		{
#ifdef SORT_UNIFORMS
			SetUniformv<float,2>(loc, std::move(array));
#else
			SetUniform(loc, array, 2);
#endif
		}
		Py_RETURN_NONE;
	}
	return NULL;
}


KX_PYMETHODDEF_DOC( BL_Shader, setUniform3f, "setUniform3f(name, fx,fy,fz) ")
{
	if (mError) {
		Py_RETURN_NONE;
	}
	const char *uniform;
	std::array<float,3> array = {{0, 0, 0}};
	if (PyArg_ParseTuple(args, "sfff:setUniform3f", &uniform, &(array.data())[0],&(array.data())[1],&(array.data())[2]))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1)
		{
#ifdef SORT_UNIFORMS
			SetUniformv<float,3>(loc, std::move(array));
#else
			SetUniform(loc, array, 3);
#endif
		}
		Py_RETURN_NONE;

	}
	return NULL;
}


KX_PYMETHODDEF_DOC( BL_Shader, setUniform4f, "setUniform4f(name, fx,fy,fz, fw) ")
{
	if (mError) {
		Py_RETURN_NONE;
	}
	const char *uniform;
	std::array<float,4> array = {{0, 0, 0, 0}};
	if (PyArg_ParseTuple(args, "sffff:setUniform4f", &uniform, &(array.data())[0],&(array.data())[1],&(array.data())[2], &(array.data())[3]))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1)
		{
#ifdef SORT_UNIFORMS
			SetUniformv<float,4>(loc, std::move(array));
#else
			SetUniform(loc, array, 4);
#endif
		}
		Py_RETURN_NONE;
	}
	return NULL;
}


KX_PYMETHODDEF_DOC( BL_Shader, setUniform1i, "setUniform1i(name, ix)" )
{
	if (mError) {
		Py_RETURN_NONE;
	}
	const char *uniform;
	std::array<int,1> value = {{0}};
	if (PyArg_ParseTuple(args, "si:setUniform1i", &uniform, value.data() ))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1)
		{
#ifdef SORT_UNIFORMS
			SetUniformv<int,1>(loc, std::move(value));
#else
			SetUniform(loc, (int)value);
#endif
		}
		Py_RETURN_NONE;
	}
	return NULL;
}


KX_PYMETHODDEF_DOC( BL_Shader, setUniform2i, "setUniform2i(name, ix, iy)")
{
	if (mError) {
		Py_RETURN_NONE;
	}
	const char *uniform;
	std::array<int,2> array = {{0, 0}};
	if (PyArg_ParseTuple(args, "sii:setUniform2i", &uniform, &(array.data())[0],&(array.data())[1] ))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1)
		{
#ifdef SORT_UNIFORMS
			SetUniformv<int,2>(loc, std::move(array));
#else
			SetUniform(loc, array, 2);
#endif
		}
		Py_RETURN_NONE;
	}
	return NULL;
}


KX_PYMETHODDEF_DOC( BL_Shader, setUniform3i, "setUniform3i(name, ix,iy,iz) ")
{
	if (mError) {
		Py_RETURN_NONE;
	}

	const char *uniform;
	std::array<int,3> array = {{0, 0, 0}};
	if (PyArg_ParseTuple(args, "siii:setUniform3i", &uniform, &(array.data())[0],&(array.data())[1],&(array.data())[2]))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1)
		{
#ifdef SORT_UNIFORMS
			SetUniformv<int,3>(loc, std::move(array));
#else
			SetUniform(loc, array, 3);
#endif
		}
		Py_RETURN_NONE;
	}
	return NULL;
}

KX_PYMETHODDEF_DOC( BL_Shader, setUniform4i, "setUniform4i(name, ix,iy,iz, iw) ")
{
	if (mError) {
		Py_RETURN_NONE;
	}
	const char *uniform;
	std::array<int,4> array = {{0, 0, 0,0}};
	if (PyArg_ParseTuple(args, "siiii:setUniform4i", &uniform, &(array.data())[0],&(array.data())[1],&(array.data())[2], &(array.data())[3] ))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1)
		{
#ifdef SORT_UNIFORMS
			SetUniformv<int,4>(loc, std::move(array));
#else
			SetUniform(loc, array, 4);
#endif
		}
		Py_RETURN_NONE;
	}
	return NULL;
}

KX_PYMETHODDEF_DOC( BL_Shader, setUniformfv, "setUniformfv( float (list2 or list3 or list4) )")
{
	if (mError) {
		Py_RETURN_NONE;
	}
	const char *uniform = "";
	PyObject *listPtr =0;
	float array_data[4] = {0.f,0.f,0.f,0.f};

	if (PyArg_ParseTuple(args, "sO:setUniformfv", &uniform, &listPtr))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1)
		{
			if (PySequence_Check(listPtr))
			{
				unsigned int list_size = PySequence_Size(listPtr);
				
				for (unsigned int i=0; (i<list_size && i<4); i++)
				{
					PyObject *item = PySequence_GetItem(listPtr, i);
					array_data[i] = (float)PyFloat_AsDouble(item);
					Py_DECREF(item);
				}

				switch (list_size)
				{
				case 2:
					{
						std::array<float,2> array2 = {{ array_data[0],array_data[1] }};
#ifdef SORT_UNIFORMS
						SetUniformv<float,2>(loc, std::move(array2));
#else
						SetUniform(loc, array2, 2);
#endif
						Py_RETURN_NONE;
					} break;
				case 3:
					{
						std::array<float,3> array3 = {{ array_data[0],array_data[1],array_data[2] }};
#ifdef SORT_UNIFORMS
						SetUniformv<float,3>(loc, std::move(array3));
#else
						SetUniform(loc, array3, 3);
#endif
						Py_RETURN_NONE;
					}break;
				case 4:
					{
						std::array<float,4> array4 = {{ array_data[0],array_data[1],array_data[2],array_data[3] }};
#ifdef SORT_UNIFORMS
						SetUniformv<float,4>(loc, std::move(array4));
#else
						SetUniform(loc, array4, 4);
#endif
						Py_RETURN_NONE;
					}break;
				default:
					{
						PyErr_SetString(PyExc_TypeError, "shader.setUniform4i(name, ix,iy,iz, iw): BL_Shader. invalid list size");
						return NULL;
					}break;
				}
			}
		}
	}
	return NULL;
}

KX_PYMETHODDEF_DOC( BL_Shader, setUniformiv, "setUniformiv( uniform_name, (list2 or list3 or list4) )")
{
	if (mError) {
		Py_RETURN_NONE;
	}
	const char *uniform = "";
	PyObject *listPtr =0;
	int array_data[4] = {0,0,0,0};

	if (!PyArg_ParseTuple(args, "sO:setUniformiv", &uniform, &listPtr))
		return NULL;
	
	int loc = GetUniformLocation(uniform);
	
	if (loc == -1) {
		PyErr_SetString(PyExc_TypeError, "shader.setUniformiv(...): BL_Shader, first string argument is not a valid uniform value");
		return NULL;
	}
	
	if (!PySequence_Check(listPtr)) {
		PyErr_SetString(PyExc_TypeError, "shader.setUniformiv(...): BL_Shader, second argument is not a sequence");
		return NULL;
	}
	
	unsigned int list_size = PySequence_Size(listPtr);
	
	for (unsigned int i=0; (i<list_size && i<4); i++)
	{
		PyObject *item = PySequence_GetItem(listPtr, i);
		array_data[i] = PyLong_AsLong(item);
		Py_DECREF(item);
	}
	
	if (PyErr_Occurred()) {
		PyErr_SetString(PyExc_TypeError, "shader.setUniformiv(...): BL_Shader, one or more values in the list is not an int");
		return NULL;
	}
	
	/* Sanity checks done! */
	
	switch (list_size)
	{
	case 2:
		{
			std::array<int,2> array2 = {{ array_data[0],array_data[1]}};
#ifdef SORT_UNIFORMS
			SetUniformv<int,2>(loc, std::move(array2));
#else
			SetUniform(loc, array2, 2);
#endif
			Py_RETURN_NONE;
		} break;
	case 3:
		{
			std::array<int,3> array3 = {{ array_data[0],array_data[1],array_data[2] }};
#ifdef SORT_UNIFORMS
			SetUniformv<int,3>(loc, std::move(array3));
			
#else
			SetUniform(loc, array3, 3);
#endif
			Py_RETURN_NONE;
		}break;
	case 4:
		{
			std::array<int,4> array4 = {{ array_data[0],array_data[1],array_data[2],array_data[3] }};
#ifdef SORT_UNIFORMS
			SetUniformv<int,4>(loc, std::move(array4));
			
#else
			SetUniform(loc, array4, 4);
#endif
			Py_RETURN_NONE;
		}break;
	default:
		{
			PyErr_SetString(PyExc_TypeError, "shader.setUniformiv(...): BL_Shader, second argument, invalid list size, expected an int list between 2 and 4");
			return NULL;
		}break;
	}
	
	Py_RETURN_NONE;
}


KX_PYMETHODDEF_DOC( BL_Shader, setUniformMatrix4, 
"setUniformMatrix4(uniform_name, mat-4x4, transpose(row-major=true, col-major=false)" )
{
	if (mError) {
		Py_RETURN_NONE;
	}

	std::array<float,16> matr = {{
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	}};

	const char *uniform;
	PyObject *matrix=0;
	int transp=0; // python use column major by default, so no transpose....
	
	if (!PyArg_ParseTuple(args, "sO|i:setUniformMatrix4",&uniform, &matrix,&transp))
		return NULL;

	int loc = GetUniformLocation(uniform);
	
	if (loc == -1) {
		PyErr_SetString(PyExc_TypeError, "shader.setUniformMatrix4(...): BL_Shader, first string argument is not a valid uniform value");
		return NULL;
	}
	
	MT_Matrix4x4 mat;
	
	if (!PyMatTo(matrix, mat)) {
		PyErr_SetString(PyExc_TypeError, "shader.setUniformMatrix4(...): BL_Shader, second argument cannot be converted into a 4x4 matrix");
		return NULL;
	}
	
	/* Sanity checks done! */

#ifdef SORT_UNIFORMS
	mat.getValue(matr.data());
	SetUniformv<float,16>(loc, std::move(matr), (transp!=0) );
#else
	SetUniform(loc,mat,(transp!=0));
#endif
	Py_RETURN_NONE;
}


KX_PYMETHODDEF_DOC( BL_Shader, setUniformMatrix3,
"setUniformMatrix3(uniform_name, list[3x3], transpose(row-major=true, col-major=false)" )
{
	if (mError) {
		Py_RETURN_NONE;
	}

	std::array<float,9> matr = {{
		1,0,0,
		0,1,0,
		0,0,1,
	}};

	const char *uniform;
	PyObject *matrix=0;
	int transp=0; // python use column major by default, so no transpose....
	if (!PyArg_ParseTuple(args, "sO|i:setUniformMatrix3",&uniform, &matrix,&transp))
		return NULL;
	
	int loc = GetUniformLocation(uniform);
	
	if (loc == -1) {
		PyErr_SetString(PyExc_TypeError, "shader.setUniformMatrix3(...): BL_Shader, first string argument is not a valid uniform value");
		return NULL;
	}
	
	
	MT_Matrix3x3 mat;
	
	if (!PyMatTo(matrix, mat)) {
		PyErr_SetString(PyExc_TypeError, "shader.setUniformMatrix3(...): BL_Shader, second argument cannot be converted into a 3x3 matrix");
		return NULL;
	}
	

#ifdef SORT_UNIFORMS
	mat.getValue(matr.data());
	SetUniformv<float,9>(loc, std::move(matr), (transp!=0) );
#else
	SetUniform(loc,mat,(transp!=0));
#endif
	Py_RETURN_NONE;
}

KX_PYMETHODDEF_DOC( BL_Shader, setAttrib, "setAttrib(enum)" )
{
	if (mError) {
		Py_RETURN_NONE;
	}
	
	int attr=0;
	
	if (!PyArg_ParseTuple(args, "i:setAttrib", &attr))
		return NULL;

	attr= SHD_TANGENT; /* user input is ignored for now, there is only 1 attr */

	if (mShader==0) {
		PyErr_SetString(PyExc_ValueError, "shader.setAttrib() BL_Shader, invalid shader object");
		return NULL;
	}

	mAttr= attr;
	glUseProgramObjectARB(mShader);
	glBindAttribLocationARB(mShader, mAttr, "Tangent");
	Py_RETURN_NONE;
}


KX_PYMETHODDEF_DOC( BL_Shader, setUniformDef, "setUniformDef(name, enum)" )
{
	if (mError) {
		Py_RETURN_NONE;
	}

	const char *uniform;
	int nloc=0;
	if (PyArg_ParseTuple(args, "si:setUniformDef", &uniform, &nloc))
	{
		int loc = GetUniformLocation(uniform);
		if (loc != -1)
		{
			bool defined = false;
			BL_UniformVecDef::iterator it = mPreDef.begin();
			while (it != mPreDef.end()) {
				if ((*it)->mLoc == loc) {
					defined = true;
					break;
				}
				it++;
			}
			if (defined) {
				Py_RETURN_NONE;
			}

			auto uni = std::make_shared<BL_DefUniform>();
			uni->mLoc = loc;
			uni->mType = nloc;
			uni->mFlag = 0;
			mPreDef.push_back(uni);
			Py_RETURN_NONE;
		}
	}
	return NULL;
}

#endif // WITH_PYTHON

// eof
