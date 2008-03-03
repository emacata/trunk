// 2008 © Václav Šmilauer <eudoxos@arcig.cz> 
#pragma once
#include<yade/extra/Shop.hpp>

#include<yade/core/InteractionSolver.hpp>
#include<yade/core/FileGenerator.hpp>
#include<yade/pkg-common/RigidBodyParameters.hpp>
#include<yade/pkg-common/Force.hpp>
#include<yade/pkg-common/Momentum.hpp>
#include<yade/pkg-common/InteractionPhysicsEngineUnit.hpp>
#include<yade/pkg-dem/SpheresContactGeometry.hpp>
#include<yade/pkg-common/GLDrawInteractionPhysicsFunctor.hpp>


/*! @brief Convert macroscopic properties to BrefcomContact with corresponding parameters.
 *
 * Most of the model is taken from MacroMicroElasticRelationships.
 *
 * @todo alpha, beta, gamma constants should be set to better values that Laurent presented in Nantes.
 *
 * */
class BrefcomMakeContact: public InteractionPhysicsEngineUnit{
	private:
	public:
		/* "constants" for macro-micro */
		Real alpha, beta, gamma;
		/* nonelastic material parameters */
		/* alternatively (and more cleanly), we would have subclass of ElasticBodyParameters,
		 * which would define just those in addition to the elastic ones.
		 * This might be done later, for now hardcode that here. Note that sigmaC and Gf are not used at all now. */
		/* uniaxial traction resistance, uniaxial compression resistance, fracture energy, elastic/softening modulus ratio */
		Real sigmaT, sigmaC, Gf, zeta;

		BrefcomMakeContact(){ alpha=3.7; beta=2.198; gamma=3.79; sigmaT=3e6; sigmaC=30e6; }
		virtual void go(const shared_ptr<PhysicalParameters>& pp1, const shared_ptr<PhysicalParameters>& pp2, const shared_ptr<Interaction>& interaction);
		virtual void registerAttributes(){
			InteractionPhysicsEngineUnit::registerAttributes();
			REGISTER_ATTRIBUTE(alpha);
			REGISTER_ATTRIBUTE(beta);
			REGISTER_ATTRIBUTE(gamma);
			/* REGISTER_ATTRIBUTE(sigmaT);
			REGISTER_ATTRIBUTE(sigmaC);
			REGISTER_ATTRIBUTE(Gf);
			REGISTER_ATTRIBUTE(zeta); */
		}

		FUNCTOR2D(BodyMacroParameters,BodyMacroParameters);
		REGISTER_CLASS_NAME(BrefcomMakeContact);
		REGISTER_BASE_CLASS_NAME(InteractionPhysicsEngineUnit);
		DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(BrefcomMakeContact,false);

/*! @brief representation of a single interaction of the brefcom type: storage for relevant parameters.
 *
 * Evolution of the contact is governed by BrefcomLaw:
 * that includes damage effects and chages of parameters inside BrefcomContact.
 *
 * @todo Ks is currently unused, but it is calculated from Kn in BrefcomMakeContact using nu12 and alpha;
 * 	if Kn changes due to damage, how do we adjust Ks? BrefcomMakeContact could do that at every iter? Messy?
 *
 * @todo frictionAngle is currently unused
 */
class BrefcomContact: public InteractionPhysics {
	private:
	public:
		/*! Fundamental parameters (constants) */
		Real Kn, Kt, frictionAngle, undamagedCohesion, equilibriumDist, crossSection, epsCracking, epsFracture, xiShear;
		/*! Up to now maximum normal strain */
		Real kappaD;
		/* prevNormal is oriented A→B (as in SpheresContactGeometry); */
		Vector3r prevNormal;
		/* previous tangential (shear) strain */
		Vector3r epsT;
		/* if not cohesive, equivalent to damage being always 1 */
		bool isCohesive; // FIXME: not yet implemented

		/* auxiliary variable for visualization, always recalculated by BrefcomLaw */
		Real omega;

		BrefcomContact(): InteractionPhysics(),Kn(0), Kt(0), frictionAngle(0), undamagedCohesion(0), equilibriumDist(0), crossSection(0), xiShear(0) { createIndex(); epsT=Vector3r::ZERO; kappaD=0; isCohesive=false; }
		BrefcomContact(Real _Kn, Real _Ks, Real _frictionAngle, Real _undamagedCohesion, Real _equilibriumDist, Real _crossSection, Real _epsCracking, Real _epsFracture, Real _xiShear=.3): InteractionPhysics(), Kn(_Kn), Kt(_Ks), frictionAngle(_frictionAngle), undamagedCohesion(_undamagedCohesion), equilibriumDist(_equilibriumDist), crossSection(_crossSection), epsCracking(_epsCracking), epsFracture(_epsFracture), xiShear(_xiShear) { epsT=Vector3r::ZERO; kappaD=0; isCohesive=false; }


