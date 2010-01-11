/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/pkg-common/InteractionGeometryFunctor.hpp>

class Ig2_Sphere_Sphere_ScGeom : public InteractionGeometryFunctor
{
	public :
		virtual bool go(const shared_ptr<Shape>& cm1, const shared_ptr<Shape>& cm2, const State& state1, const State& se32, const Vector3r& shift2, const bool& force, const shared_ptr<Interaction>& c);
		virtual bool goReverse(	const shared_ptr<Shape>& cm1, const shared_ptr<Shape>& cm2, const State& state1, const State& se32, const Vector3r& shift2, const bool& force, const shared_ptr<Interaction>& c);
					
		Ig2_Sphere_Sphere_ScGeom();		
		
		/*! enlarge both radii by this factor (if >1), to permit creation of distant interactions.
		 *
		 * InteractionGeometry will be computed when interactionDetectionFactor*(rad1+rad2) > distance.
		 *
		 * @note This parameter is functionally coupled with InteractinSphere2AABB::aabbEnlargeFactor,
		 * which will create larger bounding boxes and should be of the same value. */
		double interactionDetectionFactor;

	REGISTER_CLASS_AND_BASE(Ig2_Sphere_Sphere_ScGeom,InteractionGeometryFunctor);
	REGISTER_ATTRIBUTES(InteractionGeometryFunctor,(interactionDetectionFactor));
	FUNCTOR2D(Sphere,Sphere);
	// needed for the dispatcher, even if it is symmetric
	DEFINE_FUNCTOR_ORDER_2D(Sphere,Sphere);
};
REGISTER_SERIALIZABLE(Ig2_Sphere_Sphere_ScGeom);
