/*~--------------------------------------------------------------------------~*
 * Copyright (c) 2015 Los Alamos National Security, LLC
 * All rights reserved.
 *~--------------------------------------------------------------------------~*/

#ifndef flexi_burton_h
#define flexi_burton_h

#include "../mesh/mesh_topology.h"
#include "../geometry/point.h"
#include "../geometry/space_vector.h"

/*!
 * \file burton.h
 * \authors bergen
 * \date Initial file creation: Sep 02, 2015
 */

namespace flexi {

  class burton_mesh_types_t{
  public:
    static constexpr size_t dimension = 2;

    using Id = uint64_t;

    using point_t = point<double, dimension>;
    using vector_t = space_vector<double, dimension>;

    // Vertex type
    struct burton_vertex_t : public MeshEntity<0> {
      burton_vertex_t()
        : precedence_(0){}

      burton_vertex_t(const point_t& coordinates)
        : precedence_(0),
        coordinates_(coordinates){}

      void setRank(uint8_t rank){
        precedence_ = 1 << (63 - rank);
      }

      uint64_t precedence() const{
        return precedence_;
      }
      
      void setCoordinates(const point_t& coordinates){
        coordinates_ = coordinates;
      }

      const point_t& coordinates() const{
        return coordinates_;
      }

    private:
      point_t coordinates_;
      uint64_t precedence_;
    }; // struct burton_vertex_t

    // Edge type
    struct burton_edge_t : public MeshEntity<1> {

    }; // struct burton_edge_t

    class burton_corner_t;
    class burton_wedge_t;

    // Cell type
    class burton_cell_t : public MeshEntity<2> {
    public:
      void addCorner(burton_corner_t* c){
        corners_.add(c);
      }

      EntityGroup<burton_corner_t> & corners() {
        return corners_;
      } // getSides

      void addWedge(burton_wedge_t* w){
        wedges_.add(w);
      }

      EntityGroup<burton_wedge_t> & wedges() {
        return wedges_;
      } // getSides

    private:

      EntityGroup<burton_corner_t> corners_;
      EntityGroup<burton_wedge_t> wedges_;

    }; // class burton_cell_t
    
    // Wedge type
    class burton_wedge_t : public MeshEntity<2> {
    public:

      void setCorner(burton_corner_t* corner){
        corner_ = corner;
      }

      burton_corner_t* corner(){
        return corner_;
      }

      vector_t side_facet_normal();
      vector_t cell_facet_normal();

    private:
      burton_corner_t* corner_;
    }; // struct burton_wedge_t

    // Corner type
    class burton_corner_t : public MeshEntity<0> {
    public:
      void addWedge(burton_wedge_t* w){
        wedges_.add(w);
        w->setCorner(this);
      }

      EntityGroup<burton_wedge_t> & wedges() {
        return wedges_;
      } // wedges

    private:

      EntityGroup<burton_wedge_t> wedges_;

    }; // class burton_corner_t

    using EntityTypes =
      std::tuple<burton_vertex_t, burton_edge_t, burton_cell_t>;

    static size_t numEntitiesPerCell(size_t dim){
      switch(dim){
      case 0:
        return 4;
      case 1:
        return 4;
      case 2:
        return 1;
      default:
        assert(false && "invalid dimension");
      }
    }

    static constexpr size_t verticesPerCell(){
      return 4;
    }
  
    static size_t numVerticesPerEntity(size_t dim){
      switch(dim){
      case 0:
        return 1;
      case 1:
        return 2;
      case 2:
        return 4;
      default:
        assert(false && "iznvalid dimension");
      }
    }
  
