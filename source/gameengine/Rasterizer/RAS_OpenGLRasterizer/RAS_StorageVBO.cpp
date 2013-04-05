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

#include "RAS_StorageVBO.h"
#include "RAS_MeshObject.h"

#include "GL/glew.h"

#include <iostream>

VBO::VBO(RAS_DisplayArray *data, unsigned int indices)
{
	m_data = data;
	m_size = data->m_vertex.size();
	m_indices = indices;
	m_stride = sizeof(RAS_TexVert);
	std::clog << "Assert: 128 == " << m_stride << std::endl;
	std::clog << "Vertex size: " << m_data->m_vertex.size() << std::endl;
	std::clog << "Vertex xyz: " << m_data->m_vertex.data()->getXYZ() << std::endl;
	std::clog << "Vertex normal: " << m_data->m_vertex.data()->getNormal() << std::endl;
	std::clog << "Vertex rgba: " << m_data->m_vertex.data()->getRGBA() << std::endl;
	std::clog << "Vertex tangent: " << m_data->m_vertex.data()->getTangent() << std::endl;
	std::clog << "Vertex uv: " << m_data->m_vertex.data()->getUV(0) << std::endl;
	std::clog << "Vertex type: " << m_data->m_type << " " << m_data->TRIANGLE  << " " << m_data->QUAD << std::endl;

	//	Determine drawmode
	if (data->m_type == data->QUAD)
		m_mode = GL_QUADS;
	else if (data->m_type == data->TRIANGLE)
		m_mode = GL_TRIANGLES;
	else
		m_mode = GL_LINE;

	// Generate Buffers
	glGenBuffers(2, m_vbo.data());

	// Fill the buffers with initial data
	UpdateIndices();
	UpdateData();

// 	std::clog << "VBO ERROR" << std::endl;
// 	throw("VBO Error");
	// Establish offsets
// 	m_vertex_offset = (void*)(((RAS_TexVert*)0)->getXYZ());
// 	m_normal_offset = (void*)(((RAS_TexVert*)0)->getNormal());
// 	m_tangent_offset = (void*)(((RAS_TexVert*)0)->getTangent());
// 	m_color_offset = (void*)(((RAS_TexVert*)0)->getRGBA());;
// 	m_uv_offset = (void*)(((RAS_TexVert*)0)->getUV(0));
	m_vertex_offset = RAS_TexVert::PositionOffset();
	m_normal_offset = RAS_TexVert::NormalOffset();
	m_tangent_offset = RAS_TexVert::TangentOffset();
	m_color_offset = RAS_TexVert::RGBAOffset();
	m_uv_offset = RAS_TexVert::UVOffset();
	
	std::clog << "Vertex xyz offset: " << ((void*)(((RAS_TexVert*)0)->getXYZ())) << " " << m_vertex_offset << std::endl;
	std::clog << "Vertex normal offset: " << ((void*)(((RAS_TexVert*)0)->getNormal())) << " " << m_normal_offset << std::endl;
	std::clog << "Vertex rgba offset: " << ((void*)(((RAS_TexVert*)0)->getRGBA())) << " " << m_color_offset << std::endl;
	std::clog << "Vertex tangent offset: " << ((void*)(((RAS_TexVert*)0)->getTangent())) << " " << m_tangent_offset << std::endl;
	std::clog << "Vertex uv offset: " << ((void*)(((RAS_TexVert*)0)->getUV(0))) << " " << m_uv_offset << std::endl;
}

VBO::~VBO()
{
	glDeleteBuffers(2, m_vbo.data());
}

void VBO::UpdateData()
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, m_stride*m_size, m_data->m_vertex.data(), GL_STATIC_DRAW);
	std::clog << "Vertex data: " << m_stride*m_size << std::endl;
}

void VBO::UpdateIndices()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_data->m_index.size() * sizeof(m_data->m_index[0]), m_data->m_index.data(), GL_STATIC_DRAW);
	std::clog << "Vertex id: " << m_data->m_index.size() * sizeof(m_data->m_index[0]) << std::endl;
}

