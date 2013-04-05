/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file RAS_TexVert.h
 *  \ingroup bgerast
 */

#ifndef __RAS_TEXVERT_H__
#define __RAS_TEXVERT_H__


#include "MT_Point3.h"
#include "MT_Point2.h"
#include "MT_Transform.h"

#ifdef WITH_CXX_GUARDEDALLOC
#include "MEM_guardedalloc.h"
#endif

static MT_Point3 g_pt3;
static MT_Point2 g_pt2;

#include "GL/glew.h"

class RAS_TexVert
{
	
	GLfloat			m_localxyz[3];	// 3*4 = 12
	GLfloat			m_uvs[8][2];	// 8*2*4=64		//8 = MAX_UNIT
	GLuint			m_rgba;			//        4
	GLfloat			m_tangent[4];   // 4*4 = 16
	GLfloat			m_normal[3];	// 3*4 = 12
	GLshort			m_flag;			//        2
	GLshort			m_softBodyIndex;		//2
	GLuint			m_unit;			//		  4
	GLuint			m_origindex;		//    4
	GLchar			m_padding[8];		//    8
									//---------
									//      128
	// 32 bytes alignment improves performance on ATI cards.

public:
	enum {
		FLAT = 1,
		MAX_UNIT = 8
	};
	template<typename T> T PositionOffset(){return reinterpret_cast<T>(0);};
	template<typename T> T UVOffset(){return reinterpret_cast<T>(sizeof(m_localxyz));};
	template<typename T> T RGBAOffset(){return reinterpret_cast<T>(sizeof(m_uvs) + UVOffset<size_t>());};
	template<typename T> T TangentOffset(){return reinterpret_cast<T>(sizeof(m_rgba) + RGBAOffset<size_t>());};
	template<typename T> T NormalOffset(){return reinterpret_cast<T>(sizeof(m_tangent) + TangentOffset<size_t>());};

	GLshort getFlag() const;
	GLuint getUnit() const;
	
	RAS_TexVert()// :m_xyz(0,0,0),m_uv(0,0),m_rgba(0)
	{}
	RAS_TexVert(const MT_Point3& xyz,
				const MT_Point2 uvs[MAX_UNIT],
				const MT_Vector4& tangent,
				const unsigned int rgba,
				const MT_Vector3& normal,
				const bool flat,
				const unsigned int origindex);
	~RAS_TexVert() {};

	const GLfloat* getUV (int unit) const {
		return m_uvs[unit];
	};

	const GLfloat* getXYZ() const { 
		return m_localxyz;
	};
	
	const GLfloat* getNormal() const {
		return m_normal;
	}
	
	GLshort getSoftBodyIndex() const
	{
		return m_softBodyIndex;
	}
	
	void	setSoftBodyIndex(short int sbIndex)
	{
		m_softBodyIndex = sbIndex;
	}

	const GLfloat* getTangent() const {
		return m_tangent;
	}

	const GLuint* getRGBA() const {
		return &m_rgba;
	}

	GLuint getOrigIndex() const {
		return m_origindex;
	}

	void				SetXYZ(const MT_Point3& xyz);
	void				SetXYZ(const float xyz[3]);
	void				SetUV(int index, const MT_Point2& uv);
	void				SetUV(int index, const float uv[2]);

	void				SetRGBA(const unsigned int rgba);
	void				SetNormal(const MT_Vector3& normal);
	void				SetTangent(const MT_Vector3& tangent);
	void				SetFlag(const short flag);
	void				SetUnit(const unsigned u);
	
	void				SetRGBA(const MT_Vector4& rgba);
	const MT_Point3&	xyz();

	void				Transform(const class MT_Matrix4x4& mat,
	                              const class MT_Matrix4x4& nmat);
	void				TransformUV(int index, const MT_Matrix4x4& mat);

	// compare two vertices, to test if they can be shared, used for
	// splitting up based on uv's, colors, etc
	bool				closeTo(const RAS_TexVert* other);


#ifdef WITH_CXX_GUARDEDALLOC
	MEM_CXX_CLASS_ALLOC_FUNCS("GE:RAS_TexVert")
#endif
};

#endif  /* __RAS_TEXVERT_H__ */
