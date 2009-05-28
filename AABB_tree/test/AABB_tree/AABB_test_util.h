// Copyright (c) 2009 INRIA Sophia-Antipolis (France).
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
// $URL: $
// $Id: $
//
//
// Author(s)     : Pierre Alliez, Stephane Tayeb, Camille Wormser
//
//******************************************************************************
// File Description :
//
//******************************************************************************

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Simple_cartesian.h>

#include <CGAL/AABB_polyhedron_triangle_primitive.h>
#include <CGAL/AABB_polyhedron_segment_primitive.h>


double random_in(const double a,
                 const double b)
{
    double r = rand() / (double)RAND_MAX;
    return a + (b - a) * r;
}

template <class K>
typename K::Point_3 random_point_in(const CGAL::Bbox_3& bbox)
{
    typedef typename K::FT FT;
    FT x = (FT)random_in(bbox.xmin(),bbox.xmax());
    FT y = (FT)random_in(bbox.ymin(),bbox.ymax());
    FT z = (FT)random_in(bbox.zmin(),bbox.zmax());
    return typename K::Point_3(x,y,z);
}

template <class K>
typename K::Vector_3 random_vector()
{
    typedef typename K::FT FT;
    FT x = (FT)random_in(0.0,1.0);
    FT y = (FT)random_in(0.0,1.0);
    FT z = (FT)random_in(0.0,1.0);
    return typename K::Vector_3(x,y,z);
}


template <class Tree, class K>
void test_all_intersection_query_types(Tree& tree)
{
    std::cout << "Test all query types" << std::endl;

    typedef typename K::FT FT;
    typedef typename K::Ray_3 Ray;
    typedef typename K::Line_3 Line;
    typedef typename K::Point_3 Point;
    typedef typename K::Vector_3 Vector;
    typedef typename K::Segment_3 Segment;
    typedef typename Tree::Primitive Primitive;
    typedef typename Tree::Point_and_primitive_id Point_and_primitive_id;
    typedef typename Tree::Object_and_primitive_id Object_and_primitive_id;

    Point p((FT)-0.5, (FT)-0.5, (FT)-0.5);
    Point q((FT) 0.5, (FT) 0.5, (FT) 0.5);
    Ray ray(p,q);
    Ray line(p,q);
    Ray segment(p,q);
    bool success = false;

    // do_intersect
    success = tree.do_intersect(ray);
    success = tree.do_intersect(line);
    success = tree.do_intersect(segment);

    // number_of_intersected_primitives
    tree.number_of_intersected_primitives(ray);
    tree.number_of_intersected_primitives(line);
    tree.number_of_intersected_primitives(segment);

    // all_intersected_primitives
    std::list<typename Primitive::Id> primitives;
    tree.all_intersected_primitives(ray,std::back_inserter(primitives));
    tree.all_intersected_primitives(line,std::back_inserter(primitives));
    tree.all_intersected_primitives(segment,std::back_inserter(primitives));

    // any_intersection
    boost::optional<Object_and_primitive_id> optional_object_and_primitive;
    optional_object_and_primitive = tree.any_intersection(ray);
    optional_object_and_primitive = tree.any_intersection(line);
    optional_object_and_primitive = tree.any_intersection(segment);

    // any_intersected_primitive
    boost::optional<typename Primitive::Id> optional_primitive;
    optional_primitive = tree.any_intersected_primitive(ray);
    optional_primitive = tree.any_intersected_primitive(line);
    optional_primitive = tree.any_intersected_primitive(segment);

    // all_intersections
    std::list<Object_and_primitive_id> intersections;
    tree.all_intersections(ray,std::back_inserter(intersections));
    tree.all_intersections(line,std::back_inserter(intersections));
    tree.all_intersections(segment,std::back_inserter(intersections));
}


template <class Tree, class K>
void test_all_distance_query_types(Tree& tree)
{
    typedef typename K::FT FT;
    typedef typename K::Ray_3 Ray;
    typedef typename K::Point_3 Point;
    typedef typename K::Vector_3 Vector;
    typedef typename Tree::Primitive Primitive;
    typedef typename Tree::Point_and_primitive_id Point_and_primitive_id;

    Point query = random_point_in<K>(tree.bbox());
    Point_and_primitive_id hint = tree.any_reference_point_and_id();

    FT sqd1 = tree.squared_distance(query);
    FT sqd2 = tree.squared_distance(query,hint.first);
    if(sqd1 != sqd2)
        std::cout << "warning: different distances with and without hint";

    Point p1 = tree.closest_point(query);
    Point p2 = tree.closest_point(query,hint.first);
    if(sqd1 != sqd2)
        std::cout << "warning: different closest points with and without hint (possible, in case there are more than one)";

    Point_and_primitive_id pp1 = tree.closest_point_and_primitive(query);
    Point_and_primitive_id pp2 = tree.closest_point_and_primitive(query,hint);
    if(pp1.second != pp2.second)
        std::cout << "warning: different closest primitives with and without hint (possible, in case there are more than one)";
}


