/*
 *  Copyright 2013 Remi "Programmix" Cerise
 *
 *  This file is part of Virtuelium.
 *
 *  Virtuelium is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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
#include <io/MathParser.hpp>
//!
//! @file MathParser.cpp
//! @author Remi "Programmix" Cerise
//! @version 5.0.0
//! @date 2013
//! @details This file implements the classes declared in MathParser.hpp 
//!  @arg MathParser
//! @todo 
//! @remarks 
//!
#include <fstream>
#include <exceptions/Exception.hpp>
////////////////////////////////////////////////////////////////////////////////
Real* MathParser::loadMatrix(unsigned int width, unsigned int height, 
                             std::string filename) {
  //Open the file
  std::fstream input(filename.c_str(), std::fstream::in);
  if(!input.is_open()) {
    throw Exception("(MathParser::loadMatrix) Le fichier " + filename + " n'a pas pu être ouvert.");  
  }

  //Load the matrix
  Real* matrix = new Real[width * height];
  char trash[260];
  int i = 0;
  while (input.good()) {
    // comment
    if (input.peek() == '#') {
      input.getline(trash, 260);
    // data
    } else {
      input >> matrix[i];
      i++;
    }
  }
  // check
  if (i < width * height) {
    if (matrix != NULL) {
      delete[] matrix;
      matrix = NULL;
    }
    throw Exception("(MathParser::loadMatrix) Le fichier " + filename + " est \
incorrect. Les commentaires doivent commencer en début de ligne.");        
  }

  //Close the file and return the matrix
  input.close();
  return matrix;
}
////////////////////////////////////////////////////////////////////////////////
