// Copyright (c) 2007-08  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
// Author(s) : Pierre Alliez and Laurent Saboret

#ifndef CGAL_READ_PWN_POINT_SET_H
#define CGAL_READ_PWN_POINT_SET_H

#include <CGAL/value_type_traits.h>
#include <CGAL/point_set_processing_assertions.h>

#include <stdio.h>

CGAL_BEGIN_NAMESPACE


/// Read points from a .pwn file (position + normal, ASCII).
/// @return true on success.
template <typename OutputIterator> ///< OutputIterator value_type must be
                                   ///< a model of the PointWithNormal_3 concept.
bool read_pwn_point_set(const char* pFilename, OutputIterator output)
{
  // value_type_traits is a workaround as back_insert_iterator's value_type is void
  typedef typename value_type_traits<OutputIterator>::type Point_with_normal;

  typedef typename Point_with_normal::Geom_traits Geom_traits;
  typedef typename Geom_traits::Point_3 Point;
  typedef typename Geom_traits::Vector_3 Vector;

  CGAL_precondition(pFilename != NULL);

  FILE *pFile = fopen(pFilename,"rt");
  if(pFile == NULL)
  {
    std::cerr << "Error: cannot open " << pFilename;
    return false;
  }

  // scan points
  char pLine[4096];
  while(fgets(pLine,4096,pFile))
  {
    double x,y,z;
    double nx,ny,nz;
    if(sscanf(pLine,"%lg %lg %lg %lg %lg %lg",&x,&y,&z,&nx,&ny,&nz) == 6)
    {
      Point point(x,y,z);
      Vector normal(nx,ny,nz);
      *output = Point_with_normal(point,normal);
      output++;
    }
  }

  fclose(pFile);
  return true;
}


CGAL_END_NAMESPACE

#endif // CGAL_READ_PWN_POINT_SET_H