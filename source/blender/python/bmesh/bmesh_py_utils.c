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
 * The Original Code is Copyright (C) 2012 Blender Foundation.
 * All rights reserved.
 *
 * Contributor(s): Campbell Barton
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/python/bmesh/bmesh_py_api.c
 *  \ingroup pybmesh
 *
 * This file defines the 'bmesh.utils' module.
 * Utility functions for operating on 'bmesh.types'
 */

#include <Python.h>

#include "bmesh.h"

#include "bmesh_py_types.h"

#include "BLI_utildefines.h"

#include "bmesh_py_utils.h" /* own include */

PyDoc_STRVAR(bpy_bm_utils_vert_collapse_edge_doc,
".. method:: vert_collapse_edge(vert, edge)\n"
"\n"
"   Split an edge, return the newly created data.\n"
"\n"
"   :arg vert: The vert that will be collapsed.\n"
"   :type vert: :class:`bmesh.tupes.BMVert`\n"
"   :arg edge: The edge to collapse into.\n"
"   :type edge: :class:`bmesh.tupes.BMEdge`\n"
"   :return: The resulting edge from the collapse operation.\n"
"   :rtype: :class:`bmesh.tupes.BMEdge`\n"
);
static PyObject *bpy_bm_utils_vert_collapse_edge(PyObject *UNUSED(self), PyObject *args)
{
	BPy_BMEdge *py_edge;
	BPy_BMVert *py_vert;

	BMesh *bm;
	BMEdge *e_new = NULL;

	if (!PyArg_ParseTuple(args, "O!O!:vert_collapse_edge",
	                      &BPy_BMVert_Type, &py_vert,
	                      &BPy_BMEdge_Type, &py_edge))
	{
		return NULL;
	}

	BPY_BM_CHECK_OBJ(py_edge);
	BPY_BM_CHECK_OBJ(py_vert);

	/* this doubles for checking that the verts are in the same mesh */
	if (!(py_edge->e->v1 == py_vert->v ||
	      py_edge->e->v2 == py_vert->v))
	{
		PyErr_SetString(PyExc_ValueError,
		                "vert_collapse_edge(vert, edge): the vertex is not found in the edge");
		return NULL;
	}

	if (BM_vert_edge_count(py_vert->v) > 2) {
		PyErr_SetString(PyExc_ValueError,
		                "vert_collapse_edge(vert, edge): vert has more then 2 connected edges");
		return NULL;
	}

	bm = py_edge->bm;

	e_new = BM_vert_collapse_edge(bm, py_edge->e, py_vert->v);

	if (e_new) {
		return BPy_BMEdge_CreatePyObject(bm, e_new);
	}
	else {
		PyErr_SetString(PyExc_ValueError,
		                "vert_collapse_edge(vert, edge): no new edge created, internal error");
		return NULL;
	}
}


PyDoc_STRVAR(bpy_bm_utils_vert_collapse_faces_doc,
".. method:: vert_collapse_faces(vert, edge, fac, join_faces)\n"
"\n"
"   Split an edge, return the newly created data.\n"
"\n"
"   :arg vert: The vert that will be collapsed.\n"
"   :type vert: :class:`bmesh.tupes.BMVert`\n"
"   :arg edge: The edge to collapse into.\n"
"   :type edge: :class:`bmesh.tupes.BMEdge`\n"
"   :arg fac: The factor to use when merging customdata [0 - 1].\n"
"   :type fac: float\n"
"   :return: The resulting edge from the collapse operation.\n"
"   :rtype: :class:`bmesh.tupes.BMEdge`\n"
);
static PyObject *bpy_bm_utils_vert_collapse_faces(PyObject *UNUSED(self), PyObject *args)
{
	BPy_BMEdge *py_edge;
	BPy_BMVert *py_vert;

	float fac;
	int do_join_faces;

	BMesh *bm;
	BMEdge *e_new = NULL;

	if (!PyArg_ParseTuple(args, "O!O!fi:vert_collapse_faces",
	                      &BPy_BMVert_Type, &py_vert,
	                      &BPy_BMEdge_Type, &py_edge,
	                      &fac, &do_join_faces))
	{
		return NULL;
	}

	BPY_BM_CHECK_OBJ(py_edge);
	BPY_BM_CHECK_OBJ(py_vert);

	/* this doubles for checking that the verts are in the same mesh */
	if (!(py_edge->e->v1 == py_vert->v ||
	      py_edge->e->v2 == py_vert->v))
	{
		PyErr_SetString(PyExc_ValueError,
		                "vert_collapse_faces(vert, edge): the vertex is not found in the edge");
		return NULL;
	}

	if (BM_vert_edge_count(py_vert->v) > 2) {
		PyErr_SetString(PyExc_ValueError,
		                "vert_collapse_faces(vert, edge): vert has more then 2 connected edges");
		return NULL;
	}

	bm = py_edge->bm;

	e_new = BM_vert_collapse_faces(bm, py_edge->e, py_vert->v, CLAMPIS(fac, 0.0f, 1.0f), do_join_faces);

	if (e_new) {
		return BPy_BMEdge_CreatePyObject(bm, e_new);
	}
	else {
		PyErr_SetString(PyExc_ValueError,
		                "vert_collapse_edge(vert, edge): no new edge created, internal error");
		return NULL;
	}
}

