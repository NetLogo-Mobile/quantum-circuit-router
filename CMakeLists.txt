cmake_minimum_required(VERSION 3.18)
project(qcrouter CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# All libavoid translation units (matches qcrouter/qcrouter.vcxproj) plus the engine.
set(SOURCES
    qcrouter.cpp
    libavoid/aca3.cpp
    libavoid/actioninfo.cpp
    libavoid/block.cpp
    libavoid/blocks.cpp
    libavoid/box.cpp
    libavoid/cbuffer.cpp
    libavoid/cc_clustercontainmentconstraints.cpp
    libavoid/cc_nonoverlapconstraints.cpp
    libavoid/cluster.cpp
    libavoid/cola.cpp
    libavoid/colafd.cpp
    libavoid/cola_topology_addon.cpp
    libavoid/compound_constraints.cpp
    libavoid/compute_forces.cpp
    libavoid/conjugate_gradient.cpp
    libavoid/connected_components.cpp
    libavoid/connectionpin.cpp
    libavoid/connector.cpp
    libavoid/connend.cpp
    libavoid/constraint.cpp
    libavoid/convex_hull.cpp
    libavoid/geometry.cpp
    libavoid/geomtypes.cpp
    libavoid/gradient_projection.cpp
    libavoid/graph.cpp
    libavoid/hyperedge.cpp
    libavoid/hyperedgeimprover.cpp
    libavoid/hyperedgetree.cpp
    libavoid/junction.cpp
    libavoid/makepath.cpp
    libavoid/mtst.cpp
    libavoid/obstacle.cpp
    libavoid/orthogonal.cpp
    libavoid/orthogonal_topology.cpp
    libavoid/output_svg.cpp
    libavoid/pseudorandom.cpp
    libavoid/rectangle.cpp
    libavoid/resize.cpp
    libavoid/router.cpp
    libavoid/scanline.cpp
    libavoid/shape.cpp
    libavoid/shapepair.cpp
    libavoid/solve_VPSC.cpp
    libavoid/straightener.cpp
    libavoid/stress_terms.cpp
    libavoid/timer.cpp
    libavoid/topology_constraints.cpp
    libavoid/topology_constraints_constructor.cpp
    libavoid/topology_graph.cpp
    libavoid/variable.cpp
    libavoid/vertices.cpp
    libavoid/viscluster.cpp
    libavoid/visibility.cpp
    libavoid/vpsc.cpp
)

add_library(qcrouter SHARED ${SOURCES})

target_include_directories(qcrouter PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libavoid
)

# Android log + math. The 5 exported C functions rely on clang's default
# visibility (CIVITAS_EXPORT is empty on non-MSVC), so do NOT hide symbols.
target_link_libraries(qcrouter log m)

# --- 16 KB page-size support ---------------------------------------------
# Unity's bundled NDK is r23 (< r28), so 16 KB alignment is not the default.
# Force ELF LOAD segments to align to 16384 (2**14) bytes.
target_link_options(qcrouter PRIVATE
    -Wl,-z,max-page-size=16384
    -Wl,-z,common-page-size=16384
)

# Decade-old C++11 sources + clang 12: keep warnings non-fatal.
target_compile_options(qcrouter PRIVATE -w)