void VBO::Draw(int texco_num, RAS_IRasterizer::TexCoGen* texco, int attrib_num, RAS_IRasterizer::TexCoGen* attrib, int *attrib_layer, bool multi)
{
	int unit;

	// Bind buffers
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, m_vbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, m_vbo[1]);

	// Vertexes
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, m_stride, (GLuint*)m_vertex_offset);

	// Normals
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, m_stride, (GLuint*)m_normal_offset);

	// Colors
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, m_stride, (GLuint*)m_color_offset);

	if (multi)
	{
		for (unit = 0; unit < texco_num; ++unit)
		{
			glClientActiveTexture(GL_TEXTURE0_ARB + unit);
			switch (texco[unit]) {
				case RAS_IRasterizer::RAS_TEXCO_ORCO:
				case RAS_IRasterizer::RAS_TEXCO_GLOB:
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(3, GL_FLOAT, m_stride, (GLuint*)m_vertex_offset);
					break;
				case RAS_IRasterizer::RAS_TEXCO_UV:
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, m_stride, (GLuint*)m_uv_offset);
					break;
				case RAS_IRasterizer::RAS_TEXCO_NORM:
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(3, GL_FLOAT, m_stride, (GLuint*)m_normal_offset);
					break;
				case RAS_IRasterizer::RAS_TEXTANGENT:
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(4, GL_FLOAT, m_stride, (GLuint*)m_tangent_offset);
					break;
				default:
					break;
			}
		}
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}
	else //TexFace
	{
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, m_stride, (GLuint*)m_uv_offset);
	}

	if (GLEW_ARB_vertex_program)
	{
		for (unit = 0; unit < attrib_num; ++unit)
		{
			switch (attrib[unit]) {
				case RAS_IRasterizer::RAS_TEXCO_ORCO:
				case RAS_IRasterizer::RAS_TEXCO_GLOB:
					glVertexAttribPointerARB(unit, 3, GL_FLOAT, GL_FALSE, m_stride, (GLuint*)m_vertex_offset);
					glEnableVertexAttribArrayARB(unit);
					break;
				case RAS_IRasterizer::RAS_TEXCO_UV:
					glVertexAttribPointerARB(unit, 2, GL_FLOAT, GL_FALSE, m_stride, (GLuint*)m_uv_offset);
					glEnableVertexAttribArrayARB(unit);
					break;
				case RAS_IRasterizer::RAS_TEXCO_NORM:
					glVertexAttribPointerARB(unit, 2, GL_FLOAT, GL_FALSE, m_stride, (GLuint*)m_normal_offset);
					glEnableVertexAttribArrayARB(unit);
					break;
				case RAS_IRasterizer::RAS_TEXTANGENT:
					glVertexAttribPointerARB(unit, 4, GL_FLOAT, GL_FALSE, m_stride, (GLuint*)m_tangent_offset);
					glEnableVertexAttribArrayARB(unit);
					break;
				default:
					break;
			}
		}
	}
	
	glDrawElements(m_mode, m_indices, GL_UNSIGNED_SHORT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (GLEW_ARB_vertex_program)
	{
		for (int i = 0; i < attrib_num; ++i)
			glDisableVertexAttribArrayARB(i);
	}

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
}

RAS_StorageVBO::RAS_StorageVBO(int *texco_num, RAS_IRasterizer::TexCoGen *texco, int *attrib_num, RAS_IRasterizer::TexCoGen *attrib, int *attrib_layer):
	m_texco_num(texco_num),
	m_attrib_num(attrib_num),
	m_texco(texco),
	m_attrib(attrib),
	m_attrib_layer(attrib_layer)
{
}

RAS_StorageVBO::~RAS_StorageVBO()
{
}

bool RAS_StorageVBO::Init()
{
	return true;
}

void RAS_StorageVBO::Exit()
{
	m_vbo_lookup.clear();
}

void RAS_StorageVBO::IndexPrimitives(RAS_MeshSlot& ms)
{
	IndexPrimitivesInternal(ms, false);
}

void RAS_StorageVBO::IndexPrimitivesMulti(RAS_MeshSlot& ms)
{
	IndexPrimitivesInternal(ms, true);
}

void RAS_StorageVBO::IndexPrimitivesInternal(RAS_MeshSlot& ms, bool multi)
{
	RAS_MeshSlot::iterator it;
	VBO *vbo;

	for (ms.begin(it); !ms.end(it); ms.next(it))
	{
		vbo = m_vbo_lookup[it.array];

		if (vbo == nullptr){
			vbo = new VBO(it.array, it.totindex);
			m_vbo_lookup[it.array] = vbo;
		}

		// Update the vbo
		if (ms.m_mesh->MeshModified())
		{
			vbo->UpdateData();
		}

		vbo->Draw(*m_texco_num, m_texco, *m_attrib_num, m_attrib, m_attrib_layer, multi);
	}
}
