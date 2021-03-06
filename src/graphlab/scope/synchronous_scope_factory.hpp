#ifndef SYNCHRONOUS_SCOPE_FACTORY
#define SYNCHRONOUS_SCOPE_FACTORY
#include <vector>
#include <graphlab/scope/iscope.hpp>
#include <graphlab/scope/iscope_factory.hpp>
#include <graphlab/scope/synchronous_scope.hpp>
#include <graphlab/graph/graph.hpp>

namespace graphlab {
  /**
   * This defines a scope type which is meant for "synchronous" type of 
   * algorithms. This type of scope should only be used by synchronous_engine
   */
  template<typename Graph>
  class synchronous_scope_factory : 
    public iscope_factory<Graph> {
  public:
    typedef iscope_factory<Graph> base;
    typedef typename base::iscope_type iscope_type;
    typedef synchronous_scope<Graph> synchronous_scope_type;
    
    synchronous_scope_factory(Graph& srcgraph, 
                              Graph& destgraph, size_t ncpus) : 
      base(srcgraph, ncpus), 
      _srcgraph(&srcgraph), 
      _destgraph(&destgraph), 
      scopes(ncpus) {  
      // vertexd data graph does not change
      _vertexdatagraph = _destgraph;
    }
    
    ~synchronous_scope_factory() { }

    void set_default_scope(scope_range::scope_range_enum default_scope_range) { };
    
    iscope_type* get_scope(size_t cpuid,
                           vertex_id_t v,
                           scope_range::scope_range_enum scope_range = scope_range::USE_DEFAULT) {
      assert(cpuid < scopes.size());
      synchronous_scope_type& scope = scopes[cpuid];
      scope.init(_srcgraph,_destgraph, _vertexdatagraph, v);
      return &(scope);
    }
    
    void swap_graphs() {
      Graph* tmp = _srcgraph;
      _srcgraph = _destgraph;
      _destgraph = tmp;
    }
    
    Graph* get_src_graph() { return _srcgraph;  }
    Graph* get_dest_graph() { return _destgraph;  }
    Graph* get_vertex_data_graph() { return _vertexdatagraph;  }
    
    void release_scope(iscope_type* scope) { }

    size_t num_vertices() const {
      return _vertexdatagraph->num_vertices();
    }
    
  private:
    Graph* _srcgraph;
    Graph* _destgraph;
    Graph* _vertexdatagraph;
    std::vector<synchronous_scope_type> scopes;  
  };
  
}
#endif
