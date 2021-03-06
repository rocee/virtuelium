/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <materials/BeckmannBRDF.hpp>
#include <physics/BeckmannRoughnessFormula.hpp>
#include <physics/DielectricFormula.hpp>

/**
 * Constructor
 * spectrum : the normal incident reflection spectrum.
 */
BeckmannBRDF::BeckmannBRDF(const std::vector<Spectrum>& RPara, const std::vector<Spectrum>& ROrth, Real roughness)
: Material(true, false)
{
  _RPara = RPara;
  _ROrth = ROrth;
  _roughness = roughness;
}

/**
 * Compute the diffusely reemited light data and place the result into reemited.
 * localBasis : the object localBasis at the computation point.
 * surfaceCoordinate : the surface coordinate (texture coordinate) of the computation
 *   point on the object.
 * incident : the incident ray (light ray)
 * view : the view ray (from the camera or bounced)
 * incidentLight : the incident light comming from the incident ray.
 * reemitedLight : the light reemited into the view direction (result will be placed 
 *   here).
 */
void BeckmannBRDF::getDiffuseReemited(const Basis& localBasis, const Point2D& surfaceCoordinate, const LightVector& incidentLight, LightVector& reemitedLight)
{
  const Vector& incident = incidentLight.getRay().v;
  const Vector& view     = reemitedLight.getRay().v;

  //Computing the micro normal
  Vector microNormal;
  microNormal.setsum(incident, view);
  microNormal.mul(-1.0);
  microNormal.normalize();

  //Computing some cosinuses and sinuses
  Real cosLN = -localBasis.k.dot(incident);   // incident and normal
  Real cosVN = -localBasis.k.dot(view);       // view and normal 
  Real cosHN = localBasis.k.dot(microNormal); // micro normal and normal
  Real cosLH = -microNormal.dot(incident);    // incident and micro normal
  Real cosVH = -microNormal.dot(view);        // view and micro normal

  //Compute Beckmann and Shadowing&masking coeficients
  Real beckmann = BeckmannRoughnessFormula::BeckmannDistribution(cosHN, 
                                                                 _roughness);
  Real shadmask = BeckmannRoughnessFormula::BeckmannShadowMasking(cosLN, 
                                                                  cosVN, 
                                                                  cosVH, 
                                                                  cosHN);
  
  //Setting the polarisation framework
  LightVector localIncidentLight(incidentLight);
  localIncidentLight.changeIncidentPolarisationFramework(microNormal);
  localIncidentLight.flip();
  reemitedLight.changeReemitedPolarisationFramework(microNormal);

  if(cosVN <=0.001 || cosLN <=0.001)
  {
    reemitedLight.clear();
    return;
  }

  //Computing reflectances for each wavelength
  for(unsigned int i=0; i<localIncidentLight.size(); i++)
  {
    Real ROrth, RPara;
    getReflectance(cosLH, localIncidentLight[i].getIndex(), ROrth, RPara);
    reemitedLight[i].applyReflectance(localIncidentLight[i], RPara*beckmann*shadmask, ROrth*beckmann*shadmask);
  }
}

/**
 * Compute the secondary rays for diffuse reflexion and place them into the subrays
 * vector.
 * localBasis : the local base on the object at the computation point.
 * surfaceCoordinate : the surface  coordinate (texture coordinate) of the computation 
 *   point on the object.
 * view : the view ray (from the camera or bounced)
 * nbRays : the number of wanted ray. It is an indicative information.
 * subrays : the vector were the secondaries rays will be put.
 * weights : the weights corresponding to the distribution
 */
void BeckmannBRDF::getRandomDiffuseRay(const Basis& localBasis, const Point2D& surfaceCoordinate, LightVector& reemitedLight, unsigned int nbRays, std::vector<LightVector>& subrays)
{
  Vector view = reemitedLight.getRay().v;
  view.mul(-1.0);

  if(view.dot(localBasis.k)<0)
    return;

  for(unsigned int i=0; i<nbRays; i++)
  {
    Vector dir;
    Real weight;
    BeckmannRoughnessFormula::getBeckmannRandomRay(localBasis, view, _roughness, weight, dir);

    LightVector subray;
    subray.setRay(localBasis.o, dir);
    subray.initSpectralData(reemitedLight);
    subray.changeReemitedPolarisationFramework(localBasis.k);
    subray.setWeight(weight);
    subrays.push_back(subray);
  }
}

