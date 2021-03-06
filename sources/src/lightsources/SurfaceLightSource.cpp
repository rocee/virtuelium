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

#include <lightsources/SurfaceLightSource.hpp>

/**
 * Constructor
 * @param spectrum : the emited spectrum
 * @param power : the power of this source (multiplier factor)
 * @param o : the origine of the emission
 * @param normal : the normal of the emission surface 
 */
SurfaceLightSource::SurfaceLightSource(const Spectrum& spectrum, const Real& power, const Point& o, const Vector& normal)
: _normal(normal), _spectrum(spectrum), _lightPower(power), _o(o)
{
  _spectrum.normalizePower();
  _normal.normalize();
  _powerFactor=_lightPower/M_PI;
}

/**
 * Place incidents light data casted form this source to the given.
 * receiver : the point where we need to have the incidents lights rays.
 * incidents : incidents light data will be placed into this vector.
 */
void SurfaceLightSource::getIncidentLight(const Point& receiver, const LightVector& reemited, std::vector<LightVector>& incidents)
{
  LightVector lightdata;

  //Building geometric part of this light data
  Vector propagation(_o, receiver);
  Real norm = propagation.norm();
  propagation.mul(1.0/norm);
  lightdata.setRay(receiver, propagation);
  lightdata.setDistance(norm);

  if(_normal.dot(propagation)<0)
    return;

  if(propagation[2]<1.0)
    lightdata.changeReemitedPolarisationFramework(Vector(0.0, 0.0, 1.0));
  else
    lightdata.changeReemitedPolarisationFramework(Vector(1.0, 0.0, 0.0));

  //Building the spectral part of this light data
  lightdata.initSpectralData(reemited);
  Real power=_normal.dot(propagation)*_powerFactor/(lightdata.getDistance()*lightdata.getDistance());
  for(unsigned int i=0; i<lightdata.size() ;i++)
  {
    lightdata[i].setRadiance(_spectrum[lightdata[i].getIndex()]*power);
  }

  //Done !
  incidents.push_back(lightdata);
}

/**
 * Return the power of this source (the sum of all wavelenght)
 */
Real SurfaceLightSource::getPower()
{
  return _lightPower;
}

/**
 * Compute the emitted light for direct source view.
 * @param localBasis : the local basis on the surface of the source.
 * @param surfaceCoordinate : the surface coordinate on the source surface.
 * @param emitted : the light data to compute.
 */
void SurfaceLightSource::getEmittedLight(const Basis& localBasis, const Point2D& surfaceCoordinate, LightVector& emitted)
{
  emitted.clear();
  emitted.changeReemitedPolarisationFramework(localBasis.k);
}

/**
 * Generate a random photon according this source spectral and spatial
 * distribution.
 * photon : the photon to generate;
 */
void SurfaceLightSource::getRandomPhoton(MultispectralPhoton& photon)
{
  //Position
  photon.position = _o;

  //Direction
  Real norm2;
  Real cosOi;
  do{
    photon.direction[0]=rand()*2.0/RAND_MAX - 1.0;
    photon.direction[1]=rand()*2.0/RAND_MAX - 1.0;
    photon.direction[2]=rand()*2.0/RAND_MAX - 1.0;
    norm2=photon.direction.square();
    photon.direction.normalize();
    cosOi = _normal.dot(photon.direction);
  }while(norm2>cosOi*cosOi || cosOi<=0);

  //Building spectral part of the photon
  Real mean=0;
  for(unsigned int i=0; i<GlobalSpectrum::nbWaveLengths(); i++)
    mean+=_spectrum[i];
  for(unsigned int i=0; i<GlobalSpectrum::nbWaveLengths(); i++)
    photon.radiance[i]=_spectrum[i]/mean;
}