    static void createEntities(size_t dim,
                               std::vector<Id>& e,
                               burton_vertex_t** v){
      assert(dim = 1);
      assert(e.size() == 8);

      struct Edge_{
        Edge_(burton_vertex_t* v1, burton_vertex_t* v2)
          : v1(v1), v2(v2){}
        
        burton_vertex_t* v1;
        burton_vertex_t* v2;

        uint64_t precedence() const{
          return v1->precedence() | v2->precedence(); 
        }
      };

      std::vector<Edge_> es;
      es.emplace_back(Edge_(v[0], v[1]));
      es.emplace_back(Edge_(v[1], v[3]));
      es.emplace_back(Edge_(v[0], v[1]));
      es.emplace_back(Edge_(v[2], v[3]));

      std::sort(es.begin(), es.end(), [](const Edge_& e1, const Edge_& e2){
          return e1.precedence() > e2.precedence();
        }); 

      e[0] = es[0].v1->id();
      e[1] = es[0].v2->id();

      e[2] = es[1].v1->id();
      e[3] = es[1].v2->id();

      e[4] = es[2].v1->id();
      e[5] = es[2].v2->id();    

      e[6] = es[3].v1->id();
      e[7] = es[3].v2->id();
    }
  };

 class dual_mesh_types_t{
  public:
    static constexpr size_t dimension = burton_mesh_types_t::dimension;

    using Id = uint64_t;

    using point_t = point<double, burton_mesh_types_t::dimension>;

    using burton_vertex_t = burton_mesh_types_t::burton_vertex_t;

    using burton_edge_t = burton_mesh_types_t::burton_edge_t;

    using burton_wedge_t = burton_mesh_types_t::burton_wedge_t;
  
    using EntityTypes =
      std::tuple<burton_vertex_t, burton_edge_t, burton_wedge_t>;

    static size_t numEntitiesPerCell(size_t dim){
      switch(dim){
      case 0:
        return 3;
      case 1:
        return 3;
      case 2:
        return 1;
      default:
        assert(false && "invalid dimension");
      }
    }

    static constexpr size_t verticesPerCell(){
      return 4;
    }
  
    static size_t numVerticesPerEntity(size_t dim){
      switch(dim){
      case 0:
        return 1;
      case 1:
        return 2;
      case 2:
        return 3;
      default:
        assert(false && "invalid dimension");
      }
    }
  
    static void createEntities(size_t dim,
                               std::vector<Id>& e,
                               burton_vertex_t** v){
      assert(dim = 1);
      assert(e.size() == 6);

      struct Edge_{
        Edge_(burton_vertex_t* v1, burton_vertex_t* v2)
          : v1(v1), v2(v2){}
        
        burton_vertex_t* v1;
        burton_vertex_t* v2;

        uint64_t precedence() const{
          return v1->precedence() | v2->precedence(); 
        }
      };

      std::vector<Edge_> es;
      es.emplace_back(Edge_(v[0], v[2]));
      es.emplace_back(Edge_(v[1], v[3]));
      es.emplace_back(Edge_(v[0], v[1]));

      std::sort(es.begin(), es.end(), [](const Edge_& e1, const Edge_& e2){
          return e1.precedence() > e2.precedence();
        });

      e[0] = es[0].v1->id();
      e[1] = es[0].v2->id();

      e[2] = es[1].v1->id();
      e[3] = es[1].v2->id();

      e[4] = es[2].v1->id();
      e[5] = es[2].v2->id();
    }
  };


/*!
  \class burton_mesh_t burton.h
  \brief burton_mesh_t provides...
 */
class burton_mesh_t
{
private:

  using private_mesh_t = MeshTopology<burton_mesh_types_t>;
  using private_dual_mesh_t = MeshTopology<dual_mesh_types_t>;

public:

  using point_t = point<double, burton_mesh_types_t::dimension>;

  using vertex_t = burton_mesh_types_t::burton_vertex_t;
  using edge_t = burton_mesh_types_t::burton_edge_t;
  using cell_t = burton_mesh_types_t::burton_cell_t;
  using wedge_t = burton_mesh_types_t::burton_wedge_t;
  using corner_t = burton_mesh_types_t::burton_corner_t;

  // Iterator types
  using vertex_iterator_t = private_mesh_t::VertexIterator;
  using edge_iterator_t = private_mesh_t::EdgeIterator;
  using cell_iterator_t = private_mesh_t::CellIterator;