PyDoc_STRVAR(bpy_bm_utils_vert_dissolve_doc,
".. method:: vert_dissolve(vert)\n"
"\n"
"   Dissolve this vertex (will be removed).\n"
"\n"
"   :arg vert: The vert to be dissolved.\n"
"   :type vert: :class:`bmesh.tupes.BMVert`\n"
"   :return: True when the vertex dissolve is successful.\n"
"   :rtype: bool\n"
);
static PyObject *bpy_bm_utils_vert_dissolve(PyObject *UNUSED(self), PyObject *args)
{
	BPy_BMVert *py_vert;

	BMesh *bm;

	if (!PyArg_ParseTuple(args, "O!:vert_dissolve",
	                      &BPy_BMVert_Type, &py_vert))
	{
		return NULL;
	}

	BPY_BM_CHECK_OBJ(py_vert);

	bm = py_vert->bm;

	return PyBool_FromLong((BM_vert_dissolve(bm, py_vert->v)));
}


PyDoc_STRVAR(bpy_bm_utils_edge_split_doc,
".. method:: edge_split(edge, vert, fac)\n"
"\n"
"   Split an edge, return the newly created data.\n"
"\n"
"   :arg edge: The edge to split.\n"
"   :type edge: :class:`bmesh.tupes.BMEdge`\n"
"   :arg vert: One of the verts on the edge, defines the split direction.\n"
"   :type vert: :class:`bmesh.tupes.BMVert`\n"
"   :arg fac: The point on the edge where the new vert will be created [0 - 1].\n"
"   :type fac: float\n"
"   :return: The newly created (edge, vert) pair.\n"
"   :rtype: tuple\n"
);
static PyObject *bpy_bm_utils_edge_split(PyObject *UNUSED(self), PyObject *args)
{
	BPy_BMEdge *py_edge;
	BPy_BMVert *py_vert;
	float fac;

	BMesh *bm;
	BMVert *v_new = NULL;
	BMEdge *e_new = NULL;

	if (!PyArg_ParseTuple(args, "O!O!f:edge_split",
	                      &BPy_BMEdge_Type, &py_edge,
	                      &BPy_BMVert_Type, &py_vert,
	                      &fac))
	{
		return NULL;
	}

	BPY_BM_CHECK_OBJ(py_edge);
	BPY_BM_CHECK_OBJ(py_vert);

	/* this doubles for checking that the verts are in the same mesh */
	if (!(py_edge->e->v1 == py_vert->v ||
	      py_edge->e->v2 == py_vert->v))
	{
		PyErr_SetString(PyExc_ValueError,
		                "edge_split(edge, vert): the vertex is not found in the edge");
		return NULL;
	}

	bm = py_edge->bm;

	v_new = BM_edge_split(bm, py_edge->e, py_vert->v, &e_new, CLAMPIS(fac, 0.0f, 1.0f));

	if (v_new && e_new) {
		PyObject *ret = PyTuple_New(2);
		PyTuple_SET_ITEM(ret, 0, BPy_BMEdge_CreatePyObject(bm, e_new));
		PyTuple_SET_ITEM(ret, 1, BPy_BMVert_CreatePyObject(bm, v_new));
		return ret;
	}
	else {
		PyErr_SetString(PyExc_ValueError,
		                "edge_split(edge, vert): couldn't split the edge, internal error");
		return NULL;
	}
}