		void registerAttributes(){
			InteractionPhysics::registerAttributes();
			REGISTER_ATTRIBUTE(Kn);
			REGISTER_ATTRIBUTE(Kt);
			REGISTER_ATTRIBUTE(frictionAngle);
			REGISTER_ATTRIBUTE(undamagedCohesion);
			REGISTER_ATTRIBUTE(equilibriumDist);
			REGISTER_ATTRIBUTE(crossSection);
			REGISTER_ATTRIBUTE(epsCracking);
			REGISTER_ATTRIBUTE(epsFracture);
			REGISTER_ATTRIBUTE(xiShear);

			REGISTER_ATTRIBUTE(kappaD);
			REGISTER_ATTRIBUTE(prevNormal);
			REGISTER_ATTRIBUTE(epsT);

			REGISTER_ATTRIBUTE(isCohesive);
		};

	REGISTER_CLASS_NAME(BrefcomContact);
	REGISTER_BASE_CLASS_NAME(InteractionPhysics);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(BrefcomContact,false);

#define BREFREC(a) {recValues.push_back(a); recLabels.push_back(#a);}
#define BREFREC2(a,b) {recValues.push_back(a); recLabels.push_back(b);}
class BrefcomLaw: public InteractionSolver{
	private:
		// cache class indices to avoid lookup at every iteration
		int MomentumClassIndex, ForceClassIndex; // if possible make it "const int" and move init to initializers
		//! apply effects of normal displacements
		void calculateNormalForce();
		//! apply effects of shearing
		void calculateShearForce();
		//! apply effects of mutual particle rotation
		void rotationEffects();
		//! adjust forces so that they are not outside resistance envelope and update damage parameter if necessary
		void envelopeAndDamage();
		//! plly calculated force on particles
		void applyForces();
		/* storage variables, to avoid passing them around on the stack.
		 *
		 * The orientation is conventionally as forces should be applied on the SECOND element in the interaction.
		 * That is, attractive force will be oriented as applied to the first element.
		 * This is CONTRARY to ElasticCohesiveLaw !!! */
		Vector3r Fs; Real Fn;
		/* temporaries initialized in the loop over interactions; above methods use these. */
		body_id_t id1, id2;
		shared_ptr<Body> body1, body2;
		shared_ptr<BrefcomContact> BC;
		shared_ptr<SpheresContactGeometry> contGeom;
		shared_ptr<RigidBodyParameters> rbp1, rbp2;
		MetaBody* rootBody;
		// recording  values
		ofstream recStream;
		vector<Real> recValues;
		vector<string> recLabels;
		void applyForce(const Vector3r);
		Real funcH(Real);
		Real funcG(Real);
	public:
		BrefcomLaw() {
			/* cache indices for Force and Momentum */
			ForceClassIndex=shared_ptr<PhysicalAction>(new Force())->getClassIndex();
			MomentumClassIndex=shared_ptr<PhysicalAction>(new Momentum())->getClassIndex();
			//TRVAR2(ForceClassIndex,MomentumClassIndex);
		};
		void action(Body* body);
	protected: 
		void registerAttributes(){InteractionSolver::registerAttributes();};
		void postProcessAttributes(bool deserializing){ if(deserializing) recStream.open("/tmp/breflaw.data"); }

	REGISTER_CLASS_NAME(BrefcomLaw);
	REGISTER_BASE_CLASS_NAME(InteractionSolver);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(BrefcomLaw,false);

class GLDrawBrefcomContact: public GLDrawInteractionPhysicsFunctor {
	public: virtual void go(const shared_ptr<InteractionPhysics>&,const shared_ptr<Interaction>&,const shared_ptr<Body>&,const shared_ptr<Body>&,bool wireFrame);
	virtual ~GLDrawBrefcomContact() {};
	RENDERS(BrefcomContact);
	REGISTER_CLASS_NAME(GLDrawBrefcomContact);
	REGISTER_BASE_CLASS_NAME(GLDrawInteractionPhysicsFunctor);
};
REGISTER_SERIALIZABLE(GLDrawBrefcomContact,false);


 