  using wedges_at_corner_iterator_t = private_dual_mesh_t::CellIterator;
  using wedges_at_face_iterator_t = private_mesh_t::CellIterator;
  using wedges_at_vertex_iterator_t = private_mesh_t::CellIterator;
  using wedges_at_cell_iterator_t = private_mesh_t::CellIterator;

  //! Default constructor
  burton_mesh_t() {}

  //! Copy constructor (disabled)
  burton_mesh_t(const burton_mesh_t &) = delete;

  //! Assignment operator (disabled)
  burton_mesh_t & operator = (const burton_mesh_t &) = delete;

  //! Destructor
  ~burton_mesh_t() {}

  auto vertices(){
    return mesh_.vertices();
  }

  auto edges(){
    return mesh_.edges();
  }

  auto cells(){
    return mesh_.cells();
  }

  template<class E>
  auto vertices(E* e){
    return mesh_.vertices(e);
  }

  auto vertices(wedge_t* w){
    return dual_mesh_.vertices(w);
  }

  template<class E>
  auto edges(E* e){
    return mesh_.edges(e);
  }

  template<class E>
  auto cells(E* e){
    return mesh_.cells(e);
  }

  vertex_t* create_vertex(const point_t& pos){
    auto v = mesh_.make<vertex_t>(pos);
    mesh_.addVertex(v);
    return v;
  }

  cell_t* create_cell(std::initializer_list<vertex_t*> verts){
    assert(verts.size() == burton_mesh_types_t::verticesPerCell() && 
           "vertices size mismatch");
    auto c = mesh_.make<cell_t>();
    mesh_.addCell(c, verts);
    return c;
  }

  void init(){
    for(auto c : mesh_.cells()){
      auto vs = mesh_.vertices(c).toVec();

      dual_mesh_.addVertex(vs[0]);
      dual_mesh_.addVertex(vs[1]);
      dual_mesh_.addVertex(vs[2]);
      dual_mesh_.addVertex(vs[3]);

      point_t cp;
      cp[0] = vs[0]->coordinates()[0] + 0.5*(vs[2]->coordinates()[0] - vs[0]->coordinates()[0]);
      cp[1] = vs[0]->coordinates()[1] + 0.5*(vs[1]->coordinates()[1] - vs[0]->coordinates()[1]);

      auto cv = dual_mesh_.make<vertex_t>(cp);
      dual_mesh_.addVertex(cv);
      cv->setRank(0);

      auto w1 = dual_mesh_.make<wedge_t>();
      dual_mesh_.addCell(w1, {vs[0], vs[1], cv});
      c->addWedge(w1);

      auto w2 = dual_mesh_.make<wedge_t>();
      dual_mesh_.addCell(w2, {cv, vs[1], vs[3]});
      c->addWedge(w2);

      auto w3 = dual_mesh_.make<wedge_t>();
      dual_mesh_.addCell(w3, {vs[2], cv, vs[3]});
      c->addWedge(w3);

      auto w4 = dual_mesh_.make<wedge_t>();
      dual_mesh_.addCell(w4, {vs[0], cv, vs[2]});
      c->addWedge(w4);

      auto c1 = dual_mesh_.make<corner_t>();
      c1->addWedge(w1);
      c1->addWedge(w4);
      c->addCorner(c1);

      auto c2 = dual_mesh_.make<corner_t>();
      c2->addWedge(w1);
      c2->addWedge(w2);
      c->addCorner(c2);

      auto c3 = dual_mesh_.make<corner_t>();
      c3->addWedge(w2);
      c3->addWedge(w3);
      c->addCorner(c3);

      auto c4 = dual_mesh_.make<corner_t>();
      c4->addWedge(w3);
      c4->addWedge(w4);
      c->addCorner(c4);
    }
  }

private:

  private_mesh_t mesh_;
  private_dual_mesh_t dual_mesh_;
}; // class burton_mesh_t

using mesh_t = burton_mesh_t;

} // namespace flexi

#endif // flexi_burton_h

/*~-------------------------------------------------------------------------~-*
 * Formatting options for vim.
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~-------------------------------------------------------------------------~-*/
