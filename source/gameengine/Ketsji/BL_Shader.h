
/** \file BL_Shader.h
 *  \ingroup ketsji
 */

#ifndef __BL_SHADER_H__
#define __BL_SHADER_H__

#include "PyObjectPlus.h"
#include "BL_Material.h"
#include "BL_Texture.h"
// --
#include "MT_Matrix4x4.h"
#include "MT_Matrix3x3.h"
#include "MT_Tuple2.h"
#include "MT_Tuple3.h"
#include "MT_Tuple4.h"

#include <memory>
#include <array>
#include <unordered_map>

const int SHADER_ATTRIBMAX = 1;

/**
 * BL_Sampler
 *  Sampler access 
 */
class BL_Sampler
{
public:
	BL_Sampler():
		mLoc(-1)
	{
	}
	int				mLoc;		// Sampler location
	
#ifdef WITH_CXX_GUARDEDALLOC
	MEM_CXX_CLASS_ALLOC_FUNCS("GE:BL_Sampler")
#endif
};

class BL_UniformBase{
public:
	virtual ~BL_UniformBase(){};

	virtual int GetLocation() = 0;
	virtual void Apply(class BL_Shader *shader) = 0;
};

/**
 * BL_Uniform
 *  uniform storage 
 */
template<typename data_t, unsigned int size>
class BL_Uniform : public BL_UniformBase
{
private:
	int							mLoc;		// Uniform location
	std::array<data_t, size>	mData;		// Memory allocated for variable
	bool						mDirty;		// Caching variable  
	bool						mTranspose; // Transpose matrices
public:
	BL_Uniform();
	~BL_Uniform();
	
	void Apply(class BL_Shader *shader);
	void Apply();
	void SetData(std::array<data_t, size>&& data, int location, bool transpose);
	int GetLocation()	{ return mLoc; }
	
#ifdef WITH_CXX_GUARDEDALLOC
	MEM_CXX_CLASS_ALLOC_FUNCS("GE:BL_Uniform")
#endif
};

/**
 * BL_DefUniform
 * pre defined uniform storage 
 */
class BL_DefUniform
{
public:
	BL_DefUniform() :
		mType(0),
		mLoc(0),
		mFlag(0)
	{
	}
	int				mType;
	int				mLoc;
	unsigned int	mFlag;
	
	
#ifdef WITH_CXX_GUARDEDALLOC
	MEM_CXX_CLASS_ALLOC_FUNCS("GE:BL_DefUniform")
#endif
};

class BL_ShaderManager{
private:
	static std::shared_ptr<BL_ShaderManager> m_shaderManager;
	std::unordered_map<std::string, std::shared_ptr<BL_Shader>> m_shaderLibrary;
	unsigned long m_nextShaderIndex;
	
public:
	static std::shared_ptr<BL_ShaderManager> Instance(){return m_shaderManager;};
	unsigned long NextShaderIndex(){ return m_nextShaderIndex++;};
	std::shared_ptr<BL_Shader> AddShader( string shaderName );
	std::shared_ptr<BL_Shader> AddShader();
	void RemoveShader( shared_ptr<BL_Shader>&& shade );
	std::shared_ptr<BL_Shader> GetShader(string shaderName);
	unsigned long numAvailableShader();
	std::string availableShader();
};


/**
 * BL_Shader
 *  shader access
 */
class BL_Shader : public PyObjectPlus
{
	Py_Header
private:
	typedef std::vector<std::shared_ptr<BL_UniformBase>>	BL_UniformVec;
	typedef std::vector<std::shared_ptr<BL_DefUniform>>	BL_UniformVecDef;

	std::string		mName;
	unsigned int	mShader;			// Shader object 
	int				mPass;				// 1.. unused
	bool			mOk;				// Valid and ok
	bool			mUse;				// ...
//BL_Sampler		mSampler[MAXTEX];	// Number of samplers
	int				mAttr;				// Tangent attribute
	std::string		mVertProg;			// Vertex program string
	std::string		mFragProg;			// Fragment program string
	bool			mError;				// ...
	bool			mDirty;				// 

	// Compiles and links the shader
	bool LinkProgram();
	bool OnProgramError( unsigned int tmpVert = 0, unsigned int tmpFrag = 0, unsigned int tmpProg = 0 );

	// Stored uniform variables
	BL_UniformVec		mUniforms;
	BL_UniformVecDef	mPreDef;

	// search by location
	std::shared_ptr<BL_UniformBase>	FindUniform(const int location);
	// clears uniform data
	void			ClearUniforms();

public:
	BL_Shader();
	virtual ~BL_Shader();

	// Unused for now tangent is set as 
	// tex coords
	enum AttribTypes {
		SHD_TANGENT =1
	};

	enum GenType {
		MODELVIEWMATRIX,
		MODELVIEWMATRIX_TRANSPOSE,
		MODELVIEWMATRIX_INVERSE,
		MODELVIEWMATRIX_INVERSETRANSPOSE,
	