PyDoc_STRVAR(bpy_bm_utils_face_split_doc,
".. method:: face_split(face, vert, vert_a, vert_b, edge_example)\n"
"\n"
"   Split an edge, return the newly created data.\n"
"\n"
"   :arg face: The face to cut.\n"
"   :type face: :class:`bmesh.tupes.BMFace`\n"
"   :arg vert_a: First vertex to cut in the face (face must contain the vert).\n"
"   :type vert_a: :class:`bmesh.tupes.BMVert`\n"
"   :arg vert_b: Second vertex to cut in the face (face must contain the vert).\n"
"   :type vert_b: :class:`bmesh.tupes.BMVert`\n"
"   :arg edge_example: Optional edge argument, newly created edge will copy settings from this one.\n"
"   :type edge_example: :class:`bmesh.tupes.BMEdge`\n"
);
static PyObject *bpy_bm_utils_face_split(PyObject *UNUSED(self), PyObject *args)
{
	BPy_BMFace *py_face;
	BPy_BMVert *py_vert_a;
	BPy_BMVert *py_vert_b;
	BPy_BMEdge *py_edge_example = NULL; /* optional */

	BMesh *bm;
	BMFace *f_new = NULL;
	BMLoop *l_new = NULL;

	if (!PyArg_ParseTuple(args, "O!O!|O!:face_split",
	                      &BPy_BMFace_Type, &py_face,
	                      &BPy_BMVert_Type, &py_vert_a,
	                      &BPy_BMVert_Type, &py_vert_b,
	                      &BPy_BMEdge_Type, &py_edge_example))
	{
		return NULL;
	}

	BPY_BM_CHECK_OBJ(py_face);
	BPY_BM_CHECK_OBJ(py_vert_a);
	BPY_BM_CHECK_OBJ(py_vert_b);

	if (py_edge_example) {
		BPY_BM_CHECK_OBJ(py_edge_example);
	}

	/* this doubles for checking that the verts are in the same mesh */
	if (BM_vert_in_face(py_face->f, py_vert_a->v) == FALSE ||
	    BM_vert_in_face(py_face->f, py_vert_b->v) == FALSE)
	{
		PyErr_SetString(PyExc_ValueError,
		                "face_split(...): one of the verts passed is not found in the face");
		return NULL;
	}

	if (py_vert_a->v == py_vert_b->v) {
		PyErr_SetString(PyExc_ValueError,
		                "face_split(...): vert arguments must differ");
		return NULL;
	}

	bm = py_face->bm;

	f_new = BM_face_split(bm, py_face->f,
	                      py_vert_a->v, py_vert_a->v,
	                      &l_new, py_edge_example ? py_edge_example->e : NULL);

	if (f_new && l_new) {
		PyObject *ret = PyTuple_New(2);
		PyTuple_SET_ITEM(ret, 0, BPy_BMFace_CreatePyObject(bm, f_new));
		PyTuple_SET_ITEM(ret, 1, BPy_BMLoop_CreatePyObject(bm, l_new));
		return ret;
	}
	else {
		PyErr_SetString(PyExc_ValueError,
		                "face_split(...): couldn't split the face, internal error");
		return NULL;
	}
}


static struct PyMethodDef BPy_BM_utils_methods[] = {
    {"vert_collapse_edge",  (PyCFunction)bpy_bm_utils_vert_collapse_edge,  METH_VARARGS, bpy_bm_utils_vert_collapse_edge_doc},
    {"vert_collapse_faces", (PyCFunction)bpy_bm_utils_vert_collapse_faces, METH_VARARGS, bpy_bm_utils_vert_collapse_faces_doc},
    {"vert_dissolve",       (PyCFunction)bpy_bm_utils_vert_dissolve,       METH_VARARGS, bpy_bm_utils_vert_dissolve_doc},
    {"edge_split",          (PyCFunction)bpy_bm_utils_edge_split,          METH_VARARGS, bpy_bm_utils_edge_split_doc},
    {"face_split",          (PyCFunction)bpy_bm_utils_face_split,          METH_VARARGS, bpy_bm_utils_face_split_doc},
    {NULL, NULL, 0, NULL}
};

PyDoc_STRVAR(BPy_BM_doc,
             "This module provides access to blenders bmesh data structures."
             );
static struct PyModuleDef BPy_BM_types_module_def = {
    PyModuleDef_HEAD_INIT,
    "bmesh.utils",  /* m_name */
    BPy_BM_doc,  /* m_doc */
    0,  /* m_size */
    BPy_BM_utils_methods,  /* m_methods */
    NULL,  /* m_reload */
    NULL,  /* m_traverse */
    NULL,  /* m_clear */
    NULL,  /* m_free */
};

PyObject *BPyInit_bmesh_utils(void)
{
	PyObject *submodule;

	submodule = PyModule_Create(&BPy_BM_types_module_def);

	return submodule;
}
