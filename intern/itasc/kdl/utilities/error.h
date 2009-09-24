/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:38 CET 2005  error.h 

                        error.h -  description
                           -------------------
    begin                : Mon January 10 2005
    copyright            : (C) 2005 Erwin Aertbelien
    email                : erwin.aertbelien@mech.kuleuven.ac.be
 
 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/
 
 
/*****************************************************************************
 * \file  
 *		 Defines the exception classes that can be thrown
 *  \author 
 *  	Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version 
 *		ORO_Geometry V0.2
 *
 *	\par History
 *		- $log$
 *
 *	\par Release
 *		$Id: error.h 19905 2009-04-23 13:29:54Z ben2610 $
 *		$Name:  $ 
 ****************************************************************************/
#ifndef ERROR_H_84822 // to make it unique, a random number
#define ERROR_H_84822

#include "utility.h"
#include <string>

namespace KDL {

/** 
 *  Base class for errors generated by ORO_Geometry 
 */
class Error {
public:
    /** Returns a description string describing the error.
     *  the returned pointer only garanteed to exists as long as 
     * the Error object exists.
     */
	virtual ~Error() {}
    virtual const char* Description() const {return "Unspecified Error\n";}

    virtual int GetType() const {return 0;}
};


class Error_IO : public Error {
	std::string msg;
	int typenr;
public:
	Error_IO(const std::string& _msg="Unspecified I/O Error",int typenr=0):msg(_msg) {}
    virtual const char* Description() const {return msg.c_str();}
    virtual int GetType() const {return typenr;}
};
class Error_BasicIO : public Error_IO {};
class Error_BasicIO_File : public Error_BasicIO {
public:
    virtual const char* Description() const {return "Error while reading stream";}
    virtual int GetType() const {return 1;}
};
class Error_BasicIO_Exp_Delim : public Error_BasicIO {
public:
    virtual const char* Description() const {return "Expected Delimiter not encountered";}
    virtual int GetType() const {return 2;}
};
class Error_BasicIO_Not_A_Space : public Error_BasicIO {
public:
    virtual const char* Description() const {return "Expected space,tab or newline not encountered";}
    virtual int GetType() const {return 3;}
};
class Error_BasicIO_Unexpected : public Error_BasicIO {
public:
    virtual const char* Description() const {return "Unexpected character";}
    virtual int GetType() const {return 4;}
};

class Error_BasicIO_ToBig : public Error_BasicIO {
public:
    virtual const char* Description() const {return "Word that is read out of stream is bigger than maxsize";}
    virtual int GetType() const {return 5;}
};

class Error_BasicIO_Not_Opened : public Error_BasicIO {
public:
    virtual const char* Description() const {return "File cannot be opened";}
    virtual int GetType() const {return 6;}
};
class Error_FrameIO : public Error_IO {};
class Error_Frame_Vector_Unexpected_id : public Error_FrameIO {
public:
    virtual const char* Description() const {return "Unexpected identifier, expecting a vector (explicit or ZERO)";}
    virtual int GetType() const {return 101;}
};
class Error_Frame_Frame_Unexpected_id : public Error_FrameIO {
public:
    virtual const char* Description() const {return "Unexpected identifier, expecting a Frame (explicit or DH)";}
    virtual int GetType() const {return 102;}
};
class Error_Frame_Rotation_Unexpected_id : public Error_FrameIO {
public:
    virtual const char* Description() const {return "Unexpected identifier, expecting a Rotation (explicit or EULERZYX, EULERZYZ, RPY,ROT,IDENTITY)";}
    virtual int GetType() const {return 103;}
};
class Error_ChainIO : public Error {};
class Error_Chain_Unexpected_id : public Error_ChainIO {
public:
    virtual const char* Description() const {return "Unexpected identifier, expecting TRANS or ROT";}
    virtual int GetType() const {return 201;}
};
//! Error_Redundancy indicates an error that occured during solving for redundancy.
class Error_RedundancyIO:public Error_IO  {};
class Error_Redundancy_Illegal_Resolutiontype : public Error_RedundancyIO {
public:
    virtual const char* Description() const {return "Illegal Resolutiontype is used in I/O with ResolutionTask";}
    virtual int GetType() const {return 301;}
};
class Error_Redundancy:public Error  {};
class Error_Redundancy_Unavoidable : public Error_Redundancy {
public:
    virtual const char* Description() const {return "Joint limits cannot be avoided";}
    virtual int GetType() const {return 1002;}
};
class  Error_Redundancy_Low_Manip: public Error_Redundancy {
public:
    virtual const char* Description() const {return "Manipulability is very low";}
    virtual int GetType() const {return 1003;}
};
class Error_MotionIO : public Error {};
class Error_MotionIO_Unexpected_MotProf : public Error_MotionIO {
public:
    virtual const char* Description() const { return "Wrong keyword while reading motion profile";}
    virtual int GetType() const {return 2001;}
};
class Error_MotionIO_Unexpected_Traj : public Error_MotionIO {
public:
    virtual const char* Description() const { return "Trajectory type keyword not known";}
    virtual int GetType() const {return 2002;}
};

class Error_MotionPlanning : public Error {};

class Error_MotionPlanning_Circle_ToSmall : public Error_MotionPlanning {
public:
    virtual const char* Description() const { return "Circle : radius is to small";}
    virtual int GetType() const {return 3001;}
};

class Error_MotionPlanning_Circle_No_Plane : public Error_MotionPlanning {
public:
    virtual const char* Description() const { return "Circle : Plane for motion is not properly defined";}
    virtual int GetType() const {return 3002;}
};

class Error_MotionPlanning_Incompatible: public Error_MotionPlanning {
public:
    virtual const char* Description() const { return "Acceleration of a rectangular velocityprofile cannot be used";}
    virtual int GetType() const {return 3003;}
};

class Error_MotionPlanning_Not_Feasible: public Error_MotionPlanning {
public:
    virtual const char* Description() const { return "Motion Profile with requested parameters is not feasible";}
    virtual int GetType() const {return 3004;}
};

class Error_MotionPlanning_Not_Applicable: public Error_MotionPlanning {
public:
    virtual const char* Description() const { return "Method is not applicable for this derived object";}
    virtual int GetType() const {return 3004;}
};
//! Abstract subclass of all errors that can be thrown by Adaptive_Integrator
class Error_Integrator : public Error {};

//! Error_Stepsize_Underflow is thrown if the stepsize becomes to small
class Error_Stepsize_Underflow  : public Error_Integrator {
public: 
    virtual const char* Description() const { return "Stepsize Underflow";}
    virtual int GetType() const {return 4001;}
};

//! Error_To_Many_Steps is thrown if the number of steps needed to
//! integrate to the desired accuracy becomes to big.
class Error_To_Many_Steps : public Error_Integrator {
public:
    virtual const char* Description() const { return "To many steps"; }
    virtual int GetType() const {return 4002;}
};

//! Error_Stepsize_To_Small is thrown if the stepsize becomes to small  
class Error_Stepsize_To_Small : public Error_Integrator {
public:
    virtual const char* Description() const { return "Stepsize to small"; }
    virtual int GetType() const {return 4003;}
};

class Error_Criterium : public Error {};

class Error_Criterium_Unexpected_id: public Error_Criterium {
public:
    virtual const char* Description() const { return "Unexpected identifier while reading a criterium";   }
    virtual int GetType() const {return 5001;}
};

class Error_Limits : public Error {};

class Error_Limits_Unexpected_id: public Error_Limits {
public:
    virtual const char* Description() const { return "Unexpected identifier while reading a jointlimits"; }
    virtual int GetType() const {return 6001;}
};


class Error_Not_Implemented: public Error {
public:
    virtual const char* Description() const { return "The requested object/method/function is not implemented"; }
    virtual int GetType() const {return 7000;}
};



}

#endif