template <class Tree, class K>
void test_distance_speed(Tree& tree,
                         const double duration)
{
    typedef typename K::FT FT;
    typedef typename K::Ray_3 Ray;
    typedef typename K::Point_3 Point;
    typedef typename K::Vector_3 Vector;

    CGAL::Timer timer;
    timer.start();
    unsigned int nb = 0;
    while(timer.time() < duration)
    {
            // picks a random point in the tree bbox
            Point query = random_point_in<K>(tree.bbox());
            Point closest = tree.closest_point(query);
            nb++;
    }
    double speed = (double)nb / timer.time();
    std::cout << speed << " distance queries/s" << std::endl;
    timer.stop();
}




//-------------------------------------------------------
// Helpers
//-------------------------------------------------------
enum Primitive_type {
  SEGMENT, TRIANGLE
};

/**
 * Primitive_generator : designed to tell void test<K,Primitive>(const char* filename)
 * some information about which primitive to use.
 *
 * Must define:
 *  type Primitive
 *  type iterator
 *  iterator begin(Polyhedron&)
 *  iterator end(Polyhedron&)
 *
 * begin & end are used to build the AABB_tree.
 */
template<Primitive_type Primitive, class K, class Polyhedron>
struct Primitive_generator {};

template<class K, class Polyhedron>
struct Primitive_generator<SEGMENT, K, Polyhedron>
{
    typedef CGAL::AABB_polyhedron_segment_primitive<K,Polyhedron> Primitive;

    typedef typename Polyhedron::Edge_iterator iterator;
    iterator begin(Polyhedron& p) { return p.edges_begin(); }
    iterator end(Polyhedron& p) { return p.edges_end(); }
};

template<class K, class Polyhedron>
struct Primitive_generator<TRIANGLE, K, Polyhedron>
{
    typedef CGAL::AABB_polyhedron_triangle_primitive<K,Polyhedron> Primitive;

    typedef typename Polyhedron::Facet_iterator iterator;
    iterator begin(Polyhedron& p) { return p.facets_begin(); }
    iterator end(Polyhedron& p) { return p.facets_end(); }
};



/**
 * Declaration only, implementation should be given in .cpp file
 */
template<class K, class Tree, class Polyhedron>
void test_impl(Tree& tree, Polyhedron& p, const double duration);


/**
 * Generic test method. Build AABB_tree and call test_impl()
 */
template <class K, Primitive_type Primitive>
void test(const char *filename,
          const double duration)
{
    typedef CGAL::Polyhedron_3<K> Polyhedron;
    typedef Primitive_generator<Primitive,K,Polyhedron> Pr_generator;
    typedef typename Pr_generator::Primitive Pr;
    typedef CGAL::AABB_traits<K, Pr> Traits;
    typedef CGAL::AABB_tree<Traits> Tree;

    Polyhedron polyhedron;
    std::ifstream ifs(filename);
    ifs >> polyhedron;

    // constructs AABB tree and internal search KD-tree with
    // the points of the polyhedron
    Tree tree(Pr_generator().begin(polyhedron),Pr_generator().end(polyhedron));
    tree.accelerate_distance_queries(polyhedron.points_begin(),polyhedron.points_end());

    // call all tests
    test_impl<K,Tree,Polyhedron>(tree,polyhedron,duration);
}


/**
 * Generic test_kernel method. call test<K> for various kernel K.
 */
template<Primitive_type Primitive>
void test_kernels(const char *filename,
                  const double duration)
{
    std::cout << std::endl;
    std::cout << "Polyhedron " << filename << std::endl;
    std::cout << "============================" << std::endl;

    std::cout << std::endl;
    std::cout << "Simple cartesian float kernel" << std::endl;
    test<CGAL::Simple_cartesian<float>,Primitive>(filename,duration);

    std::cout << std::endl;
    std::cout << "Cartesian float kernel" << std::endl;
    test<CGAL::Cartesian<float>,Primitive>(filename,duration);

    std::cout << std::endl;
    std::cout << "Simple cartesian double kernel" << std::endl;
    test<CGAL::Simple_cartesian<double>,Primitive>(filename,duration);

    std::cout << std::endl;
    std::cout << "Cartesian double kernel" << std::endl;
    test<CGAL::Cartesian<double>,Primitive>(filename,duration);

    std::cout << std::endl;
    std::cout << "Epic kernel" << std::endl;
    test<CGAL::Exact_predicates_inexact_constructions_kernel,Primitive>(filename,duration);
}