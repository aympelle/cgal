// poisson_reconstruction.cpp

//----------------------------------------------------------
// Poisson Delaunay Reconstruction method.
// Read a point set or a mesh's set of vertices, reconstruct a surface using Poisson,
// and save the surface.
// Output format is .off.
//----------------------------------------------------------
// poisson_reconstruction file_in file_out [options]

// CGAL
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Timer.h>
#include <CGAL/Memory_sizer.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Implicit_surface_3.h>
#define CGAL_C2T3_USE_FILE_WRITER_OFF
#include <CGAL/IO/Complex_2_in_triangulation_3_file_writer.h>

// This package
#include <CGAL/Poisson_reconstruction_function.h>
#include <CGAL/Point_with_normal_3.h>
#include <CGAL/IO/read_xyz_point_set.h>
#include <CGAL/IO/read_pwn_point_set.h>

#include "enriched_polyhedron.h"

#include <deque>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cassert>


// ----------------------------------------------------------------------------
// Types
// ----------------------------------------------------------------------------

// kernel
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;

// Simple geometric types
typedef Kernel::FT FT;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;
typedef CGAL::Point_with_normal_3<Kernel> Point_with_normal;
typedef Kernel::Sphere_3 Sphere;
typedef std::deque<Point_with_normal> PointList;

// Poisson's Delaunay triangulation 3 and implicit function
typedef CGAL::Reconstruction_triangulation_3<Kernel> Dt3;
typedef CGAL::Poisson_reconstruction_function<Kernel, Dt3> Poisson_reconstruction_function;

// Surface mesher
typedef CGAL::Surface_mesh_default_triangulation_3 STr;
typedef CGAL::Surface_mesh_complex_2_in_triangulation_3<STr> C2t3;
typedef CGAL::Implicit_surface_3<Kernel, Poisson_reconstruction_function> Surface_3;


// ----------------------------------------------------------------------------
// main()
// ----------------------------------------------------------------------------