		// Model matrix
		MODELMATRIX,
		MODELMATRIX_TRANSPOSE,
		MODELMATRIX_INVERSE,
		MODELMATRIX_INVERSETRANSPOSE,
	
		// View Matrix
		VIEWMATRIX,
		VIEWMATRIX_TRANSPOSE,
		VIEWMATRIX_INVERSE,
		VIEWMATRIX_INVERSETRANSPOSE,

		// Current camera position 
		CAM_POS,

		// RAS timer
		CONSTANT_TIMER
	};

	string GetVertPtr();
	string GetFragPtr();
	void SetVertPtr( string vert );
	void SetFragPtr( string frag );
	
	// ---
	int getNumPass()	{return mPass;}
	bool GetError()		{return mError;}
	// ---
	//const BL_Sampler*	GetSampler(int i);
	void				SetSampler(int loc, int unit);

	bool				Ok()const;
	unsigned int		GetProg();
	void				SetProg(bool enable);
	int					GetAttribute() { return mAttr; }

	std::string GetName();
	void SetName(std::string name);

	// -- 
	// Apply methods : sets colected uniforms
	void ApplyShader();
	void UnloadShader();

	// Update predefined uniforms each render call
	void Update(const class RAS_MeshSlot & ms, class RAS_IRasterizer* rasty);

	//// Set sampler units (copied)
	//void InitializeSampler(int unit, BL_Texture* texture );


	template<typename data_t, int size>
	void SetUniformv( int location, std::array< data_t, size >&& param, bool transpose = false );

	int GetAttribLocation(const char *name);
	void BindAttribute(const char *attr, int loc);
	int GetUniformLocation(const char *name);

	void SetUniform(int uniform, const MT_Tuple2& vec);
	void SetUniform(int uniform, const MT_Tuple3& vec);
	void SetUniform(int uniform, const MT_Tuple4& vec);
	void SetUniform(int uniform, const MT_Matrix4x4& vec, bool transpose=false);
	void SetUniform(int uniform, const MT_Matrix3x3& vec, bool transpose=false);
	void SetUniform(int uniform, const float& val);
	void SetUniform(int uniform, const float* val, int len);
	void SetUniform(int uniform, const int* val, int len);
	void SetUniform(int uniform, const unsigned int& val);
	void SetUniform(int uniform, const int val);

	// Python interface
#ifdef WITH_PYTHON
	virtual PyObject *py_repr(void) { return PyUnicode_FromFormat("BL_Shader\n\tvertex shader:%s\n\n\tfragment shader%s\n\n", mVertProg.c_str(), mFragProg.c_str()); }

	// -----------------------------------
	KX_PYMETHOD_DOC(BL_Shader, setSource);
	KX_PYMETHOD_DOC(BL_Shader, delSource);
	KX_PYMETHOD_DOC(BL_Shader, hasSource);
	KX_PYMETHOD_DOC(BL_Shader, getVertexProg);
	KX_PYMETHOD_DOC(BL_Shader, getFragmentProg);
	KX_PYMETHOD_DOC(BL_Shader, setNumberOfPasses);
	KX_PYMETHOD_DOC(BL_Shader, isValid);
	KX_PYMETHOD_DOC(BL_Shader, validate);
	KX_PYMETHOD_DOC(BL_Shader, getName);
	KX_PYMETHOD_DOC(BL_Shader, numAvailableShader);
	KX_PYMETHOD_DOC(BL_Shader, listAvailableShader);
// 	KX_PYMETHOD_DOC(BL_Shader, setName);

	// -----------------------------------
	KX_PYMETHOD_DOC(BL_Shader, setUniform4f);
	KX_PYMETHOD_DOC(BL_Shader, setUniform3f);
	KX_PYMETHOD_DOC(BL_Shader, setUniform2f);
	KX_PYMETHOD_DOC(BL_Shader, setUniform1f);
	KX_PYMETHOD_DOC(BL_Shader, setUniform4i);
	KX_PYMETHOD_DOC(BL_Shader, setUniform3i);
	KX_PYMETHOD_DOC(BL_Shader, setUniform2i);
	KX_PYMETHOD_DOC(BL_Shader, setUniform1i);
	KX_PYMETHOD_DOC(BL_Shader, setUniformfv);
	KX_PYMETHOD_DOC(BL_Shader, setUniformiv);
	KX_PYMETHOD_DOC(BL_Shader, setUniformMatrix4);
	KX_PYMETHOD_DOC(BL_Shader, setUniformMatrix3);
	KX_PYMETHOD_DOC(BL_Shader, setUniformDef);
	KX_PYMETHOD_DOC(BL_Shader, setAttrib);
	KX_PYMETHOD_DOC(BL_Shader, setSampler);
#endif
};

#endif /* __BL_SHADER_H__ */