/**
 * Compute the bounce of the given photon and modify it. Use the russian 
 * roulette to return if it bounce (true) or if it's absorbed. Tel also if 
 * this bounce is specular or not.
 * localBasis : the local base on the object at the computation point.
 * surfaceCoordinate : the surface  coordinate (texture coordinate) of the 
 *   computation point on the object.
 * photon : the photon to bounce. This parametre will be modified to reflect
 *   the bounce.
 * specular : this function will set it to true if this bounce is specular
 */
bool BeckmannBRDF::bouncePhoton(const Basis& localBasis, const Point2D& surfaceCoordinate, MultispectralPhoton& photon, bool& specular)
{
  //Computing base angles
  Vector normal = localBasis.k;
  Real cosOi = - normal.dot(photon.direction);

  //Don't bounce from the interrior
  if(cosOi < 0)
    return false;

  //Computing photon absorption at the reflexion
  Real mean=0;
  Real ROrth, RPara;
  for(unsigned int i=0; i<GlobalSpectrum::nbWaveLengths(); i++)
  { 
    getReflectance(cosOi, i, ROrth, RPara);
    photon.radiance[i]   *=(ROrth+RPara)*0.5;
    mean+=photon.radiance[i];
  }

  //Reflexion
  if(rand()/(Real)RAND_MAX > mean)
    return false;

  //Normalizing photon energy
  for(unsigned int i=0; i<GlobalSpectrum::nbWaveLengths(); i++)
    photon.radiance[i] /= mean;

  //Compute the reflected direction
  BeckmannRoughnessFormula::reflect(normal, photon.direction, _roughness);

  //Done
  specular = false;
  return true;
}

/**
 * Get the reflectance in the reflection direction
 * @param cosOi : the cosinus of the angle between the normal and incident.
 * @param index : the wavelenght index to get
 * @param ROrth : the othogonal polarised computed Reflectance will be placed here
 * @param Rpara : the parallel polarised computed Reflectance will be placed here
 */
void BeckmannBRDF::getReflectance(const Real& cosOi, const int& index, Real& ROrth, Real& RPara)
{
  Real factor = 2.0*std::acos(cosOi)*_RPara.size()/M_PI;
  if(cosOi>1.0) factor=0.0;
  unsigned int iOi =(int)factor;
  factor -= iOi;

  Real Rs0, Rp0, Rs1, Rp1;
  if(iOi>=_RPara.size()-1){
    Rs0=_ROrth[_RPara.size()-1][index];
    Rp0=_RPara[_RPara.size()-1][index];
    Rs1=_ROrth[_RPara.size()-1][index];
    Rp1=_RPara[_RPara.size()-1][index];
  } else {
    Rs0=_ROrth[iOi][index];
    Rp0=_RPara[iOi][index];
    Rs1=_ROrth[iOi+1][index];
    Rp1=_RPara[iOi+1][index];    
  }

  RPara = Rp0*(1-factor) + Rp1*factor;
  ROrth = Rs0*(1-factor) + Rs1*factor;
}

void BeckmannBRDF::getDiffuseReemitedFromAmbiant(const Basis& localBasis, const Point2D& surfaceCoordinate, LightVector& reemitedLight, const Spectrum& incident)
{
  const Vector& light = localBasis.k;
  const Vector& view     = reemitedLight.getRay().v;

  //Computing the micro normal
  Vector microNormal;
  microNormal.setsum(light, view);
  microNormal.mul(-1.0);
  microNormal.normalize();

  //Computing some cosinuses and sinuses
  Real cosLN = -localBasis.k.dot(light);   // light and normal
  Real cosVN = -localBasis.k.dot(view);       // view and normal 
  Real cosHN = localBasis.k.dot(microNormal); // micro normal and normal
  Real cosLH = -microNormal.dot(light);    // light and micro normal
  Real cosVH = -microNormal.dot(view);        // view and micro normal

  //Compute Beckmann and Shadowing&masking coeficients
  Real beckmann = BeckmannRoughnessFormula::BeckmannDistribution(cosHN, 
                                                                 _roughness);
  Real shadmask = BeckmannRoughnessFormula::BeckmannShadowMasking(cosLN, 
                                                                  cosVN, 
                                                                  cosVH, 
                                                                  cosHN);

  if(cosVN <=0.001 )
  {
    reemitedLight.clear();
    return;
  }
  
  for(unsigned int wl=0; wl < reemitedLight.size(); wl++) {
    Real ROrth, RPara;
    getReflectance(cosLH, wl, ROrth, RPara);
    ROrth *= beckmann * shadmask;
    RPara *= beckmann * shadmask;
    reemitedLight[wl].setRadiance(incident[wl] /** cosVN*/ * 0.5 * (ROrth + RPara));
  }

 reemitedLight.changeReemitedPolarisationFramework(microNormal);
}