int main(int argc, char * argv[])
{
    std::cerr << "Poisson Delaunay Reconstruction method" << std::endl;

    //***************************************
    // decode parameters
    //***************************************

    // usage
    if (argc-1 < 2)
    {
      std::cerr << "Read a point set or a mesh's set of vertices, reconstruct a surface using Poisson,\n";
      std::cerr << "and save the surface.\n";
      std::cerr << "\n";
      std::cerr << "Usage: " << argv[0] << " file_in file_out [options]\n";
      std::cerr << "Input file formats are .off (mesh) and .xyz or .pwn (point set).\n";
      std::cerr << "Output file format is .off.\n";
      std::cerr << "Options:\n";
      std::cerr << "  -sm_radius <float>     Radius upper bound (default=0.1 * point set radius)\n";
      std::cerr << "  -sm_distance <float>   Distance upper bound (default=0.002 * point set radius)\n";
      return EXIT_FAILURE;
    }

    // Poisson options
    FT sm_angle_poisson = 20.0; // Theorical guaranty if angle >= 30, but slower
    FT sm_radius_poisson = 0.1; // Upper bound of Delaunay balls radii. 0.1 is fine (LR).
    FT sm_error_bound_poisson = 1e-3; // Default value 1e-3 is fine.
    FT sm_distance_poisson = 0.002; // Upper bound of distance to surface (Poisson). 0.004 = fast, 0.002 = smooth.

    // decode parameters
    std::string input_filename  = argv[1];
    std::string output_filename = argv[2];
    for (int i=3; i+1<argc ; ++i)
    {
      if (std::string(argv[i])=="-sm_radius")
        sm_radius_poisson = atof(argv[++i]);
      else if (std::string(argv[i])=="-sm_distance")
        sm_distance_poisson = atof(argv[++i]);
      else
        std::cerr << "invalid option " << argv[i] << "\n";
    }

    CGAL::Timer task_timer; task_timer.start();

    //***************************************
    // Load mesh/point set
    //***************************************

    PointList points;

    std::string extension = input_filename.substr(input_filename.find_last_of('.'));
    if (extension == ".off" || extension == ".OFF")
    {
      // Read the mesh file in a polyhedron
      std::ifstream stream(input_filename.c_str());
      typedef Enriched_polyhedron<Kernel,Enriched_items> Polyhedron;
      Polyhedron input_mesh;
      CGAL::scan_OFF(stream, input_mesh, true /* verbose */);
      if(!stream || !input_mesh.is_valid() || input_mesh.empty())
      {
        std::cerr << "Error: cannot read file " << input_filename << std::endl;
        return EXIT_FAILURE;
      }

      // Compute vertices' normals from connectivity
      input_mesh.compute_normals();

      // Convert vertices and normals to PointList
      Polyhedron::Vertex_iterator v;
      for (v = input_mesh.vertices_begin(); v != input_mesh.vertices_end(); v++)
      {
        const Point& p = v->point();
        const Vector& n = v->normal();
        points.push_back(Point_with_normal(p,n));
      }
    }
    else if (extension == ".xyz" || extension == ".XYZ")
    {
      // Read the point set file in points[]
      if(!CGAL::read_xyz_point_set(input_filename.c_str(),
                                   std::back_inserter(points)))
      {
        std::cerr << "Error: cannot read file " << input_filename << std::endl;
        return EXIT_FAILURE;
      }
    }
    else if (extension == ".pwn" || extension == ".PWN")
    {
      // Read the point set file in points[]
      if(!CGAL::read_pwn_point_set(input_filename.c_str(),
                                   std::back_inserter(points)))
      {
        std::cerr << "Error: cannot read file " << input_filename << std::endl;
        return EXIT_FAILURE;
      }
    }
    else
    {
      std::cerr << "Error: cannot read file " << input_filename << std::endl;
      return EXIT_FAILURE;
    }

    // Print status
    long memory = CGAL::Memory_sizer().virtual_size();
    int nb_vertices = points.size();
    std::cerr << "Read file " << input_filename << ": " << nb_vertices << " vertices, "
                                                        << task_timer.time() << " seconds, "
                                                        << (memory>>20) << " Mb allocated"
                                                        << std::endl;
    task_timer.reset();

    //***************************************
    // Check requirements
    //***************************************

    if (nb_vertices == 0)
    {
      std::cerr << "Error: empty file" << std::endl;
      return EXIT_FAILURE;
    }

    assert(points.begin() != points.end());
    bool points_have_normals = (points.begin()->normal() != CGAL::NULL_VECTOR);
    if ( ! points_have_normals )
    {
      std::cerr << "Input point set not supported: this reconstruction method requires oriented normals" << std::endl;
      return EXIT_FAILURE;
    }

    //***************************************
    // Compute implicit function
    //***************************************

    std::cerr << "Create triangulation...\n";

    // Create implicit function and triangulation.
    // Insert vertices and normals in triangulation.
    Dt3 dt;
    Poisson_reconstruction_function poisson_function(dt, points.begin(), points.end());

    // Recover memory used by points[]
    points.clear();

    // Print status
    /*long*/ memory = CGAL::Memory_sizer().virtual_size();
    std::cerr << "Create triangulation: " << task_timer.time() << " seconds, "
                                          << (memory>>20) << " Mb allocated"
                                          << std::endl;
    task_timer.reset();

    std::cerr << "Compute implicit function...\n";

    /// Compute the Poisson indicator function f()
    /// at each vertex of the triangulation.
    if ( ! poisson_function.compute_implicit_function() )
    {
      std::cerr << "Error: cannot compute implicit function" << std::endl;
      return EXIT_FAILURE;
    }

    // Print status
    /*long*/ memory = CGAL::Memory_sizer().virtual_size();
    std::cerr << "Compute implicit function: " << task_timer.time() << " seconds, "
                                               << (memory>>20) << " Mb allocated"
                                               << std::endl;
    task_timer.reset();

    //***************************************
    // Surface mesh generation
    //***************************************

    std::cerr << "Surface meshing...\n";

    STr tr; // 3D-Delaunay triangulation for Surface Mesher
    C2t3 surface_mesher_c2t3 (tr); // 2D-complex in 3D-Delaunay triangulation

    // Get inner point
    Point inner_point = poisson_function.get_inner_point();
    FT inner_point_value = poisson_function(inner_point);
    if(inner_point_value >= 0.0)
    {
      std::cerr << "Error: unable to seed (" << inner_point_value << " at inner_point)" << std::endl;
      return EXIT_FAILURE;
    }

    // Get implicit surface's radius
    Sphere bounding_sphere = poisson_function.bounding_sphere();
    FT size = sqrt(bounding_sphere.squared_radius());

    // defining the surface
    Point sm_sphere_center = inner_point; // bounding sphere centered at inner_point
    FT    sm_sphere_radius = 2 * size;
    sm_sphere_radius *= 1.1; // <= the Surface Mesher fails if the sphere does not contain the surface
    Surface_3 surface(poisson_function,
                      Sphere(sm_sphere_center,sm_sphere_radius*sm_sphere_radius),
                      sm_error_bound_poisson*size/sm_sphere_radius); // dichotomy stops when segment < sm_error_bound_poisson*size

    // defining meshing criteria
    CGAL::Surface_mesh_default_criteria_3<STr> criteria(sm_angle_poisson,  // lower bound of facets angles (degrees)
                                                        sm_radius_poisson*size,  // upper bound of Delaunay balls radii
                                                        sm_distance_poisson*size); // upper bound of distance to surface

    CGAL_TRACE_STREAM << "  make_surface_mesh(dichotomy error="<<sm_error_bound_poisson<<" * point set radius,\n"
                      << "                    sphere center=("<<sm_sphere_center << "),\n"
                      << "                    sphere radius="<<sm_sphere_radius/size<<" * p.s.r.,\n"
                      << "                    angle="<<sm_angle_poisson << " degrees,\n"
                      << "                    radius="<<sm_radius_poisson<<" * p.s.r.,\n"
                      << "                    distance="<<sm_distance_poisson<<" * p.s.r.,\n"
                      << "                    Manifold_with_boundary_tag)\n";

    // meshing surface
    CGAL::make_surface_mesh(surface_mesher_c2t3, surface, criteria, CGAL::Manifold_with_boundary_tag());

    // Print status
    /*long*/ memory = CGAL::Memory_sizer().virtual_size();
    std::cerr << "Surface meshing: " << task_timer.time() << " seconds, "
                                     << tr.number_of_vertices() << " output vertices, "
                                     << (memory>>20) << " Mb allocated"
                                     << std::endl;
    task_timer.reset();

    //***************************************
    // save the mesh
    //***************************************

    std::cerr << "Write file " << output_filename << std::endl << std::endl;

    std::ofstream out(output_filename.c_str());
    CGAL::output_surface_facets_to_off(out, surface_mesher_c2t3);

    return EXIT_SUCCESS;
}
